# Third-party provenance

The eight files in `third_party/freeverb/Components` are unmodified copies from https://github.com/sinshu/freeverb, which republishes Jezar at Dreampoint's original Freeverb source code (June 2000). The source files explicitly state **public domain**. See original [readme](https://github.com/sinshu/freeverb/blob/main/readme.txt).

The Windows VST3 build obtains Steinberg's **VST3 SDK 3.8.1** separately using Git, pinned to `v3.8.1_build_84`. Its MIT license applies to the SDK files. This repository does not bundle SDK files or Vocaid binaries.

The 125A processor wrapper and tests are new source, not extracted from Virtual Vocaid. No samples, ROMs, IRs, or binary data from Virtual Vocaid are shipped.
