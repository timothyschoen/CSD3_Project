/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>


struct PeakScaler
{
    
    float scaling = 1.0;
    float gain = 1.0;
    
    float release_ms = 10.0;
    
    
    float alpha_release;
    float alpha_attack = 0.0;
    //float alpha_attack;
    

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> amp_filter;
    
    
    std::vector<float> state;
    dsp::AudioBlock<float> max_values;
    dsp::AudioBlock<float> inv_values;
    
    HeapBlock<char> value_data;
    HeapBlock<char> inv_data;
    
    PeakScaler(const dsp::ProcessSpec& spec);
    
    void set_amount(float scaling_amount);
    void set_gain(float gain_amount);
    
    void scale(dsp::AudioBlock<float> input);
    void unscale(dsp::AudioBlock<float> input);
    
    
};
