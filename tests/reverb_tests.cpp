#include "revmodel.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

static void testDryAndBypassLaw() {
  revmodel r;
  r.setwet(0.f); r.setdry(0.5f); r.mute();
  float l[4]={.1f,-.2f,.5f,.9f},rr[4]={-.1f,.2f,-.5f,-.9f},ol[4]{},orr[4]{};
  r.processreplace(l,rr,ol,orr,4,1);
  for(int i=0;i<4;++i){ assert(ol[i]==l[i]); assert(orr[i]==rr[i]); }
  r.setdry(1.f);
  r.processreplace(l,rr,ol,orr,4,1);
  for(int i=0;i<4;++i){ assert(std::abs(ol[i]-l[i]*2)<1e-6f); assert(std::abs(orr[i]-rr[i]*2)<1e-6f); }
}

static void testTailAndReset() {
  revmodel r;
  r.setroomsize(.8f);r.setdamp(.2f);r.setwidth(1.f);r.setwet(1.f);r.setdry(0.f);r.mute();
  constexpr int samples=32768;
  std::vector<float> il(samples),ir(samples),ol(samples),orr(samples);
  il[0]=ir[0]=.5f;
  r.processreplace(il.data(),ir.data(),ol.data(),orr.data(),samples,1);
  bool tail=false,stereo=false;
  for(int i=1;i<samples;++i){
    assert(std::isfinite(ol[i])&&std::isfinite(orr[i]));
    tail |= std::abs(ol[i])>1e-9f || std::abs(orr[i])>1e-9f;
    stereo |= std::abs(ol[i]-orr[i])>1e-9f;
  }
  assert(tail && stereo);
  const auto refL=ol,refR=orr;
  r.mute();std::fill(ol.begin(),ol.end(),0);std::fill(orr.begin(),orr.end(),0);
  for(int i=0;i<samples;i+=31){
    int n=std::min(31,samples-i);
    r.processreplace(il.data()+i,ir.data()+i,ol.data()+i,orr.data()+i,n,1);
  }
  for(int i=0;i<samples;++i){assert(ol[i]==refL[i]);assert(orr[i]==refR[i]);}
}

static void testCallbacks() {
  revmodel r;r.setwet(.4f);r.setdry(.6f);
  float l[256]{},rr[256]{},ol[256]{},orr[256]{};
  l[0]=rr[0]=.7f;
  for(int n=0;n<1200;++n){
    r.processreplace(l,rr,ol,orr,256,1);
    for(int j=0;j<256;++j){assert(std::isfinite(ol[j]));assert(std::isfinite(orr[j]));}
    std::fill(std::begin(l),std::end(l),0.f);
    std::fill(std::begin(rr),std::end(rr),0.f);
  }
}
int main(){testDryAndBypassLaw();testTailAndReset();testCallbacks();std::puts("PASS original Freeverb core: dry, tail, stereo, reset, fragmentation, 1200 blocks");}
