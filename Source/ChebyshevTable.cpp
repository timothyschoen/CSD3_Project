/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */
#include "ChebyshevTable.hpp"


void ChebyshevFactory::deallocate() {
    for(auto& polynomial : first_tables) {
        delete polynomial;
    }
    for(auto& polynomial : second_tables) {
        delete polynomial;
    }
}


std::array<dsp::LookupTableTransform<float>*, num_polynomials> ChebyshevFactory::fillTables(bool second_kind) {
    std::array<dsp::LookupTableTransform<float>*, num_polynomials> result;
    
    
    for(int i = 0; i < num_polynomials; i++) {
        int order = i;
        result[i] = new dsp::LookupTableTransform<float>();
        
        result[i]->initialise([order, second_kind](float x) mutable {
            if(x == 0.0) return 0.0f;
            
            float offset = (order - 1 & 1) - (((order & 3) == 0) * 2);
            
            float y;
            if(second_kind) {
                // Because sin(acos(-1 or 1)) == 0.0
                if(x == -1.0) x += 1e-5;
                if(x == 1.0) x -= 1e-5;
                y = sin((order + 1.0) * acos(x)) / sin(acos(x)) + offset;
            }
            else {
                y = cos(acos(x) * order) + offset;
            }
            
            assert(std::isfinite(y));
            
            return y;
            
        }, -1.0, 1.0, 1<<8);
    }
    
    return result;
}



ChebyshevTable::ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind) {
        
    shift = order;
    g = gain;
    
    sample_rate = spec.sampleRate;
    
    set_table(order, gain, second_kind);
    
    svf.prepare(spec);
    
    set_filter_type(0);
    set_filter_cutoff(sample_rate / 2.0);
    
}

float ChebyshevTable::get_scaling()
{
    return scaling;
}

void ChebyshevTable::set_scaling(float amount)
{
    scaling = amount;
    
}

void ChebyshevTable::process(dsp::AudioBlock<float> input, dsp::AudioBlock<float> output, int num_samples) {
    
    if(!enabled) {
        output.clear();
        return;
    }

    output.copyFrom(input);
    //scaler->scale(output);
    
    for(int ch = 0; ch < input.getNumChannels(); ch++) {
        auto* input_ptr = input.getChannelPointer(ch);
        auto* output_ptr = output.getChannelPointer(ch);
        
        for(int n = 0; n < input.getNumSamples(); n++) {
        
            float y1 = table1->processSample(input_ptr[n]);
            float y2 = table2->processSample(input_ptr[n]);
            
            output_ptr[n] = (y1 * first_amp + y2 * second_amp) * gain;
        
        }
    }
    
    //scaler->unscale(output);
    
    svf.process(dsp::ProcessContextReplacing<float>(output));
}



void ChebyshevTable::set_filter_type(int filterType) {
    svf.state->setCutOffFrequency(sample_rate, last_cutoff, last_q);
    switch (filterType)
    {
        case 0:  svf.state->type = dsp::StateVariableFilter::Parameters<float>::Type::lowPass;  break;
        case 2: svf.state->type = dsp::StateVariableFilter::Parameters<float>::Type::highPass; break;
        case 1: svf.state->type = dsp::StateVariableFilter::Parameters<float>::Type::bandPass; break;
    }
}

void ChebyshevTable::set_filter_cutoff(float cutoff, float resonance) {
    last_q = resonance;
    last_cutoff = cutoff;
    svf.state->setCutOffFrequency(sample_rate, cutoff, resonance);
}

void ChebyshevTable::set_table(float order, float g, bool second_kind)
{
    // Apply gain scaling
    gain =  pow((g + 1.0), 2.0) - 1.0;
    first_order = (int)order;
    second_order = first_order + 1;
    second_amp = (order - (float)first_order);
    first_amp = 1.0 - second_amp;
    
    
    if(second_kind) {
        table1 = ChebyshevFactory::second_tables[first_order];
        table2 = ChebyshevFactory::second_tables[second_order];
    }
    else {
        table1 = ChebyshevFactory::first_tables[first_order];
        table2 = ChebyshevFactory::first_tables[second_order];
    }
}
