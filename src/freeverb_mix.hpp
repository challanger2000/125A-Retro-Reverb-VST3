#pragma once
// Original Freeverb by Jezar at Dreampoint (June 2000) is used unchanged
// in the sample-processing path. Only the 125A Mix control is adapted.
//
// Original Freeverb setdry(x) multiplies x by scaledry=2.
// Thus a *unity* dry signal at Mix=0 requires setdry(0.5).
// Wet values are passed directly to the original setwet() function.
#include <algorithm>
#include <cmath>
#include "revmodel.hpp"

namespace a125 {
inline float normalizedMix(float mix) noexcept {
    return std::isfinite(mix) ? std::clamp(mix, 0.f, 1.f) : 0.f;
}
inline void applyMix(revmodel& originalFreeverb, float mix) noexcept {
    const float value=normalizedMix(mix);
    originalFreeverb.setwet(value);                  // original wet scaling
    originalFreeverb.setdry((1.f-value)*0.5f);       // correct x2 dry scaling
}
} // namespace a125
