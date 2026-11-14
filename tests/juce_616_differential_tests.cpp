#include "juce_reverb_shim.hpp"
#include "juce_Reverb.h" // untouched original ISC-licensed source from JUCE 6.1.6
#include "reference_reverb.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define REQUIRE(c) do {if(!(c)){std::fprintf(stderr,"FAIL: %s at %s:%d\n",#c,__FILE__,__LINE__);std::abort();}}while(false)
static std::uint32_t rng=0x12345678u;
static float inputValue() noexcept {
    rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
    return static_cast<float>((rng&0xFFFFu)/65535.0*0.2-0.1);
}
static void setBoth(juce::Reverb& original,a125::VocaidReferenceReverb& ours,
                    float size,float damp,float width,float mix) {
    juce::Reverb::Parameters a;
    // 125A presentation law: dry output is unity at 0% Mix; JUCE's
    // original kernel internally multiplies dryLevel by two.
    a.roomSize=size;a.damping=damp;a.width=width;a.wetLevel=mix;a.dryLevel=(1.f-mix)*.5f;
    original.setParameters(a);
    auto b=ours.controls();
    b.size=size;b.damping=damp;b.width=width;b.mix=mix;
    ours.setControls(b);
}
static void checkStereo(double sr) {
    juce::Reverb original;
    a125::VocaidReferenceReverb ours;
    setBoth(original,ours,.65f,.37f,.88f,.68f);
    original.setSampleRate(sr); // JUCE smoothers snap to initial parameter targets.
    REQUIRE(ours.prepare(sr));
    std::vector<float> aL(50121),aR(50121),bL(50121),bR(50121);
    float maxError=0.f;
    constexpr int n=50121;
    for(int pos=0;pos<n;) {
        if(pos==0) { // bootstrap impulse, diffuse before controls change
            aL[0]=.3f; aR[0]=-.2f;
        }
        const int remain=n-pos;
        int block=std::min(remain, 1+static_cast<int>((pos*31u+7u)%513u));
        for(int i=0;i<block;++i) {
            const auto k=pos+i;
            aL[k]=inputValue();aR[k]=inputValue();
            if(k==0){aL[k]+=.4f;aR[k]+=.2f;}
            if(k==25000){aL[k]+=.45f;aR[k]-=.25f;}
            bL[k]=aL[k];bR[k]=aR[k];
        }
        // Automation and reset only on sample-defined boundaries.
        if(pos>=16000 && pos<16000+block) { // boundary may be between blocks: skip to avoid different parameter positions
        }
        original.processStereo(aL.data()+pos,aR.data()+pos,block);
        ours.processStereo(bL.data()+pos,bR.data()+pos,bL.data()+pos,bR.data()+pos,block);
        for(int i=0;i<block;++i) {
            const auto k=pos+i;
            const float dx=std::abs(aL[k]-bL[k]),dy=std::abs(aR[k]-bR[k]);
            maxError=std::max({maxError,dx,dy});
            if(!(dx<=3.e-6f && dy<=3.e-6f)) {
                std::fprintf(stderr,"Mismatch stereo @ %.3f Hz index %d, ref=(%.8f %.8f) ours=(%.8f %.8f) errors=(%g %g)\n",
                             sr,k,aL[k],aR[k],bL[k],bR[k],dx,dy);
                std::abort();
            }
        }
        pos+=block;
        if(pos>15000 && pos<15520) setBoth(original,ours,.21f,.88f,.12f,.90f);
        if(pos>31000 && pos<31600) setBoth(original,ours,.99f,.12f,1.f,.25f);
        if(pos>40000 && pos<40550) {original.reset();ours.reset();}
    }
    std::printf("PASS stereo JUCE 6.1.6 differential @ %.3f Hz, 50k frames, max error %.9g\n",sr,maxError);
}
static void checkMono(double sr) {
    juce::Reverb original;
    a125::VocaidReferenceReverb ours;
    setBoth(original,ours,.77f,.45f,.5f,.81f);
    original.setSampleRate(sr);
    REQUIRE(ours.prepare(sr));
    std::vector<float> a(32768),b(32768);
    float maxError=0.f;
    for(int pos=0;pos<static_cast<int>(a.size());) {
        const int block=std::min(static_cast<int>(a.size())-pos,1+(pos*7%201));
        for(int i=0;i<block;++i) {
            const int k=pos+i;
            a[k]=inputValue() + (k==0?.5f:0.f);
            b[k]=a[k];
        }
        original.processMono(a.data()+pos,block);
        ours.processMono(b.data()+pos,b.data()+pos,block);
        for(int i=0;i<block;++i) {
            const float d=std::abs(a[pos+i]-b[pos+i]);maxError=std::max(maxError,d);
            REQUIRE(d<=3.e-6f);
        }
        pos+=block;
        if(pos>12000 && pos<12200)setBoth(original,ours,.38f,.63f,.2f,.15f);
    }
    std::printf("PASS mono JUCE 6.1.6 differential @ %.3f Hz, 32k frames, max error %.9g\n",sr,maxError);
}
int main(){
    for(double sr : {44100.0,48000.0,96000.0,22050.0,12345.678}) {
        checkStereo(sr);
        checkMono(sr);
    }
    std::puts("PASS: JUCE 6.1.6 ISC Reverb reference vs 125A Vocaid Reference DSP");
}
