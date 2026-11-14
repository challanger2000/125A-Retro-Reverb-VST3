#pragma once
// Test-only minimal JUCE utilities for compiling the untouched ISC-licensed
// JUCE 6.1.6 Reverb header in a standalone C++23 conformance test.
// No JUCE utility is used in the production VST3 or DSP library.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <vector>

#define jassert(c) do { if(!(c)) std::abort(); } while(false)
#define JUCE_BEGIN_IGNORE_WARNINGS_MSVC(...)
#define JUCE_END_IGNORE_WARNINGS_MSVC
#define JUCE_DECLARE_NON_COPYABLE(...)
#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(...)
#define JUCE_UNDENORMALISE(v) do { if(std::fpclassify(v)==FP_SUBNORMAL) (v)=0.0f; } while(false)
namespace juce {
template<typename T> class HeapBlock {
  std::vector<T> storage_;
public:
  void malloc(std::size_t count) {storage_.resize(count);}
  void clear(std::size_t count) {
    jassert(count<=storage_.size());
    std::fill(storage_.begin(),storage_.begin()+static_cast<std::ptrdiff_t>(count),T{});
  }
  T& operator[](std::size_t i) noexcept{return storage_[i];}
  const T& operator[](std::size_t i) const noexcept{return storage_[i];}
};
template<typename T> class SmoothedValue {
  T current_={},target_={},step_={};
  int stepsToTarget_=0,countdown_=0;
public:
  void setCurrentAndTargetValue(T x) noexcept{current_=target_=x;step_=T{};countdown_=0;}
  void reset(double sr,double seconds) noexcept {
    stepsToTarget_=static_cast<int>(std::floor(sr*seconds));
    setCurrentAndTargetValue(target_);
  }
  void setTargetValue(T x) noexcept {
    if(x==target_)return;
    target_=x;
    if(stepsToTarget_<=0){setCurrentAndTargetValue(x);return;}
    countdown_=stepsToTarget_;
    step_=(target_-current_)/static_cast<T>(stepsToTarget_);
  }
  T getNextValue() noexcept {
    if(countdown_<=0)return target_;
    --countdown_;
    if(countdown_>0)current_+=step_;
    else current_=target_;
    return current_;
  }
};
} // namespace juce
