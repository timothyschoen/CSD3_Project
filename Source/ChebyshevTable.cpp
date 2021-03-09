/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */
#include "ChebyshevTable.hpp"


ChebyshevFactory::ChebyshevFactory(int order, bool second_kind)
{
    initialise([this, order, second_kind](float x) mutable {
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

void ChebyshevFactory::deallocate() {
    for(auto& polynomial : first_tables) {
        delete polynomial;
    }
    for(auto& polynomial : second_tables) {
        delete polynomial;
    }
}


std::array<ChebyshevFactory*, num_polynomials> ChebyshevFactory::fillTables(bool second_kind) {
    std::array<ChebyshevFactory*, num_polynomials> result;
    
    for(int i = 0; i < num_polynomials; i++) {
        result[i] = new ChebyshevFactory(i, second_kind);
    }
    
    return result;
}



ChebyshevTable::ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind) :  oversampler(spec.numChannels, factor, dsp::Oversampling<float>::filterHalfBandFIREquiripple) {
    
    scaler.reset(new PeakScaler(spec));
    
    shift = order;
    g = gain;
    
    sample_rate = spec.sampleRate;
    oversampler.initProcessing(spec.maximumBlockSize);
    
    set_table(order, gain, second_kind);
    
    svf.prepare(spec);
    
    set_filter_type(0);
    set_filter_cutoff(sample_rate / 2.0);
    
}

void ChebyshevTable::set_scaling(float amount)
{
    scaler->set_amount(amount);
}

void ChebyshevTable::set_gain(float amount) {
    scaler->set_gain(amount);
}


void ChebyshevTable::process(dsp::AudioBlock<float> input, dsp::AudioBlock<float> output, int num_samples) {
    
    if(!enabled) {
        output.clear();
        return;
    }
    
    if(factor != 0) {
        
        output.copyFrom(input);
        scaler->scale(output);
        
        dsp::AudioBlock<float> oversampled = oversampler.processSamplesUp(output);
        
        for(int ch = 0; ch < input.getNumChannels(); ch++) {
            table.process(oversampled.getChannelPointer(ch), oversampled.getChannelPointer(ch), num_samples * factor << 1);
            
        }
        
        oversampler.processSamplesDown(output);
        scaler->unscale(output);
    }
    else {
        output.copyFrom(input);
        scaler->scale(output);
        
        
        for(int ch = 0; ch < input.getNumChannels(); ch++) {
            table.process(output.getChannelPointer(ch), output.getChannelPointer(ch), num_samples);
            
        }
        scaler->unscale(output);
    }
    
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

void ChebyshevTable::set_table(float order, float gain, bool second_kind)
{
    table.initialise([this, order, gain, second_kind](float x) mutable {
        float y1, y2;
        
        if(x == 0.0) return 0.0f;
        
        
        int first_order = (int)order;
        int second_order = first_order + 1;
        float second_amp = (order - (float)first_order);
        float first_amp = 1.0 - second_amp;
        
        if(second_kind) {
            y1 = ChebyshevFactory::second_tables[first_order]->processSample(x);
            y2 = ChebyshevFactory::second_tables[second_order]->processSample(x);
        }
        else {
            y1 = ChebyshevFactory::first_tables[first_order]->processSample(x);
            y2 = ChebyshevFactory::first_tables[second_order]->processSample(x);
        }
        
        return (y1 * first_amp + y2 * second_amp) * gain;
        
    }, -1.0, 1.0, 1<<8);
}
