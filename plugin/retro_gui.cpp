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
// Self-contained 125A polished-silver rocker. Uses VSTGUI only as the
// underlying plugin control/host event adapter; all artwork is drawn here.
// Original Ugly Reverb knobs, logo and zoom remain untouched.
class RetroSilverToggle final : public VSTGUI::COnOffButton {
public:
  RetroSilverToggle(const VSTGUI::CRect& rect,VSTGUI::IControlListener* listener,int32_t tag)
      : VSTGUI::COnOffButton(rect,listener,tag,nullptr) {
    setTransparency(true);
    setWantsFocus(true);
  }
  RetroSilverToggle(const RetroSilverToggle& other):VSTGUI::COnOffButton(other) {}
  VSTGUI::CBaseObject* newCopy() const override{return new RetroSilverToggle(*this);}
  void draw(VSTGUI::CDrawContext* c) override {
    const auto r=getViewSize();
    const bool enabled=getValueNormalized()>=0.5;
    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);
    const auto gradientRound=[&](VSTGUI::CRect rect,double radius,
                                 VSTGUI::CColor top,VSTGUI::CColor bottom,
                                 VSTGUI::CColor frame,double width) {
      auto* path=c->createRoundRectGraphicsPath(rect,radius);
      if(!path)return;
      if(auto* gradient=VSTGUI::CGradient::create(0.,1.,top,bottom)){
        c->fillLinearGradient(path,*gradient,rect.getTopLeft(),rect.getBottomLeft(),false);
        gradient->forget();
      }
      c->setFrameColor(frame);
      c->setLineWidth(width);
      c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);
      path->forget();
    };
    VSTGUI::CRect shadow=r;
    shadow.offset(1.5,2.0);
    gradientRound(shadow,shadow.getHeight()*.5,
                  {0,0,0,140},{0,0,0,20},{0,0,0,0},0.);
    gradientRound(r,r.getHeight()*.5,
                  {249,252,255,255},{87,102,118,255},{240,247,252,255},1.5);
    VSTGUI::CRect track=r;
    track.inset(3.5,3.5);
    gradientRound(track,track.getHeight()*.5,
                  enabled?VSTGUI::CColor{29,47,65,255}:VSTGUI::CColor{16,22,29,255},
                  enabled?VSTGUI::CColor{12,20,30,255}:VSTGUI::CColor{32,40,50,255},
                  {113,136,157,255},1.0);

    // A larger 44px chrome globe is deliberately much brighter than the
    // former matte-grey factory thumb; no photographic skin or resizing.
    const double d=track.getHeight()-4.;
    const double cy=track.getCenter().y;
    const double cx=enabled?(track.right-2.-d*.5):(track.left+2.+d*.5);
    const VSTGUI::CRect ring(cx-d*.5,cy-d*.5,cx+d*.5,cy+d*.5);
    gradientRound(ring,d*.5,
                  {255,255,255,255},{102,118,135,255},{255,255,255,255},1.45);
    VSTGUI::CRect steel=ring;
    steel.inset(2.2,2.2);
    gradientRound(steel,steel.getWidth()*.5,
                  {152,172,189,255},{47,63,79,255},{215,229,241,255},.8);
    VSTGUI::CRect silver=ring;
    silver.inset(4.2,4.2);
    gradientRound(silver,silver.getWidth()*.5,
                  {255,255,255,255},{174,190,203,255},{252,254,255,255},.9);
    // A distinct white light reflection makes SILVER unmistakable at 100%.
    c->setFillColor({255,255,255,250});
    c->drawEllipse({cx-d*.22,cy-d*.34,cx+d*.17,cy-d*.17},
                   VSTGUI::kDrawFilled);
    c->setFont(VSTGUI::kNormalFont,9.5,VSTGUI::kBoldFace);
    c->setFontColor(enabled?VSTGUI::CColor{179,224,252,255}
                           :VSTGUI::CColor{178,192,203,255});
    VSTGUI::CRect text=track;
    if(enabled)text.right=ring.left-1.;
    else text.left=ring.right+1.;
    c->drawString(VSTGUI::UTF8String(enabled?"ON":"OFF"),text,VSTGUI::kCenterText);
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
