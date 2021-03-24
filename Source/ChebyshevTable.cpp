/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */
#include "ChebyshevTable.hpp"



bool ChebyshevFactory::fillTables() {
    
    for(int i = 0; i < num_polynomials; i++) {
        int order = i;
        
        first_tables[i].initialise([order](float x) mutable {
            if(x == 0.0) return 0.0f;
            
            if(order == 0) return (float)tanh(x * 2.0);
                        
            float y;
            y = cos(acos(x) * order);
            
            assert(std::isfinite(y));
            
            return y;
            
        }, -1.0, 1.0, 1<<8);
        
        
        second_tables[i].initialise([order](float x) mutable {
            if(x == 0.0) return 0.0f;
            
            if(order == 0) return (float)tanh(x * 2.0);
                        
            float y;
            if(x == -1.0) x += 1e-5;
            if(x == 1.0) x -= 1e-5;
            y = sin((order + 1.0) * acos(x)) / sin(acos(x)) + (1 - (order & 1));

            
            assert(std::isfinite(y));
            
            return y;
            
        }, -1.0, 1.0, 1<<8);
    }
    return true;
}


ChebyshevTable::ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind) {
    
        
    shift = order;
    g = gain;
    
    sample_rate = spec.sampleRate;
    
    set_table(order, gain, second_kind);
    
    num_channels = spec.numChannels;
    
    lfo_states.resize(num_channels, 0.0);
    
    lfos.resize(spec.numChannels, SequenceLFO(spec));
    
    set_stereo(true);
    
    buffer = dsp::AudioBlock<float>(buffer_data, num_channels, spec.maximumBlockSize);
    
    
}

void ChebyshevTable::set_scaling(float amount)
{
    scaling = amount;
    
}

void ChebyshevTable::process(std::vector<dsp::AudioBlock<float>>& input, std::vector<dsp::AudioBlock<float>>& output) {
    
    
    for(int ch = 0; ch < input[0].getNumChannels(); ch++) {
        lfo_states[ch] = lfos[ch].get_state();
    }
    
    for(int b = 0; b < input.size(); b++) {
            
        if(!enabled) {
            output[b].clear();
            continue;
        }
        
        // Restore lfo state for each band
        for(int ch = 0; ch < input[b].getNumChannels(); ch++) {
            lfos[ch].set_state(lfo_states[ch]);
        }

        buffer.copyFrom(input[b]);
        buffer *= scaling;
        
        for(int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto* channel_ptr = buffer.getChannelPointer(ch);
            
            for(int n = 0; n < input[b].getNumSamples(); n++) {

                lfos[ch].tick();

                float mod_source = lfos[ch].get_sample();
                
                float order = poly_order + mod_source;
                
                order = std::clamp(order, 0.0f, 12.0f);
                
                bool single = (even + odd) == 1;
                int distance = single ? 2 : 1;
                
                int first_order = single ? odd + round(order * 0.5) * 2.0 : order;
                int second_order = first_order + distance;
                float amp = (order - (float)first_order);
                

                float y1 = (*current_table)[first_order].processSample(channel_ptr[n]) * (1.0 - amp);
                float y2 = (*current_table)[second_order].processSample(channel_ptr[n]) * amp;
                
                channel_ptr[n] = (y1 + y2) * gain;
            
            }
        }
        output[b] += buffer;
    }
    
    
}



void ChebyshevTable::set_stereo(bool stereo) {
    
    for(int i = 0; i < lfos.size(); i++) {
        lfos[i].set_inverse(stereo ? (i & 1) : 0);
    }
}

void ChebyshevTable::set_mod_depth(float depth) {
    mod_depth = depth;
    for(auto& lfo : lfos) {
        lfo.set_depth(depth);
    }
}

void ChebyshevTable::set_mod_rate(float rate) {
    mod_freq = rate;
    
    for(auto& lfo : lfos) {
        lfo.set_frequency(rate);
    }
}

void ChebyshevTable::set_mod_shape(int shape_flag) {
    mod_shape = shape_flag;
    for(auto& lfo : lfos) {
        lfo.set_voice(shape_flag);
    }
}

void ChebyshevTable::set_table(float order, float g, bool second_kind)
{
    // Apply gain scaling
    gain =  pow((g + 1.0), 2.0) - 1.0;
    
    poly_order = order;
    current_table = second_kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;

}
void ChebyshevTable::set_even(bool enable_even) {
    even = enable_even;
}

void ChebyshevTable::set_odd(bool enable_odd) {
    odd = enable_odd;
}

