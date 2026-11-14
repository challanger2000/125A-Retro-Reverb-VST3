#!/usr/bin/env python3
"""Guard the two-frame, transparent public-domain metal lever asset.

Top frame: OFF/down (parameter value 0).
Bottom frame: ON/up (parameter value 1).
COnOffButton uses the same 125x125 native pixel dimensions as the frame.
No Pillow or non-stdlib Python dependency in Windows Actions.
"""
import argparse
import hashlib
import pathlib
import struct
import xml.etree.ElementTree as ET

SHA256 = "08f0875ef2d80010e21d281f9d2f9520885a6b3ce98bb840d902af468384b517"

def check(sprite: pathlib.Path, layout: pathlib.Path, gui: pathlib.Path, cmake: pathlib.Path) -> None:
    raw = sprite.read_bytes()
    assert raw[:8] == b"\x89PNG\r\n\x1a\n", "not a PNG"
    assert struct.unpack(">I", raw[8:12])[0] == 13 and raw[12:16] == b"IHDR"
    width, height, depth, color_type = struct.unpack(">IIBB", raw[16:26])
    assert (width, height) == (125, 250), (width, height)
    assert depth == 8 and color_type == 3, "expected 8-bit indexed image"
    assert b"tRNS" in raw, "transparent background missing"
    assert hashlib.sha256(raw).hexdigest() == SHA256, "Invalid/corrupt sprite or changed OFF/ON frame order"
    root = ET.parse(layout).getroot()
    controls = [v for v in root.iter("view") if v.get("custom-view-name") == "Enable"]
    assert len(controls) == 1 and controls[0].get("size") == "125, 125", controls
    assert controls[0].get("origin") == "787, 201", controls[0].attrib
    labels = {v.get("title"): v for v in root.iter("view") if v.get("class") == "CTextLabel"}
    assert "ON" in labels and "OFF" in labels, "physical switch ON/OFF labels missing"
    assert labels["ON"].get("origin") == "819, 195" and labels["OFF"].get("origin") == "819, 324"
    assert labels["ON"].get("mouse-enabled") == "false" and labels["OFF"].get("mouse-enabled") == "false"
    assert labels["FREEVERB"].get("origin") == "789, 358", "bottom label would collide with OFF"
    code = gui.read_text(encoding="utf-8")
    assert 'VSTGUI::COnOffButton(r,editor,kEnable,bitmap)' in code
    assert 'CResourceDescription("retro_lever_off_on_125.png")' in code
    assert "RetroSilverToggle" not in code
    build = cmake.read_text(encoding="utf-8")
    assert "resource/retro_lever_off_on_125.png" in build
    print("PASS: exact, valid 125x250 alpha OFF/ON sprite, native VSTGUI click control, ON-top/OFF-bottom labels")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    for key in ("sprite", "layout", "gui", "cmake"):
        p.add_argument("--" + key, type=pathlib.Path, required=True)
    a = p.parse_args()
    check(a.sprite, a.layout, a.gui, a.cmake)
