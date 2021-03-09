/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "PeakScaler.hpp"


PeakScaler::PeakScaler(const dsp::ProcessSpec& spec) {
    
    amp_filter.state = dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, 60);
    
    amp_filter.prepare(spec);
    
    alpha_release = 0.9;//exp(-1.0 / (spec.sampleRate * release_ms));
    
    state.resize(spec.numChannels, 1.0);
    max_values = dsp::AudioBlock<float>(value_data, spec.numChannels, spec.maximumBlockSize);
    inv_values = dsp::AudioBlock<float>(inv_data, spec.numChannels, spec.maximumBlockSize);
    
}

void PeakScaler::set_amount(float scaling_amount) {
    scaling = scaling_amount;
}

void PeakScaler::set_gain(float gain_amount) {
    gain = gain_amount;
}


void PeakScaler::scale(dsp::AudioBlock<float> input){
    
    
    float scaled_gain = exp(-1.0 / (gain * 50)) + 0.01980132669;
    
    int block_size = 256;
    int num_blocks = input.getNumSamples() / block_size;
    
    for(int ch = 0; ch < input.getNumChannels(); ch++) {
        for(int b = 0; b < num_blocks; b++) {
            
            auto range = input.getSingleChannelBlock(ch).getSubBlock(b*block_size, block_size).findMinAndMax();
            float val = std::max(std::abs(range.getStart()), std::abs(range.getEnd()));
            val = (val * scaled_gain) + (1.0 - scaled_gain);
            val *= scaling;
            val *= val > 1e-7;
            val += val == 0.0;
            //scale *= 0.1;
            max_values.getSingleChannelBlock(ch).getSubBlock(b*block_size, block_size).fill(val);

        }
        
    }
    // Smooth changes between blocks
    amp_filter.process(dsp::ProcessContextReplacing(max_values));
    
    for(int ch = 0; ch < input.getNumChannels(); ch++) {
        for(int n = 0; n < input.getNumSamples(); n++) {
            inv_values.setSample(ch, n, 1.0 / max_values.getSample(ch, n));
        }
    }
    
    //output.multiplyBy(1.0 / scale);
    /*
     for(int ch = 0; ch < input.getNumChannels(); ch++) {
     for(int n = 0; n < input.getNumSamples(); n++) {
     state[ch] *= alpha_release;
     state[ch] = (1.0 - alpha_attack) * std::max(std::abs(input.getChannelPointer(ch)[n]), state[ch]) + alpha_attack * state[ch];
     
     double val = state[ch];
     
     
     
     // This will control the amount of gain applied
     //val = (val * scaled_gain) + (1.0 - scaled_gain);
     val *= scaling;
     //
     val *= val > 1e-8;
     val += val == 0.0;
     
     
     
     max_values.setSample(ch, n, val);
     inv_values.setSample(ch, n, 1.0 / val);
     }
     } */
    
    input *= inv_values;
    
}

void PeakScaler::unscale(dsp::AudioBlock<float> input)
{
    input *= max_values;
}

