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

def check_fixed_scale_typography(root):
    """Independent GUI layout contract for a fixed 100%-scale editor."""
    fonts = {f.get("name"): f for f in root.findall("./fonts/font")}
    contract = {
        "RetroControlLabel": (20, "true"),
        "RetroValue": (18, "true"),
        "RetroSecondaryLabel": (16, "true"),
        "RetroSwitchState": (15, "true"),
    }
    for name, (size, weight) in contract.items():
        f = fonts.get(name)
        assert f is not None, f"Missing named font: {name}"
        assert (f.get("size"), f.get("bold"), f.get("font-name")) == (
            str(size), weight, "Arial"), f"Incorrect font contract: {name}"
    desc = root.find("./template")
    assert desc is not None and desc.get("size") == "930, 410"
    views = list(desc.findall("view"))
    for name, center in (("Size", 118.5), ("Damping", 300.5),
                         ("Width", 482.5), ("Mix", 664.5)):
        labels = [v for v in views if v.get("class") == "CTextLabel"
                  and v.get("title") == name]
        edits = [v for v in views if v.get("class") == "CTextEdit"
                 and v.get("control-tag") == name]
        knobs = [v for v in views if v.get("custom-view-name") == name]
        assert len(labels) == len(edits) == len(knobs) == 1, name
        label, edit, knob = labels[0], edits[0], knobs[0]
        assert label.get("font") == "RetroControlLabel", name
        assert edit.get("font") == "RetroValue", name
        x, y = [int(n.strip()) for n in label.get("origin").split(",")]
        w, h = [int(n.strip()) for n in label.get("size").split(",")]
        assert (y, w, h) == (143, 125, 24), name
        assert x + w / 2 == center, f"label no longer centered: {name}"
        kx, ky = [int(n.strip()) for n in knob.get("origin").split(",")]
        assert (kx + 62.5, ky) == (center, 176), name
        ex, ey = [int(n.strip()) for n in edit.get("origin").split(",")]
        ew, eh = [int(n.strip()) for n in edit.get("size").split(",")]
        assert (ex + ew / 2, ey, eh) == (center, 320, 30), name
    for title, font in (("Enable", "RetroControlLabel"),
                        ("MASTER", "RetroSecondaryLabel"),
                        ("FREEVERB", "RetroSecondaryLabel"),
                        ("ON", "RetroSwitchState"), ("OFF", "RetroSwitchState")):
        labels = [v for v in views if v.get("class") == "CTextLabel"
                  and v.get("title") == title]
        assert len(labels) == 1 and labels[0].get("font") == font, title
    tags = {x.get("name"): x.get("tag") for x in root.findall("./control-tags/control-tag")}
    assert tags == {"Enable": "100", "Size": "101", "Damping": "102",
                    "Width": "103", "Mix": "104"}, "GUI parameter IDs changed"


def check_typography_negative_controls(root):
    """The layout oracle must reject the original tiny labels and values."""
    import copy
    for target, attr, bad in (
        ("Size", "font", "~ NormalFontSmall"),
        ("Damping", "font", "~ NormalFontSmall"),
        ("Width", "font", "~ NormalFontSmall"),
        ("Mix", "font", "~ NormalFontSmall"),
        ("Enable", "font", "~ NormalFontSmall"),
    ):
        mutated = copy.deepcopy(root)
        label = next(v for v in mutated.findall("./template/view")
                     if v.get("class") == "CTextLabel" and v.get("title") == target)
        label.set(attr, bad)
        try:
            check_fixed_scale_typography(mutated)
        except AssertionError:
            pass
        else:
            raise AssertionError(f"Typography negative control survived: {target}")
    mutated = copy.deepcopy(root)
    value = next(v for v in mutated.findall("./template/view")
                 if v.get("class") == "CTextEdit" and v.get("control-tag") == "Size")
    value.set("font", "~ NormalFontSmall")
    try:
        check_fixed_scale_typography(mutated)
    except AssertionError:
        pass
    else:
        raise AssertionError("Numeric-value negative control survived")
    mutated = copy.deepcopy(root)
    font = next(v for v in mutated.findall("./fonts/font") if v.get("name") == "RetroControlLabel")
    font.set("size", "11")
    try:
        check_fixed_scale_typography(mutated)
    except AssertionError:
        pass
    else:
        raise AssertionError("Tiny font-size negative control survived")


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
    check_fixed_scale_typography(root)
    check_typography_negative_controls(root)
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
    print("PASS original knobs, fixed 930x410 editor, large 20/18px typography with killed negative controls, and legacy editor state")

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
