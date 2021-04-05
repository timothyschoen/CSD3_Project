/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct SequenceLFO
{
    int state = 0;
    
    std::vector<int> sequence = {0, 0, 0, 0};
    
    int num_channels;
    float sample_rate;
    float phase = 0;
    
    SmoothedValue<float> synced_frequency;
    
    SmoothedValue<float> frequency;
    SmoothedValue<float> depth;
    
    const float NoteDivisionScalers[7] = { 1.0f / 1.0f * 4.0f,
                                           1.0f / 2.0f * 4.0f,
                                           1.0f / 4.0f * 4.0f,
                                           1.0f / 8.0f * 4.0f,
                                           1.0f / 16.0f * 4.0f,
                                           1.0f / 32.0f * 4.0f,
                                           1.0f / 64.0f * 4.0f };
    
    bool enabled = true;
    bool sync = false;
    bool stereo = false;

    
    SequenceLFO(const dsp::ProcessSpec& spec) {
        sample_rate = spec.sampleRate;
        num_channels = spec.numChannels;
        
        synced_frequency.reset(sample_rate, 0.02f);
        frequency.reset(sample_rate, 0.02f);
        depth.reset(sample_rate, 0.02f);
        
        waveshapes.resize(5);

        // flat shape
        waveshapes[4] = [this](float x) {
            return 0;
        };
        
        // Sine shape
        waveshapes[0] = [this](float x) {
            return dsp::FastMathApproximations::sin((x * 2.0f * M_PI) - M_PI);
        };
        
        // Square shape
        waveshapes[1] = [this](float x) {
            return x < 0.5f ? -1.0f : 1.0f;
        };
        
        //Triangle shape
        waveshapes[2] = [this](float x) {
            // Shift phase to make it start at 0, and smoothly connect to other shapes
            x += 0.25;
            if(x >= 1.0f) x -= 1.0f;
            
            return ((x < 0.5) * (4.0f * x - 1.0f)) + ((x >= 0.5) * (1.0f - 4.0f * (x - 0.5)));
        };
        // Sawtooth shape
        waveshapes[3] = [this](float x) {
            return ((phase * 2.0f) - 1.0f);
        };
    }
    
    void set_voice(int shape) {
        if(shape != 0) enabled = true;
        sequence = {shape&1, shape&2, shape&4, shape&8};
    };
    
    void set_frequency(float freq) {
        frequency.setTargetValue(freq);
    }
    
    void set_depth(float value) {
        depth.setTargetValue(value);
    }
    
    // To allow stereo lfo-ing
    void set_inverse(bool is_stereo) {
        stereo = is_stereo;
    }

    void set_sync(bool should_sync) {
        sync = should_sync;
    }
    
    void set_stereo(bool is_stereo) {
        stereo = is_stereo;
    }
    
    
    std::vector<std::function<float(float)>> waveshapes;
    
    void process(dsp::AudioBlock<float> output) {
        if(!enabled) {
            output.clear();
            return;
        }
        auto* output_ptr = output.getChannelPointer(0);
        for(int n = 0; n < output.getNumSamples(); n++) {
            phase += (sync ? synced_frequency.getNextValue() : frequency.getNextValue() * 2.0f) / sample_rate;
            
            if(phase >= 1.0f) {
                phase -= 1.0f;
                next_shape();
            }
            output_ptr[n] = waveshapes[state](phase) * depth.getNextValue();
        }
        
        if(num_channels > 1) {
            output.getSingleChannelBlock(1).copyFrom(output.getSingleChannelBlock(0));
            if(stereo) output.getSingleChannelBlock(1) *= -1.0f;
        }
        
    }
    
    void next_shape() {
        // Find next shape in sequence
        enabled = true;
        
        state++;
        state &= 3;
        
        for(int i = 0; i < sequence.size(); i++) {
            if(sequence[state] != 0) break;
            state++;
            state &= 3;
        }

        if(sequence[state & 3] == 0) {
            state = 4;
            enabled = false;
        }
    }
        
    void sync_with_playhead(AudioPlayHead* playhead) {
        AudioPlayHead::CurrentPositionInfo position_info;
        
        if(sync) {
            int noteDivision = frequency.getTargetValue();
            
            playhead->getCurrentPosition(position_info);
            
            float tempo_freq = position_info.bpm > 0.0f ? 60.0f / position_info.bpm : 0.0f;
            
            float scaler = NoteDivisionScalers[6 - noteDivision];

            synced_frequency.setTargetValue(tempo_freq * scaler);
            
            // sync phase
            float p = position_info.ppqPosition * scaler;
            phase = p - floor(p);
        }
    }
    
};
