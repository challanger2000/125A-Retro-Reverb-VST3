#!/usr/bin/env python3
"""Controlled Virtual Vocaid dry/wet comparison, with no loudness normalization.

Required inputs: two identical MIDI/instrument renders of Virtual Vocaid V1.0:
  dry.wav  : Reverb ON, Mix=0%, no other effects; all other settings identical.
  wet.wav  : Reverb ON, Mix=100%, no other effects; same MIDI and export range.
The user's actual Virtual Vocaid captures show unity dry at Mix=0,
so the dry capture is used directly as the candidate input. We do not
apply any gain normalization. Both WAV exports must be linear PCM.
"""
import argparse
import json
import math
import pathlib
import struct
import subprocess
import tempfile
import wave


def load_pcm(path):
    with wave.open(str(path), "rb") as w:
        channels, sr, width, nframes = w.getnchannels(), w.getframerate(), w.getsampwidth(), w.getnframes()
        if channels not in (1, 2) or width not in (2, 3, 4):
            raise ValueError(f"{path}: require 16/24/32-bit PCM mono or stereo WAV")
        if w.getcomptype() != "NONE":
            raise ValueError("WAV must be uncompressed PCM")
        buf = w.readframes(nframes)
    if width == 2:
        floats = [x / 32768.0 for x in struct.unpack("<" + "h"*(len(buf)//2), buf)]
    elif width == 4:
        floats = [x / 2147483648.0 for x in struct.unpack("<" + "i"*(len(buf)//4), buf)]
    else:
        floats = []
        for pos in range(0, len(buf), 3):
            val = int.from_bytes(buf[pos:pos+3], "little", signed=False)
            if val & 0x800000: val -= 0x1000000
            floats.append(val / 8388608.0)
    if channels == 1:
        floats = [y for v in floats for y in (v, v)]
    if not all(math.isfinite(x) for x in floats):
        raise ValueError("nonfinite PCM sample")
    return sr, floats


def save_wav(path, rate, samples):
    clipped = sum(abs(x)>1.0 for x in samples)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(2); w.setframerate(rate); w.setsampwidth(3)
        raw=bytearray()
        for value in samples:
            value = max(-1.0, min((8388607.0/8388608.0), value))
            num = round(value * 8388608.0)
            raw += int(num).to_bytes(3, "little", signed=True)
        w.writeframes(raw)
    return clipped


def render(renderer, rate, size, damping, width, mix, src):
    with tempfile.TemporaryDirectory() as tmp:
        inp=pathlib.Path(tmp)/"input.f32";out=pathlib.Path(tmp)/"output.f32"
        inp.write_bytes(struct.pack("<" + "f"*len(src), *src))
        result=subprocess.run([str(renderer), str(rate), str(size), str(damping),
             str(width), str(mix), str(inp), str(out)],check=True,text=True,capture_output=True)
        data=out.read_bytes()
        if len(data)!=len(src)*4:
            raise RuntimeError("renderer returned unexpected sample count")
        samples=list(struct.unpack("<" + "f"*(len(data)//4),data))
    return samples


def metrics(reference,candidate):
    if len(reference)!=len(candidate):
        raise ValueError("captures have different lengths; re-export with identical song range")
    n=len(reference)
    if n==0: raise ValueError("empty capture")
    sum_ref=sum(x*x for x in reference);sum_cand=sum(x*x for x in candidate)
    err=sum((a-b)*(a-b) for a,b in zip(reference,candidate))
    dot=sum(a*b for a,b in zip(reference,candidate))
    correlation=dot/math.sqrt(sum_ref*sum_cand) if sum_ref*sum_cand>0 else 0.0
    return {
        "frames":n//2,
        "reference_rms":math.sqrt(sum_ref/n),
        "candidate_rms":math.sqrt(sum_cand/n),
        "error_rms":math.sqrt(err/n),
        "relative_error_percent":100.0*math.sqrt(err/max(sum_ref,1.e-30)),
        "correlation":correlation,
        "candidate_over_reference_dB":10.0*math.log10(max(sum_cand,1.e-30)/max(sum_ref,1.e-30)),
        "latency_alignment_samples":0,
        "gain_normalization_applied":False,
    }


def run(dry,wet,renderer,size,damping,width,output):
    sr,dry_samples=load_pcm(dry)
    sr2,wet_samples=load_pcm(wet)
    if sr!=sr2:raise ValueError("dry and wet sample rates differ")
    if len(dry_samples)!=len(wet_samples):raise ValueError("dry and wet export lengths differ")
    # Grounded in the uploaded OFF/default/100%-wet captures:
    # default = OFF + 0.5 * wet100 to floating-point precision.
    src=dry_samples
    candidate=render(renderer,sr,size,damping,width,1.0,src)
    result=metrics(wet_samples,candidate)
    result.update({"sample_rate":sr,"size":size,"damping":damping,"width":width,
         "reference":"Virtual Vocaid wet=100% external WAV",
         "candidate":"125A original Jezar Freeverb from unmodified dry source",
         "assumptions":"same source samples, disabled other FX, identical timing",
         "claim":"numeric comparison only, not a proof of identical underlying plugin"})
    if output:
        result["candidate_output_wav"]=str(output)
        result["candidate_wav_clipped_samples"]=save_wav(output,sr,candidate)
    return result


def selftest(renderer):
    with tempfile.TemporaryDirectory() as tmp:
        a=pathlib.Path(tmp)
        sr=44100;n=sr
        src=[0.0]*(n*2)
        for i in (300,10000,19920):
            src[2*i]=.12;src[2*i+1]=-.07
        dry=list(src)
        wet=render(renderer,sr,.5,.5,1.0,1.0,src)
        save_wav(a/"dry.wav",sr,dry)
        save_wav(a/"wet.wav",sr,wet)
        result=run(a/"dry.wav",a/"wet.wav",renderer,.5,.5,1.0,None)
        if result["relative_error_percent"] > .01:
            raise AssertionError(f"selftest audio pipeline error: {result}")
        print(f"PASS A/B WAV pipeline, roundtrip PCM24 error={result['relative_error_percent']:.5f}%")
        
def main():
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--dry",type=pathlib.Path)
    p.add_argument("--wet",type=pathlib.Path)
    p.add_argument("--renderer",type=pathlib.Path,required=True)
    p.add_argument("--size",type=float,default=.5)
    p.add_argument("--damping",type=float,default=.5)
    p.add_argument("--width",type=float,default=1.0)
    p.add_argument("--out-wav",type=pathlib.Path)
    p.add_argument("--selftest",action="store_true")
    args=p.parse_args()
    if args.selftest:
        selftest(args.renderer);return
    if not args.dry or not args.wet:
        p.error("--dry and --wet are required without --selftest")
    for name in ("size","damping","width"):
        v=getattr(args,name)
        if not math.isfinite(v) or v<0 or v>1:p.error(f"{name} must be 0..1")
    print(json.dumps(run(args.dry,args.wet,args.renderer,args.size,args.damping,
         args.width,args.out_wav),indent=2))
if __name__=="__main__":
    main()
