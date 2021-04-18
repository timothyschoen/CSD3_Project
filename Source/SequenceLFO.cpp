/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "SequenceLFO.hpp"


SequenceLFO::SequenceLFO(const ProcessSpec& spec) {
    sample_rate = spec.sampleRate;
    num_channels = spec.numChannels;
    
    synced_frequency.reset(sample_rate, 0.02f);
    frequency.reset(sample_rate, 0.02f);
    depth.reset(sample_rate, 0.02f);
}



void SequenceLFO::process(AudioBlock<float> output) {
    if(!shape) {
        output.clear();
        return;
    }
    
    float num_selected = count_bits(shape);
    
    auto* output_ptr = output.getChannelPointer(0);
    for(int n = 0; n < output.getNumSamples(); n++) {
        phase += (sync ? synced_frequency.getNextValue() : frequency.getNextValue() * 2.0f) / sample_rate;
        
        if(phase >= num_selected) {
            phase -= num_selected;
        }
        
        output_ptr[n] = lfo_tables[shape](phase) * depth.getNextValue();
    }
    
    if(num_channels > 1) {
        output.getSingleChannelBlock(1).copyFrom(output.getSingleChannelBlock(0));
        if(stereo) output.getSingleChannelBlock(1) *= -1.0f;
    }
    
}


void SequenceLFO::sync_with_playhead(AudioPlayHead* playhead) {
    AudioPlayHead::CurrentPositionInfo position_info;
    
    if(sync) {
        int noteDivision = frequency.getTargetValue();
        
        playhead->getCurrentPosition(position_info);
        
        float tempo_freq = position_info.bpm > 0.0f ? 60.0f / position_info.bpm : 0.0f;
        
        float scaler = note_divisions[6 - noteDivision];
        
        synced_frequency.setTargetValue(tempo_freq * scaler);
        
        // sync phase
        float p = position_info.ppqPosition * scaler;
        phase = p - floor(p);
    }
}

bool SequenceLFO::fill_tables() {
    
    for(int i = 0; i < 16; i++) {
        lfo_tables[i].initialise([i](float x) mutable {
            float input_value = fmod(x, 1.0f);
            
            // Sine
            if(i & 1 && x <= 1.0f) {
                return FastMathApproximations::sin((input_value * MathConstants<float>::twoPi) - MathConstants<float>::pi);
            }
            else if(!(i & 1)){
                x += 1.0f;
            }
            
            // Square
            if(i & 2 && x <= 2.0f) {
                return input_value < 0.5f ? -1.0f : 1.0f;
            }
            else if(!(i & 2)){
                x += 1.0f;
            }
            // Triangle
            if(i & 4 && x <= 3.0f) {
                input_value += 0.25;
                if(input_value >= 1.0f) input_value -= 1.0f;
                
                return ((input_value < 0.5f) * (4.0f * input_value - 1.0f)) + ((input_value >= 0.5f) * (1.0f - 4.0f * (input_value - 0.5f)));
            }
            else if(!(i & 4)){
                x += 1.0f;
            }
            
            // Sawtooth
            if(i & 8 && x <= 4.0f) {
                return ((input_value * 2.0f) - 1.0f);
            }
            else if(!(i & 8)){
                x += 1.0f;
            }
            
            return 0.0f;
            
        }, 0.0f, 4.0f, 128);
    }
    
    return true;
}
