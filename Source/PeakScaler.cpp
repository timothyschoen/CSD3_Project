/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "PeakScaler.hpp"
#include <complex>

PeakScaler::PeakScaler(const dsp::ProcessSpec& spec, int oversample_factor) {
    
    sample_rate = spec.sampleRate;
    
    hilbert.resize(spec.numChannels, Hilbert(oversample_factor));
    
    max_values = dsp::AudioBlock<float>(value_data, spec.numChannels, spec.maximumBlockSize);
    inv_values = dsp::AudioBlock<float>(inv_data, spec.numChannels, spec.maximumBlockSize);
    smoothed_gain = dsp::AudioBlock<float>(gain_data, 1, spec.maximumBlockSize);
    
    gain.reset(spec.sampleRate, 0.02f);
}

void PeakScaler::set_gain(float gain_amount) {
    
    target_gain = gain_amount;
    float heavy_scalar = heavy ? 1e4 : 75.0f;
    float scaled_gain = dsp::FastMathApproximations::exp(-1.0f / (std::clamp(gain_amount, 0.1f, 1.0f) * heavy_scalar)) + (1.0f - dsp::FastMathApproximations::exp(-1.0f / heavy_scalar));
    scaled_gain -= heavy ? 0.0f : 0.002f;
    
    gain.setTargetValue(scaled_gain);
    
}


void PeakScaler::get_scaling(const dsp::AudioBlock<float>& input, dsp::AudioBlock<float>& output){
    smoothed_gain.fill(1.0f);
    smoothed_gain *= gain;
    
    for(int ch = 0; ch < input.getNumChannels(); ch++) {
        std::vector<float> real(input.getNumSamples());
        std::vector<float> imag(input.getNumSamples());
        
        hilbert[ch].process(input.getChannelPointer(ch), real.data(), imag.data(), (int)input.getNumSamples());
        
        for(int i = 0; i < input.getNumSamples(); i++){
            float abs_value = std::abs(std::complex<float>(real[i], imag[i]));
            float gain_value = smoothed_gain.getSample(0, i);
            
            abs_value = (abs_value * gain_value) + (1.0f - gain_value);
            
            max_values.getChannelPointer(ch)[i] = abs_value;
            inv_values.getChannelPointer(ch)[i] = 1.0f / abs_value;
        }
    }
    output.copyFrom(inv_values);
}

void PeakScaler::set_heavy(bool is_heavy) {
    heavy = is_heavy;
    set_gain(target_gain);
}

void PeakScaler::get_inverse(dsp::AudioBlock<float>& output)
{
    output.copyFrom(max_values);
}
