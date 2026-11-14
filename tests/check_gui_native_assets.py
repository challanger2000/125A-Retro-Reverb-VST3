#!/usr/bin/env python3
"""Regression check: P004.1 GUI uses the original 125x125 master pixels.

The 101-frame 1x sprite strip must be pixel-identical to the old Ugly Reverb
master, not bilinear-downsampled to 66px or scaled by VSTGUI at draw time.
No third-party Python modules are required.
"""
import argparse
import importlib.util
import pathlib
import sys
import xml.etree.ElementTree as ET

def check(master,strip,script,layout,gui,processor):
    spec=importlib.util.spec_from_file_location("original_ugly_generator",str(script))
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mw,mh,mraw=mod.read_png(str(master))
    sw,sh,sraw=mod.read_png(str(strip))
    assert sw==125 and sh==125*101,(sw,sh)
    assert (mw,mh) in ((125,125*101),(125*101,125)),(mw,mh)
    for frame in (0,1,17,50,83,100):
        for row in range(125):
            if mw==125:
                a=((frame*125+row)*125)*4
            else:
                a=(row*mw+frame*125)*4
            b=((frame*125+row)*125)*4
            assert mraw[a:a+125*4]==sraw[b:b+125*4],f"master pixel mismatch at frame={frame} row={row}"
    root=ET.parse(layout).getroot()
    desc=root.find(".//template")
    assert desc is not None
    assert desc.get("size")=="930, 410",desc.get("size")
    for knob in ("Size","Damping","Width","Mix"):
        views=[node for node in desc.iter("view") if node.attrib.get("custom-view-name")==knob]
        assert len(views)==1,(knob,len(views))
        assert views[0].attrib.get("size")=="125, 125",(knob,views[0].attrib)
    assert not any(node.attrib.get("title")=="REVERB ENGINE" for node in desc.iter("view"))
    toggles=[node for node in desc.iter("view") if node.attrib.get("custom-view-name")=="Enable"]
    assert len(toggles)==1
    assert not any(v.get("custom-view-name")=="GuiZoom" for v in desc.iter("view"))
    assert not any(v.get("title")=="GUI" for v in desc.iter("view"))
    gui_source=gui.read_text(encoding="utf-8")
    processor_source=processor.read_text(encoding="utf-8")
    assert 'setAllowedZoomFactors({1.})' in gui_source, "editor must only allow 100% zoom"
    assert gui_source.count('setZoomFactor(1.)')>=3, "open and legacy state must force normal size"
    assert 'UglyZoomControl' not in gui_source and 'guiZoomIndex_' not in gui_source
    assert 'guiZoomIndex_' not in processor_source, "obsolete zoom state member present"
    assert 's.writeInt32(kGuiMagic)&&s.writeInt32(0)' in gui_source, "legacy editor state must remain compatible at 100%"
    assert 'magic==kGuiMagic' in gui_source and 'legacyZoom' in gui_source, "old DAW project GUI state must be accepted"
    print("PASS original 125px master frames, 4 normal-size knobs, no Zoom UI, locked 100% and legacy editor state")

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--master",type=pathlib.Path,required=True)
    p.add_argument("--strip",type=pathlib.Path,required=True)
    p.add_argument("--generator",type=pathlib.Path,required=True)
    p.add_argument("--layout",type=pathlib.Path,required=True)
    p.add_argument("--gui",type=pathlib.Path,required=True)
    p.add_argument("--processor",type=pathlib.Path,required=True)
    a=p.parse_args()
    check(a.master,a.strip,a.generator,a.layout,a.gui,a.processor)

if __name__=="__main__":
    main()
