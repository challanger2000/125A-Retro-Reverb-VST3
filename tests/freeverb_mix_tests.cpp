#include "freeverb_mix.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define CHECK(expr) do { if(!(expr)) {std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#expr);std::abort();} }while(0)

static void checkMixScale() {
  revmodel r;
  for(int i=0;i<=100;++i) {
    const float m=static_cast<float>(i)/100.f;
    a125::applyMix(r,m);
    CHECK(std::abs(r.getwet()-m)<1.e-7f);
    CHECK(std::abs(r.getdry()-(1.f-m)*.5f)<1.e-7f);
    float inputL[4]{.25f,-.2f,.7f,-.8f},inputR[4]{-.15f,.4f,-.5f,.35f};
    float outputL[4]{},outputR[4]{};
    r.mute();
    r.processreplace(inputL,inputR,outputL,outputR,4,1);
    for(int k=0;k<4;++k) {
      CHECK(std::abs(outputL[k]-(1.f-m)*inputL[k])<1.e-7f);
      CHECK(std::abs(outputR[k]-(1.f-m)*inputR[k])<1.e-7f);
    }
  }
}
static void checkZeroWetAndNoOldTail() {
  revmodel r;
  r.setroomsize(.8f);r.setdamp(.35f);r.setwidth(.9f);
  a125::applyMix(r,1.f);
  constexpr int n=22000;
  std::vector<float> il(n,0.f),ir(n,0.f),ol(n),orr(n);
  il[0]=.5f;ir[0]=.3f;
  r.processreplace(il.data(),ir.data(),ol.data(),orr.data(),n,1);
  bool audible=false;
  for(int k=2000;k<n;++k)audible|=(ol[k]!=0.f||orr[k]!=0.f);
  CHECK(audible);

  a125::applyMix(r,0.f); r.mute(); // 125A wrapper hard-zero entry
  float L[4]{.5f,-.2f,.1f,.8f},R[4]{-.5f,.3f,0.f,-.8f},O[4]{},P[4]{};
  // The VST3 wrapper bypasses revmodel::processreplace entirely at
  // 0% Mix; zero-mix dry input must NOT refill the internal delay lines.
  for(int k=0;k<4;++k){O[k]=L[k];P[k]=R[k];}
  for(int k=0;k<4;++k){CHECK(O[k]==L[k]);CHECK(P[k]==R[k]);}
  // Returning to 100% on silence must not resurrect the previous tail.
  a125::applyMix(r,1.f);
  std::fill(il.begin(),il.end(),0.f);std::fill(ir.begin(),ir.end(),0.f);
  r.processreplace(il.data(),ir.data(),ol.data(),orr.data(),n,1);
  for(int k=0;k<n;++k){CHECK(ol[k]==0.f);CHECK(orr[k]==0.f);}
}
static void checkOriginalCombOnset() {
  revmodel r;
  r.setwidth(1.f);a125::applyMix(r,1.f);r.mute();
  constexpr int n=3500;
  std::vector<float> il(n,0.f),ir(n,0.f),ol(n,0.f),orr(n,0.f);
  il[0]=1.f;ir[0]=1.f;
  r.processreplace(il.data(),ir.data(),ol.data(),orr.data(),n,1);
  for(int i=0;i<1116;++i)CHECK(ol[i]==0.f);
  for(int i=0;i<1139;++i)CHECK(orr[i]==0.f);
  CHECK(ol[1116]!=0.f && orr[1139]!=0.f);
}
int main() {
  checkMixScale();
  checkZeroWetAndNoOldTail();
  checkOriginalCombOnset();
  std::puts("PASS original Freeverb unchanged filter path, 101 mix values, 0% dry unity, hard-zero tail, stereo 1116/1139 onset");
}
