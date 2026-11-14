#include "retro_processor.hpp"
#include "UglyControls.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstring.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstring>
namespace a125 {
using namespace Steinberg;
namespace {
class RetroFaceplate final : public VSTGUI::CView {
public:
  explicit RetroFaceplate(const VSTGUI::CRect& r):CView(r){setMouseEnabled(false);}
  RetroFaceplate(const RetroFaceplate& o):CView(o){}
  VSTGUI::CBaseObject* newCopy() const override{return new RetroFaceplate(*this);}
  void draw(VSTGUI::CDrawContext* c) override {
    const auto r=getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);
    c->setFillColor({5,7,10,255});c->drawRect(r,VSTGUI::kDrawFilled);
    auto panel=[&](VSTGUI::CRect bounds,double radius,
                   VSTGUI::CColor top,VSTGUI::CColor bottom,VSTGUI::CColor edge){
      auto* path=c->createRoundRectGraphicsPath(bounds,radius);if(!path)return;
      if(auto* gradient=VSTGUI::CGradient::create(0.,1.,top,bottom)){
        c->fillLinearGradient(path,*gradient,bounds.getTopLeft(),bounds.getBottomLeft(),false);
        gradient->forget();
      }
      c->setFrameColor(edge);c->setLineWidth(1);
      c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);path->forget();
    };
    panel({0,0,650,68},0,{18,22,28,255},{7,10,15,255},{26,39,52,255});
    panel({15,82,522,282},9,{24,66,108,255},{9,30,55,255},{55,111,167,230});
    panel({536,82,634,282},9,{23,66,105,255},{9,30,53,255},{55,111,167,230});
    c->setFrameColor({62,122,178,200});c->setLineWidth(1);
    c->drawLine({18,68},{632,68});
    c->setFillColor({23,29,37,255});
    for(auto p:{VSTGUI::CPoint{9,9},VSTGUI::CPoint{641,9},
                VSTGUI::CPoint{9,289},VSTGUI::CPoint{641,289}})
      c->drawEllipse({p.x-3,p.y-3,p.x+3,p.y+3},VSTGUI::kDrawFilled);
    setDirty(false);
  }
};
constexpr double kZoom[]={1.,1.25,1.5,1.75,2.};
constexpr int32 kGuiMagic=0x315A5252;
}
Steinberg::IPlugView* PLUGIN_API RetroProcessor::createView(const char* name) {
  if(name && ConstString(name)==Steinberg::Vst::ViewType::kEditor) {
    auto* e=new VSTGUI::VST3Editor(this,"view","retro_reverb.uidesc");
    e->setAllowedZoomFactors({1.,1.25,1.5,1.75,2.});
    e->setZoomFactor(kZoom[std::clamp(guiZoomIndex_,0,4)]);
    return e;
  }
  return nullptr;
}
void RetroProcessor::didOpen(VSTGUI::VST3Editor* editor) {
  openEditor_=editor;
  if(editor)editor->setZoomFactor(kZoom[std::clamp(guiZoomIndex_,0,4)]);
}
void RetroProcessor::willClose(VSTGUI::VST3Editor* editor) {
  if(openEditor_==editor)openEditor_=nullptr;
}
VSTGUI::CView* RetroProcessor::createCustomView(VSTGUI::UTF8StringPtr name,
   const VSTGUI::UIAttributes& attrs,const VSTGUI::IUIDescription* desc,
   VSTGUI::VST3Editor* editor) {
  if(!name||!desc||!editor)return nullptr;
  VSTGUI::CPoint p{0,0},s{66,66};
  attrs.getPointAttribute("origin",p);attrs.getPointAttribute("size",s);
  VSTGUI::CRect r(p.x,p.y,p.x+s.x,p.y+s.y);
  if(std::strcmp(name,"RetroFaceplate")==0)return new RetroFaceplate(r);
  if(std::strcmp(name,"BrandLogo")==0)return new UglyReverb::UglyLogo(r);
  if(std::strcmp(name,"GuiZoom")==0)return new UglyReverb::UglyZoomControl(r,editor,&guiZoomIndex_);
  if(std::strcmp(name,"Size")==0)return new UglyReverb::UglyKnob(r,editor,kSize);
  if(std::strcmp(name,"Damping")==0)return new UglyReverb::UglyKnob(r,editor,kDamping);
  if(std::strcmp(name,"Width")==0)return new UglyReverb::UglyKnob(r,editor,kWidth);
  if(std::strcmp(name,"Mix")==0)return new UglyReverb::UglyKnob(r,editor,kMix);
  if(std::strcmp(name,"Enable")==0)return new UglyReverb::UglyToggle(r,editor,kEnable);
  return nullptr;
}
tresult PLUGIN_API RetroProcessor::getEditorState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  return s.writeInt32(kGuiMagic)&&s.writeInt32(guiZoomIndex_)?kResultOk:kResultFalse;
}
tresult PLUGIN_API RetroProcessor::setEditorState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);int32 magic=0,index=0;
  if(!s.readInt32(magic)||magic!=kGuiMagic||!s.readInt32(index)){
    guiZoomIndex_=0;return kResultOk;
  }
  guiZoomIndex_=std::clamp(static_cast<int>(index),0,4);
  if(openEditor_)openEditor_->setZoomFactor(kZoom[guiZoomIndex_]);
  return kResultOk;
}
} // namespace a125
