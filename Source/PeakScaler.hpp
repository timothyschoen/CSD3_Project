/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Hilbert.hpp"

struct PeakScaler
{
    float gain = 1.0;
    
    float release_ms = 10.0;
    
    
    float alpha_release;
    float alpha_attack = 0.0;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> amp_filter;
    
    std::vector<Hilbert> hilbert;
    
    std::vector<float> state;
    dsp::AudioBlock<float> max_values;
    dsp::AudioBlock<float> inv_values;
    
    HeapBlock<char> value_data;
    HeapBlock<char> inv_data;
    
    PeakScaler(const dsp::ProcessSpec& spec);

    void set_gain(float gain_amount);
    
    void get_scaling(dsp::AudioBlock<float> input, dsp::AudioBlock<float> output);
    void get_inverse(dsp::AudioBlock<float> output);
};
