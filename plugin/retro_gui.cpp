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
// Original UglyToggle handles all interaction and parameter binding. 125A
// paints only a new raised, polished-silver thumb over the existing switch.
// This avoids replacing the GUI framework or touching any DSP code.
class RetroSilverToggle final : public UglyReverb::UglyToggle {
public:
  RetroSilverToggle(const VSTGUI::CRect& rect,VSTGUI::IControlListener* listener,int32_t tag)
    : UglyReverb::UglyToggle(rect,listener,tag) {}
  RetroSilverToggle(const RetroSilverToggle& other)
    : UglyReverb::UglyToggle(other) {}
  VSTGUI::CBaseObject* newCopy() const override {return new RetroSilverToggle(*this);}
  void draw(VSTGUI::CDrawContext* ctx) override {
    UglyReverb::UglyToggle::draw(ctx);
    const auto r=getViewSize();
    const bool enabled=getValueNormalized()>=0.5f;
    VSTGUI::CRect track=r;
    track.inset(3.,3.);
    const double diameter=track.getHeight()-4.0;
    const double cy=track.getCenter().y;
    const double cx=enabled?(track.right-2.0-diameter*0.5)
                           :(track.left+2.0+diameter*0.5);
    // Draw exactly over the formerly gray thumb, at its original position.
    VSTGUI::CRect outer(cx-diameter*.5,cy-diameter*.5,
                        cx+diameter*.5,cy+diameter*.5);
    ctx->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);
    auto metalDisc=[&](VSTGUI::CRect rect,VSTGUI::CColor light,
                       VSTGUI::CColor dark,VSTGUI::CColor border,double width) {
      auto* path=ctx->createRoundRectGraphicsPath(rect,rect.getWidth()*.5);
      if(!path)return;
      if(auto* grad=VSTGUI::CGradient::create(0.,1.,light,dark)){
        ctx->fillLinearGradient(path,*grad,rect.getTopLeft(),rect.getBottomLeft(),false);
        grad->forget();
      }
      ctx->setFrameColor(border);
      ctx->setLineWidth(width);
      ctx->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);
      path->forget();
    };
    // Three concentric rings, like a machined chrome push-switch.
    metalDisc(outer,{255,255,255,255},{111,127,142,255},{249,253,255,255},1.4);
    VSTGUI::CRect bezel=outer;bezel.inset(2.1,2.1);
    metalDisc(bezel,{150,167,181,255},{55,70,84,255},{223,233,242,255},.7);
    VSTGUI::CRect face=outer;face.inset(4.0,4.0);
    metalDisc(face,{255,255,255,255},{179,191,202,255},{247,251,254,255},.7);
    // Bright, asymmetric reflection: the button must read as SILVER,
    // not as a uniform gray UI circle, even in a 100%-zoom screenshot.
    ctx->setFillColor({255,255,255,230});
    ctx->drawEllipse({cx-diameter*.24,cy-diameter*.35,
                      cx+diameter*.13,cy-diameter*.19},VSTGUI::kDrawFilled);
    ctx->setFrameColor({245,250,255,215});
    ctx->setLineWidth(1.2);
    ctx->drawLine({face.left+4.,face.top+4.},
                  {face.left+9.,face.top+2.});
    setDirty(false);
  }
};

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
    panel({0,0,930,88},0,{18,22,28,255},{7,10,15,255},{26,39,52,255});
    panel({20,101,766,395},9,{24,66,108,255},{9,30,55,255},{55,111,167,230});
    panel({782,101,917,395},9,{23,66,105,255},{9,30,53,255},{55,111,167,230});
    c->setFrameColor({62,122,178,200});c->setLineWidth(1);
    c->drawLine({22,88},{908,88});
    c->setFillColor({23,29,37,255});
    for(auto p:{VSTGUI::CPoint{10,10},VSTGUI::CPoint{920,10},
                VSTGUI::CPoint{10,400},VSTGUI::CPoint{920,400}})
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
  if(std::strcmp(name,"Enable")==0)return new RetroSilverToggle(r,editor,kEnable);
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
