#pragma once

// 125A – independent JUCE 6.1.6/Freeverb-family reference model.
// The algorithm, tuning and 10 ms smoothing below follow juce::Reverb
// (modules/juce_audio_basics/utilities/juce_Reverb.h, JUCE 6.1.6).
// Copyright (c) 2020 Raw Material Software Limited.
// Used with acknowledgement of the ISC license:
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES.
//
// The implementation below was rewritten independently, has no JUCE dependency
// and contains no proprietary Virtual Vocaid data.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace a125 {
class VocaidReferenceReverb final {
public:
    struct Controls {
        float size = .5f;
        float damping = .5f;
        float width = 1.f;
        float mix = .33f;
        bool enabled = true;
    };

    // Must run OFF the realtime audio thread; allocates the delay lines.
    bool prepare(double sampleRate) {
        if (!std::isfinite(sampleRate) || sampleRate < 1000.0 || sampleRate > 2000000.0)
            return false;
        sampleRate_ = sampleRate;
        static constexpr int combTunings[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static constexpr int allpassTunings[4] = { 556, 441, 341, 225 };
        const int sr = static_cast<int>(sampleRate);
        for (int ch=0; ch<2; ++ch) {
            for (int i=0; i<8; ++i)
                combs_[ch][i].prepare(std::max(1, static_cast<int>((static_cast<std::int64_t>(sr) * (combTunings[i] + 23 * ch)) / 44100)));
            for (int i=0; i<4; ++i)
                allpasses_[ch][i].prepare(std::max(1, static_cast<int>((static_cast<std::int64_t>(sr) * (allpassTunings[i] + 23 * ch)) / 44100)));
        }
        // JUCE Reverb constructor sets its default parameter targets before
        // setSampleRate(), which snaps every SmoothedValue to its current target.
        updateTargets();
        const int smoothingSamples = static_cast<int>(std::floor(sampleRate * .01)); // JUCE 6.1.6
        damping_.reset(smoothingSamples);
        feedback_.reset(smoothingSamples);
        dry_.reset(smoothingSamples);
        wetSame_.reset(smoothingSamples);
        wetCross_.reset(smoothingSamples);
        reset();
        ready_ = true;
        return true;
    }

    void setControls(Controls controls) noexcept {
        controls.size = sanitize(controls.size);
        controls.damping = sanitize(controls.damping);
        controls.width = sanitize(controls.width);
        controls.mix = sanitize(controls.mix);
        const bool zeroMix = controls.mix == 0.f;
        // Zero is a hard musical boundary: 0% = dry unity, no residual wet
        // or 10 ms smoothing tail. Clear stale reverb when entering zero.
        if (controls_.enabled != controls.enabled || (zeroMix && controls_.mix != 0.f))
            reset();
        controls_ = controls;
        updateTargets();
        if (zeroMix) {
            dry_.snap(1.f);
            wetSame_.snap(0.f);
            wetCross_.snap(0.f);
        }
    }

    [[nodiscard]] Controls controls() const noexcept { return controls_; }
    [[nodiscard]] bool isReady() const noexcept { return ready_; }
    [[nodiscard]] double sampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] std::size_t firstCombDelay(int channel) const noexcept {
        return combs_[std::clamp(channel,0,1)][0].delay.size();
    }

    void reset() noexcept {
        for (auto& channel: combs_)
            for (auto& f:channel) f.clear();
        for (auto& channel: allpasses_)
            for (auto& f:channel) f.clear();
        // JUCE Reverb::reset does NOT jump smoothing parameters to targets.
    }

    // Supports both in-place and non-overlapping input/output audio buffers.
    // All processing below is allocation-free.
    void processStereo(const float* inL, const float* inR, float* outL, float* outR, std::size_t n) noexcept {
        if(!ready_ || !inL || !inR || !outL || !outR) return;
        if(!controls_.enabled || controls_.mix == 0.f) {
            for(std::size_t i=0;i<n;++i) {const float l=inL[i],r=inR[i];outL[i]=l;outR[i]=r;}
            return;
        }
        for (std::size_t i=0;i<n;++i) {
            const float l=inL[i],r=inR[i];
            const auto gains = nextGains();
            const float excite=(l+r)*.015f;
            const float wetL=filter(0,excite,gains.damp,gains.feedback);
            const float wetR=filter(1,excite,gains.damp,gains.feedback);
            outL[i]=wetL*gains.wet1 + wetR*gains.wet2 + l*gains.dry;
            outR[i]=wetR*gains.wet1 + wetL*gains.wet2 + r*gains.dry;
        }
    }

