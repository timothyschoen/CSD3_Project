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
    float sample_rate;
    bool heavy = false;
    
    std::vector<Hilbert> hilbert;
    
    dsp::AudioBlock<float> max_values, inv_values;
    HeapBlock<char> value_data, inv_data;
    
    PeakScaler(const dsp::ProcessSpec& spec, int oversample_factor);

    void set_gain(float gain_amount);
    void set_heavy(bool is_heavy);
    
    void get_scaling(const dsp::AudioBlock<float>& input, dsp::AudioBlock<float>& output);
    void get_inverse(dsp::AudioBlock<float>& output);
};
