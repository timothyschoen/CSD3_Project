/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PeakScaler.hpp"
#include "SequenceLFO.hpp"

inline static const int num_polynomials = 15;

class ChebyshevFactory : public dsp::LookupTableTransform<float>
{
    
public:
    
    static bool fillTables();
    
    inline static std::array<dsp::LookupTableTransform<float>, num_polynomials> first_tables;
    inline static std::array<dsp::LookupTableTransform<float>, num_polynomials> second_tables;
    
    inline static bool initialised = fillTables();
    
    
private:
    
    ChebyshevFactory(int order, bool second_kind);
};




class ChebyshevTable
{
public:
    //==============================================================================
    
    float scaling;
    bool enabled = true;
    float sample_rate;
    
    float poly_order;
    float gain;
    
    std::array<dsp::LookupTableTransform<float>, num_polynomials>* current_table;
        
    float shift;
    float g;
    
    bool odd = true, even = true;
    
    int num_channels;
    
    std::vector<SequenceLFO> lfos;
    std::vector<float> lfo_states;
    
    float mod_freq = 2.0;
    float mod_depth = 0.25;
    int mod_shape = 0.0;
    
    ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind = false);
    
    void set_scaling(float amount);
    
    void process(std::vector<dsp::AudioBlock<float>>& input, std::vector<dsp::AudioBlock<float>>& output);
    
    dsp::AudioBlock<float> buffer;
    HeapBlock<char> buffer_data;
    
    void set_mod_depth(float depth);
    void set_mod_rate(float rate);
    void set_mod_shape(int shape_flag);
    
    void set_stereo(bool stereo);
    
    void set_even(bool enable_even);
    void set_odd(bool enable_odd);
    
    void set_table(float order, float gain, bool second_kind = false);

};
