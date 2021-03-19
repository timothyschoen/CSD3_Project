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
            
            if(order == 0) return (float)tanh(x * 2.0);
            
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
    
    feedback = dsp::AudioBlock<float>(feedback_data, spec.numChannels, spec.maximumBlockSize);
        
    shift = order;
    g = gain;
    
    sample_rate = spec.sampleRate;
    
    set_table(order, gain, second_kind);
    
    svf.prepare(spec);
    svf.setType(dsp::StateVariableTPTFilterType::lowpass);
    svf.setCutoffFrequency(20000);
    svf.setResonance(1.0 / sqrt(2.0));
    
    sine_phase.resize(spec.numChannels, 0.0);
    
    set_filter_type(0);
    set_filter_cutoff(20000);
    
    set_stereo(true);
    
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
    
    for(int ch = 0; ch < input.getNumChannels(); ch++) {
        auto* input_ptr = input.getChannelPointer(ch);
        auto* output_ptr = output.getChannelPointer(ch);
        
        for(int n = 0; n < input.getNumSamples(); n++) {

            // Add stereo switch!
            sine_phase[ch] += mod_freq / sample_rate;

            if (sine_phase[ch] >= 1)
                sine_phase[ch] -= 1;

            float sine = dsp::FastMathApproximations::sin(sine_phase[ch] * 2.0 * M_PI) * mod_depth * 2.5;
            
            float order = poly_order + sine;
            
            order = std::clamp(order, 0.0f, 12.0f);
            
            int first_order = order;
            int second_order = first_order + 1;
            float amp = (order - (float)first_order);
            

            float y1 = (*current_table)[first_order]->processSample(input_ptr[n]) * (1.0 - amp);
            float y2 = (*current_table)[second_order]->processSample(input_ptr[n]) * amp;
            
            output_ptr[n] = (y1 + y2) * gain;
        
        }
    }
    
    //svf.process(dsp::ProcessContextReplacing<float>(output));
}



void ChebyshevTable::set_filter_type(int filterType) {
    svf.setType((dsp::StateVariableTPTFilterType)filterType);
}

void ChebyshevTable::set_filter_cutoff(float cutoff) {
    svf.setCutoffFrequency(cutoff);
    
}

void ChebyshevTable::set_filter_resonance(float resonance) {
    svf.setResonance(resonance);
}

void ChebyshevTable::set_stereo(bool stereo) {
    if(stereo) {
        for(int i = 1; i < sine_phase.size(); i++) {
            sine_phase[i] = sine_phase[0] + (M_PI / (sine_phase.size() - 1));
        }
    }
    else {
        for(int i = 1; i < sine_phase.size(); i++) {
            sine_phase[i] = sine_phase[0];
        }
    }
}

void ChebyshevTable::set_table(float order, float g, bool second_kind)
{
    // Apply gain scaling
    gain =  pow((g + 1.0), 2.0) - 1.0;
    
    poly_order = order;
    current_table = second_kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;

}
