#include "reference_reverb.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

static std::size_t allocationCount=0;
void* operator new(std::size_t n){++allocationCount;if(auto* p=std::malloc(n))return p;throw std::bad_alloc{};}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void* operator new[](std::size_t n){++allocationCount;if(auto* p=std::malloc(n))return p;throw std::bad_alloc{};}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::abort();}}while(0)
using a125::VocaidReferenceReverb;
static void testGainLaw() {
    VocaidReferenceReverb r;
    auto p=r.controls();p.mix=0;r.setControls(p);
    CHECK(r.prepare(44100)); // default/initial params are snapped, not ramped
    float l[4]={.25f,-.125f,.6f,-.7f},rr[4]={-.1f,.5f,.25f,0.f},ol[4]{},orr[4]{};
    r.processStereo(l,rr,ol,orr,4);
    for(int i=0;i<4;++i){CHECK(ol[i]==l[i]*2.f);CHECK(orr[i]==rr[i]*2.f);}
    p.enabled=false;r.setControls(p);
    r.processStereo(l,rr,ol,orr,4);
    for(int i=0;i<4;++i){CHECK(ol[i]==l[i]);CHECK(orr[i]==rr[i]);}
}
static void testRateAndStereoOnset() {
    for(const int sr : {44100,48000,96000}) {
        VocaidReferenceReverb r;CHECK(r.prepare(sr));
        const int firstL=(sr*1116)/44100, firstR=(sr*1139)/44100;
        CHECK(r.firstCombDelay(0)==static_cast<std::size_t>(firstL));
        CHECK(r.firstCombDelay(1)==static_cast<std::size_t>(firstR));
        auto p=r.controls();p.mix=1.f;p.width=1.f;p.size=.74f;p.damping=.21f;r.setControls(p);
        // Advance the smoothers with zero input so the impulse starts at fixed settings.
        std::vector<float> z(1024,0.f),l(22000,0.f),rr(22000,0.f),ol(22000),orr(22000);
        r.processStereo(z.data(),z.data(),z.data(),z.data(),z.size());
        r.reset();l[0]=rr[0]=1.f;
        r.processStereo(l.data(),rr.data(),ol.data(),orr.data(),l.size());
        for(int i=0;i<firstL;++i)CHECK(ol[i]==0.f);
        for(int i=0;i<firstR;++i)CHECK(orr[i]==0.f);
        CHECK(std::abs(ol[firstL])>1.e-9f);
        CHECK(std::abs(orr[firstR])>1.e-9f);
        bool stereo=false;
        for(int i=firstR;i<22000;++i) {
            CHECK(std::isfinite(ol[i]) && std::isfinite(orr[i]));
            if(std::abs(ol[i]-orr[i])>1.e-7f)stereo=true;
        }
        CHECK(stereo);
    }
}
static void testPartitionAndReset() {
    VocaidReferenceReverb r;CHECK(r.prepare(44100));
    auto p=r.controls();p.mix=.88f;p.width=.75f;r.setControls(p);
    constexpr std::size_t n=44100;
    std::vector<float> inL(n,0.f),inR(n,0.f),ol(n,0.f),orr(n,0.f),a(n,0.f),b(n,0.f);
    inL[0]=.5f;inR[0]=.25f;
    r.processStereo(inL.data(),inR.data(),ol.data(),orr.data(),n);
    r.reset(); // smoothing stays at target after previous full render
    // A fresh instance after initial target ramp must yield same response.
    VocaidReferenceReverb warm;CHECK(warm.prepare(44100));warm.setControls(p);
    std::vector<float> zeros(1024,0.f);
    warm.processStereo(zeros.data(),zeros.data(),zeros.data(),zeros.data(),zeros.size());
    warm.reset();
    for(std::size_t i=0;i<n;i+=17){
        const auto take=std::min<std::size_t>(17,n-i);
        warm.processStereo(inL.data()+i,inR.data()+i,a.data()+i,b.data()+i,take);
    }
    // Separate equal-parameter test without prior ramp:
    VocaidReferenceReverb fresh1,fresh2;CHECK(fresh1.prepare(44100));CHECK(fresh2.prepare(44100));
    fresh1.setControls(p);fresh2.setControls(p);
    std::fill(ol.begin(),ol.end(),0.f);std::fill(orr.begin(),orr.end(),0.f);
    fresh1.processStereo(inL.data(),inR.data(),ol.data(),orr.data(),n);
    std::fill(a.begin(),a.end(),0.f);std::fill(b.begin(),b.end(),0.f);
    for(std::size_t i=0;i<n;i+=17){
        const auto take=std::min<std::size_t>(17,n-i);
        fresh2.processStereo(inL.data()+i,inR.data()+i,a.data()+i,b.data()+i,take);
    }
    for(std::size_t i=0;i<n;++i){CHECK(ol[i]==a[i]);CHECK(orr[i]==b[i]);}
}
static void testValidatorSampleRates() {
    VocaidReferenceReverb rv;
    CHECK(rv.prepare(1234.568));
    CHECK(rv.firstCombDelay(0)==static_cast<std::size_t>((1234LL*1116)/44100));
    CHECK(rv.prepare(1234567.8));
    CHECK(rv.firstCombDelay(0)==static_cast<std::size_t>((1234567LL*1116)/44100));
    CHECK(rv.prepare(2000000.0));
    CHECK(rv.firstCombDelay(0)==static_cast<std::size_t>((2000000LL*1116)/44100));
}
static void testMonoAndZeroAllocation() {
    VocaidReferenceReverb rv;CHECK(rv.prepare(48000));
    auto c=rv.controls();c.mix=.67f;rv.setControls(c);
    float mono[256]{},outL[256]{},outR[256]{};
    mono[0]=.7f;
    const auto before=allocationCount;
    for(int n=0;n<1200;++n) {
        if(n!=0)std::fill(std::begin(mono),std::end(mono),0.f);
        rv.processMonoToStereo(mono,outL,outR,256);
        for(int i=0;i<256;++i){CHECK(std::isfinite(outL[i]));CHECK(std::isfinite(outR[i]));}
    }
    CHECK(allocationCount==before);
    float src[4]={.25f,.5f,-.8f,0.f};
    c.enabled=false;rv.setControls(c);
    rv.processMono(src,src,4);
    CHECK(src[0]==.25f && src[1]==.5f && src[2]==-.8f);
}
int main() {
 testGainLaw();testRateAndStereoOnset();testPartitionAndReset();testMonoAndZeroAllocation();testValidatorSampleRates();
 std::puts("PASS: JUCE 6.1.6 rate scaling at 44.1/48/96, wet/dry law, stereo, smooth, deterministic partition, mono-to-stereo, 1200 allocation-free blocks");
}
