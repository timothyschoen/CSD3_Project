/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PeakScaler.hpp"

inline static const int num_polynomials = 15;

class ChebyshevFactory : public dsp::LookupTableTransform<float>
{
    
public:
    ChebyshevFactory(int order, bool second_kind);
    
    static void deallocate();

    
    static std::array<ChebyshevFactory*, num_polynomials> fillTables(bool second_kind);
    
    inline static std::array<ChebyshevFactory*, num_polynomials> first_tables = fillTables(false);
    inline static std::array<ChebyshevFactory*, num_polynomials> second_tables = fillTables(true);
    
    
};




class ChebyshevTable
{
public:
    //==============================================================================
    
    
    bool enabled = true;
    float sample_rate;
    int factor = 1;
    dsp::LookupTableTransform<float> table;
    
    dsp::Oversampling<float> oversampler;
    
    dsp::ProcessorDuplicator<dsp::StateVariableFilter::Filter<float>, dsp::StateVariableFilter::Parameters<float>> svf;
    
    std::unique_ptr<PeakScaler> scaler;
    
    float last_cutoff = 20000;
    float last_q = 0.1;
    float shift;
    float g;
    
    ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind = false);
    
    void set_scaling(float amount);
    void set_gain(float amount);
    
    
    void process(dsp::AudioBlock<float> input, dsp::AudioBlock<float> output, int num_samples);
        
    
    void set_filter_type(int filterType);
    
    void set_filter_cutoff(float cutoff, float resonance = 1.0 / sqrt(2.0));
    
    void set_table(float order, float gain, bool second_kind = false);

};
