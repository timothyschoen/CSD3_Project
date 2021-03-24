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
}

void PeakScaler::set_gain(float gain_amount) {
   gain = gain_amount;
}


void PeakScaler::get_scaling(const dsp::AudioBlock<float>& input, dsp::AudioBlock<float>& output){
   
   float heavy_scalar = heavy ? 1e4 : 75.0f;
   float scaled_gain = dsp::FastMathApproximations::exp(-1.0 / (gain * heavy_scalar)) + (1.0 - dsp::FastMathApproximations::exp(-1.0 / heavy_scalar));
   scaled_gain -= heavy ? 0.0f : 0.002f;
   
   if(!std::isfinite(scaled_gain)) scaled_gain = 0.0f;
   
   for(int ch = 0; ch < input.getNumChannels(); ch++) {
      std::vector<float> real(input.getNumSamples());
      std::vector<float> imag(input.getNumSamples());
      
      hilbert[ch].process(input.getChannelPointer(ch), real.data(), imag.data(), (int)input.getNumSamples());
      
      for(int i = 0; i < input.getNumSamples(); i++){
         float abs_value = std::abs(std::complex<float>(real[i], imag[i]));
         
         abs_value = (abs_value * scaled_gain) + (1.0f - scaled_gain);

         max_values.getChannelPointer(ch)[i] = abs_value;
         inv_values.getChannelPointer(ch)[i] = 1.0f / abs_value;
      }
   }
   output.copyFrom(inv_values);
}

void PeakScaler::set_heavy(bool is_heavy) {
   heavy = is_heavy;
}

void PeakScaler::get_inverse(dsp::AudioBlock<float>& output)
{
   output.copyFrom(max_values);
}
