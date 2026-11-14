# Reverse engineering history (research only)

Static analysis of the Virtual Vocaid Windows VST3 showed the classic Freeverb comb tuning (1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617) and JUCE 6.1.6 Reverb-style support code. JUCE's sample-rate scaling and parameter smoothing were formerly used as a speculative 125A implementation. **They are not used by the current VST3**, because the chosen product requirement is to port **Jezar's original Freeverb** rather than introduce improvements or a JUCE reimplementation.

The reference JUCE comparison under \`third_party/juce_616\` is retained as evidence but deliberately excluded from the shipped original-Freeverb VST3. Jezar's filter equations and original delay lengths are authoritative. Its historical raw dry scale is 2, which requires \`setdry((1-mix)/2)\` for 125A's zero-mix unity rule. This adapter change is strictly outside the audio filter algorithm.

Actual supplied WAVs provide an independent baseline, but similarity of filter timings is not proof of matching the entire Virtual Vocaid preset output.
