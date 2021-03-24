/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Hilbert.hpp"

struct SequenceLFO
{
    int state = 0;
    
    std::vector<int> sequence = {0, 0, 0, 0};
    
    float sample_rate;
    float phase = 0;
    float frequency = 0;
    float depth = 0;
    
    bool enabled = true;
    
    int sign = 1;

    
    SequenceLFO(const dsp::ProcessSpec& spec) {
        sample_rate = spec.sampleRate;
        
        waveshapes.resize(4);
        
        
        // Sine shape
        waveshapes[0] = [this](float x) {
            return dsp::FastMathApproximations::sin(x * M_PI);
        };
        
        // Square shape
        waveshapes[1] = [this](float x) {
            return x < 0.0f ? -1.0f : 1.0f;
        };
        
        //Triangle shape
        waveshapes[2] = [this](float x) {
            return (abs(x) - 0.5f) * 2.0f;
        };
        // Sawtooth shape
        waveshapes[3] = [this](float x) {
            return x;
        };
    }
    
    void set_voice(int shape) {
        sequence = {shape&1, shape&2, shape&4, shape&8};
    };
    
    void set_frequency(float freq) {
        frequency = freq;
    }
    
    void set_depth(float value) {
        depth = value;
    }
    
    void tick() {
        phase += frequency / sample_rate;
        
        // Change shape when we complete a cycle
        if(phase >= 1.0f) {
            phase -= 2.0f;
            // Find next sequence
            int num_trials = 0;
            enabled = true;
            
            state++;
            state &= 3;
            
            while(sequence[state] == 0 && num_trials < 3) {
                num_trials++;
                state++;
                state &= 3;
            }
            
            if(sequence[state & 3] == 0) {
                state = 0;
                enabled = false;
            }
        }
    }
    
    // To allow stereo lfo-ing
    bool set_inverse(bool inv) {
        sign = inv ? -1 : 1;
    }
    
    float get_state() {
        return phase;
    }
    void set_state(float state) {
        phase = state;
    }
    
    std::vector<std::function<float(float)>> waveshapes;
    
    float get_sample() {
        if(!enabled) return 0;
        return waveshapes[state](phase) * depth * sign;
    }
};
