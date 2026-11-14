// Compiles the production RetroProcessor.cpp, not an independent reverb model.
// Only GUI function bodies are stubbed; full plugin GUI is validated separately.
#include "plugin/retro_processor.hpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"
#include <array>
#include <atomic>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#define CHECK(c) do {if(!(c)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#c);std::abort();}}while(0)
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace a125 {
IPlugView* PLUGIN_API RetroProcessor::createView(const char*) {return nullptr;}
void RetroProcessor::didOpen(VSTGUI::VST3Editor*) {}
void RetroProcessor::willClose(VSTGUI::VST3Editor*) {}
VSTGUI::CView* RetroProcessor::createCustomView(VSTGUI::UTF8StringPtr,
  const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
  VSTGUI::VST3Editor*) {return nullptr;}
tresult PLUGIN_API RetroProcessor::getEditorState(IBStream*) {return kResultFalse;}
tresult PLUGIN_API RetroProcessor::setEditorState(IBStream*) {return kResultFalse;}
}
struct Plugin {
  a125::RetroProcessor p;
  Plugin() {
    CHECK(p.initialize(nullptr)==kResultOk);
    ProcessSetup s{};
    s.processMode=kRealtime;s.symbolicSampleSize=kSample32;
    s.sampleRate=48000;s.maxSamplesPerBlock=4096;
    CHECK(p.setupProcessing(s)==kResultOk);
    CHECK(p.setActive(true)==kResultOk);
  }
  ~Plugin(){CHECK(p.terminate()==kResultOk);}
  void flush(ParamID id,ParamValue value) {
    ParameterChanges c(1);int32 qi=0,pi=0;
    auto* q=c.addParameterData(id,qi);CHECK(q!=nullptr);
    CHECK(q->addPoint(0,value,pi)==kResultTrue);
    ProcessData d{};d.inputParameterChanges=&c;CHECK(p.process(d)==kResultOk);
  }
};
template<size_t N> struct Buffer {
  std::array<float,N> il{},ir{},ol{},orr{};
  std::array<float*,2> ip{il.data(),ir.data()},op{ol.data(),orr.data()};
  AudioBusBuffers in{},out{};
  ProcessData d{};
  Buffer(){
    in.numChannels=out.numChannels=2;
    in.channelBuffers32=ip.data();out.channelBuffers32=op.data();
    d.numSamples=static_cast<int32>(N);d.numInputs=d.numOutputs=1;
    d.symbolicSampleSize=kSample32;d.inputs=&in;d.outputs=&out;
  }
  void run(Plugin& p,IParameterChanges* c=nullptr) {
    d.inputParameterChanges=c;CHECK(p.p.process(d)==kResultOk);
    for(size_t i=0;i<N;++i)CHECK(std::isfinite(ol[i])&&std::isfinite(orr[i]));
  }
};
static bool near(float a,float b){return std::abs(a-b)<2.e-6f;}
static void testStateAndText(){
  Plugin p;String128 s{};ParamValue v=-1;
  CHECK(p.p.getParamStringByValue(a125::kEnable,1.,s)==kResultTrue);
  CHECK(p.p.getParamValueByString(a125::kEnable,s,v)==kResultTrue&&v==1.);
  CHECK(p.p.getParamStringByValue(a125::kEnable,0.,s)==kResultTrue);
  CHECK(p.p.getParamValueByString(a125::kEnable,s,v)==kResultTrue&&v==0.);
  MemoryStream stream;
  CHECK(p.p.getState(&stream)==kResultOk&&stream.getSize()==24);
  CHECK(stream.seek(0,IBStream::kIBSeekSet,nullptr)==kResultOk);
  CHECK(p.p.setComponentState(&stream)==kResultOk);
  CHECK(stream.seek(0,IBStream::kIBSeekSet,nullptr)==kResultOk);
  CHECK(p.p.setState(&stream)==kResultOk);
  unsigned char bad[4]{};MemoryStream corrupted(bad,4);
  CHECK(p.p.setComponentState(&corrupted)!=kResultOk);
}
static void testAutomation(){
  {Plugin p;Buffer<256> b;b.il.fill(.25f);b.ir.fill(.25f);
   ParameterChanges c(1);int32 qi=0,pi=0;
   auto* q=c.addParameterData(a125::kEnable,qi);
   CHECK(q&&q->addPoint(224,0.,pi)==kResultTrue);
   b.run(p,&c);
   for(int i=0;i<224;++i)CHECK(near(b.ol[i],.25f*(2.f/3.f)));
   for(int i=224;i<256;++i)CHECK(b.ol[i]==.25f);
  }
  {Plugin p;Buffer<256> b;b.il.fill(.25f);b.ir.fill(.25f);
   ParameterChanges c(1);int32 qi=0,pi=0;
   auto* q=c.addParameterData(a125::kMix,qi);
   CHECK(q&&q->addPoint(64,0.,pi)==kResultTrue);
   CHECK(q->addPoint(128,1.,pi)==kResultTrue);
   b.run(p,&c);
   for(int i=0;i<64;++i)CHECK(near(b.ol[i],.25f*(2.f/3.f)));
   for(int i=64;i<128;++i)CHECK(b.ol[i]==.25f);
   for(int i=128;i<256;++i)CHECK(b.ol[i]==0.f);
  }
}
static void testNonfinite(){
  Plugin p;p.flush(a125::kMix,1.);
  Buffer<256> first;first.il[0]=std::numeric_limits<float>::quiet_NaN();
  first.ir[0]=std::numeric_limits<float>::infinity();first.run(p);
  for(float x:first.ol)CHECK(x==0.f);for(float x:first.orr)CHECK(x==0.f);
  for(int k=0;k<48;++k){Buffer<256> a;a.run(p);
    for(float x:a.ol)CHECK(x==0.f);for(float x:a.orr)CHECK(x==0.f);}
}
static void testDryUnityAndConcurrentState() {
  Plugin p;
  p.flush(a125::kMix,0.);
  Buffer<256> large;
  large.il.fill(20.f);
  large.ir.fill(-20.f);
  large.run(p);
  for(int i=0;i<256;++i) {
    CHECK(large.ol[i]==20.f && large.orr[i]==-20.f);
  }
  p.flush(a125::kMix,1.);
  std::atomic<bool> okay{true};
  std::thread saver([&]{
    for(int i=0;i<500;++i) {
      MemoryStream s;
      if(p.p.getState(&s)!=kResultOk || s.getSize()!=24)
        okay.store(false);
    }
  });
  for(int i=0;i<500;++i) {
    Buffer<64> audio;
    audio.il[0]=.3f;
    audio.ir[0]=.3f;
    audio.run(p);
  }
  saver.join();
  CHECK(okay.load());
}
static void testBypassClock(){
  Plugin a,b;a.flush(a125::kMix,1.);b.flush(a125::kMix,1.);
  Buffer<256> x,y;x.il[0]=x.ir[0]=y.il[0]=y.ir[0]=1.f;
  x.run(a);y.run(b);
  b.flush(a125::kEnable,0.);
  for(int k=0;k<16;++k){Buffer<256> ref,disabled;ref.run(a);disabled.run(b);}
  b.flush(a125::kEnable,1.);
  Buffer<256> ref,restored;ref.run(a);restored.run(b);
  bool tail=false;
  for(int i=0;i<256;++i){
    CHECK(ref.ol[i]==restored.ol[i]&&ref.orr[i]==restored.orr[i]);
    tail|=(ref.ol[i]!=0.f||ref.orr[i]!=0.f);}
  CHECK(tail);
}
int main(){testStateAndText();testAutomation();testNonfinite();testBypassClock();testDryUnityAndConcurrentState();
 std::puts("PASS ACTUAL RetroProcessor: Enable text/state, sample-offset + multi-point automation, NaN isolation, bypass tail-clock, >1.0 dry unity, concurrent state reads");}
