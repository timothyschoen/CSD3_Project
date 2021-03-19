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
    
    float poly_order;
    float gain;
    
    std::array<dsp::LookupTableTransform<float>*, num_polynomials>* current_table;
    
    dsp::LookupTableTransform<float>* table1 = nullptr;
    dsp::LookupTableTransform<float>* table2 = nullptr;
        
    dsp::StateVariableTPTFilter<float> svf;
    
    
    std::unique_ptr<PeakScaler> scaler;
    
    float shift;
    float g;
    
    
    std::vector<float> sine_phase;
    
    float mod_freq = 2.0;
    float mod_depth = 0.25;
    
    
    ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind = false);
    
    void set_scaling(float amount);
    float get_scaling();
    
    dsp::AudioBlock<float> feedback;
    HeapBlock<char> feedback_data;
    
    
    void process(dsp::AudioBlock<float> input, dsp::AudioBlock<float> output, int num_samples);
    
    void set_filter_type(int filterType);
    void set_filter_cutoff(float cutoff);
    void set_filter_resonance(float resonance);
    
    void set_stereo(bool stereo);
    
    void set_table(float order, float gain, bool second_kind = false);

};
