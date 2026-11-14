# Third-party provenance

The eight files in `third_party/freeverb/Components` are derived from unmodified upstream copies from https://github.com/sinshu/freeverb, which republishes Jezar at Dreampoint's original Freeverb source code (June 2000). The source files explicitly state **public domain**. **125A exceptions:** `comb.cpp` additionally resets `filterstore` inside `comb::mute()`; this changes only reset state, not the normal per-sample filter equation. In `revmodel.cpp`, five pre-standard loop-variable uses are localized to their `for` statements so modern MSVC/C++23 compiles, without modifying the audio equations. The `revmodel` constructor also initializes scalar fields before its first `update()`, avoiding reads of uninitialized memory in modern C++. See original [readme](https://github.com/sinshu/freeverb/blob/main/readme.txt).

The Windows VST3 build obtains Steinberg's **VST3 SDK 3.8.1** separately using Git, pinned to `v3.8.1_build_84`. Its MIT license applies to the SDK files. This repository does not bundle SDK files or Vocaid binaries.

The 125A processor wrapper and tests are new source, not extracted from Virtual Vocaid. No samples, ROMs, IRs, or binary data from Virtual Vocaid are shipped.