    // Speaker layout 1→2 for mono microphone tracks, preserving stereo diffusion.
    void processMonoToStereo(const float* input, float* outL, float* outR, std::size_t n) noexcept {
        if(!ready_ || !input || !outL || !outR) return;
        if(!controls_.enabled || controls_.mix == 0.f) {
            for(std::size_t i=0;i<n;++i){const float x=input[i];outL[i]=x;outR[i]=x;}
            return;
        }
        for(std::size_t i=0;i<n;++i) {
            const float x=input[i]; // read before writing, safe for in-place outL aliasing
            const auto gains=nextGains();
            const float excite=2.f*x*.015f; // JUCE stereo path driven by duplicated mono input
            const float wetL=filter(0,excite,gains.damp,gains.feedback);
            const float wetR=filter(1,excite,gains.damp,gains.feedback);
            outL[i]=wetL*gains.wet1+wetR*gains.wet2+x*gains.dry;
            outR[i]=wetR*gains.wet1+wetL*gains.wet2+x*gains.dry;
        }
    }

    // JUCE processMono equivalent: the left filter bank only.
    void processMono(const float* input, float* output, std::size_t n) noexcept {
        if(!ready_ || !input || !output) return;
        if(!controls_.enabled || controls_.mix == 0.f) {
            for(std::size_t i=0;i<n;++i)output[i]=input[i];
            return;
        }
        for(std::size_t i=0;i<n;++i) {
            const float x=input[i];
            const auto gains=nextGains();
            const float wet=filter(0,x*.015f,gains.damp,gains.feedback);
            output[i]=wet*gains.wet1 + x*gains.dry;
        }
    }

private:
    static float sanitize(float x) noexcept {return std::isfinite(x)?std::clamp(x,0.f,1.f):0.f;}
    static float undenormalise(float v) noexcept {return std::abs(v)<1.e-30f?0.f:v;}
    struct Smooth {
        float current=0.f,target=0.f,step=0.f;
        int steps=0,remaining=0;
        void reset(int n) noexcept {steps=n;current=target;remaining=0;step=0.f;}
        void snap(float x) noexcept {current=target=x;remaining=0;step=0.f;}
        void set(float x) noexcept {
            if(x==target) return;
            target=x;
            if(steps<=0){current=target;remaining=0;return;}
            remaining=steps;step=(target-current)/static_cast<float>(steps);
        }
        float next() noexcept {
            if(remaining>0) {
                --remaining;
                if(remaining>0) current+=step;
                else current=target;
            }
            return current;
        }
    };
    struct Comb {
        std::vector<float> delay;
        std::size_t index=0;
        float last=0.f;
        void prepare(int size){delay.assign(static_cast<std::size_t>(size),0.f);index=0;last=0.f;}
        void clear() noexcept{std::fill(delay.begin(),delay.end(),0.f);last=0.f;}
        float process(float input,float damp,float feedback) noexcept {
            const float output=delay[index];
            last=undenormalise(output*(1.f-damp)+last*damp);
            delay[index]=undenormalise(input+last*feedback);
            if(++index==delay.size())index=0;
            return output;
        }
    };
    struct Allpass {
        std::vector<float> delay;
        std::size_t index=0;
        void prepare(int size){delay.assign(static_cast<std::size_t>(size),0.f);index=0;}
        void clear() noexcept{std::fill(delay.begin(),delay.end(),0.f);}
        float process(float input) noexcept {
            const float buffered=delay[index];
            delay[index]=undenormalise(input+buffered*.5f);
            if(++index==delay.size())index=0;
            return buffered-input;
        }
    };
    struct Gains {float damp,feedback,dry,wet1,wet2;};
    Gains nextGains() noexcept {
        // Same smoother update order as JUCE 6.1.6: damp, feedback, dry, wet1, wet2.
        return {damping_.next(),feedback_.next(),dry_.next(),wetSame_.next(),wetCross_.next()};
    }
    float filter(int ch,float x,float damp,float feedback) noexcept {
        float sum=0.f;
        for(auto& comb:combs_[ch]) sum+=comb.process(x,damp,feedback);
        for(auto& allpass:allpasses_[ch])sum=allpass.process(sum);
        return sum;
    }
    void updateTargets() noexcept {
        const float wet=controls_.mix*3.f; // JUCE wetScaleFactor
        // The JUCE internal dry x2 is a historical gain law, NOT ours:
        // 125A 0% Mix must equal an unmodified 0 dB dry signal.
        dry_.set(1.f-controls_.mix);
        wetSame_.set(.5f*wet*(1.f+controls_.width));
        wetCross_.set(.5f*wet*(1.f-controls_.width));
        damping_.set(controls_.damping*.4f);
        feedback_.set(controls_.size*.28f+.7f);
    }

    Controls controls_{};
    std::array<std::array<Comb,8>,2> combs_{};
    std::array<std::array<Allpass,4>,2> allpasses_{};
    Smooth damping_{}, feedback_{}, dry_{}, wetSame_{}, wetCross_{};
    double sampleRate_=44100.0;
    bool ready_=false;
};
} // namespace a125
