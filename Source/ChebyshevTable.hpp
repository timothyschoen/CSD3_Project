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
    
    static void deallocate();
    
    static std::array<dsp::LookupTableTransform<float>*, num_polynomials> fillTables(bool second_kind);
    
    inline static std::array<dsp::LookupTableTransform<float>*, num_polynomials> first_tables = fillTables(false);
    inline static std::array<dsp::LookupTableTransform<float>*, num_polynomials> second_tables = fillTables(true);
    
    
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
    
    int first_order = 1, second_order = 2;
    float first_amp = 1.0, second_amp = 0.0;
    float gain = 1.0;
    
    dsp::LookupTableTransform<float>* table1 = nullptr;
    dsp::LookupTableTransform<float>* table2 = nullptr;
    
    dsp::ProcessorDuplicator<dsp::StateVariableFilter::Filter<float>, dsp::StateVariableFilter::Parameters<float>> svf;
    
    std::unique_ptr<PeakScaler> scaler;
    
    float last_cutoff = 20000;
    float last_q = 0.1;
    float shift;
    float g;
    
    ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind = false);
    
    void set_scaling(float amount);
    float get_scaling();
    
    
    void process(dsp::AudioBlock<float> input, dsp::AudioBlock<float> output, int num_samples);
    
    void set_filter_type(int filterType);
    
    void set_filter_cutoff(float cutoff, float resonance = 1.0 / sqrt(2.0));
    
    void set_table(float order, float gain, bool second_kind = false);

};
