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
            if(x == 0.0f) return 0.0f;
            
            if(order == 0) return (float)tanh(x * 2.0f);
                        
            
            float offset = (order - 1 & 1) - (((order & 3) == 0) * 2);
            
            float y = cos(acos(x) * order) + offset;
            
            assert(std::isfinite(y));
            
            return y;
            
        }, -1.0f, 1.0f, 1<<8);
        
        
        second_tables[i].initialise([order](float x) mutable {
            if(x == 0.0f) return 0.0f;
            
            if(order == 0) return (float)tanh(x * 2.0f);
            if(x == -1.0f) x += 1e-5;
            if(x == 1.0f) x -= 1e-5;
            
            float offset = (order - 1 & 1) - (((order & 3) == 0) * 2);
            
            float y = sin((order + 1.0f) * acos(x)) / sin(acos(x)) + offset;

            
            assert(std::isfinite(y));
            
            return y;
            
        }, -1.0f, 1.0f, 1<<8);
    }
    return true;
}


ChebyshevTable::ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind) :  lfo(spec)

{
    
    sample_rate = spec.sampleRate;
    
    set_table(order, gain, second_kind);
    
    num_channels = spec.numChannels;
    
    smoothed_order.reset(sample_rate, 0.02f);
    smoothed_gain.reset(sample_rate, 0.02f);
    smoothed_scaling.reset(sample_rate, 0.02f);

    set_stereo(true);
    
    buffer = dsp::AudioBlock<float>(buffer_data, num_channels, spec.maximumBlockSize);
    lfo_buffer = dsp::AudioBlock<float>(lfo_buffer_data, num_channels, spec.maximumBlockSize);
    
    smoothed_order_buffer = dsp::AudioBlock<float>(smoothed_order_data, 1, spec.maximumBlockSize);
    smoothed_gain_buffer = dsp::AudioBlock<float>(smoothed_gain_data, 1, spec.maximumBlockSize);
    smoothed_scaling_buffer = dsp::AudioBlock<float>(smoothed_scaling_data, 1, spec.maximumBlockSize);
}

DistortionState ChebyshevTable::get_state() {
    return {enabled, kind, poly_order, gain, scaling, odd, even, mod_freq, mod_depth, mod_shape, lfo_sync, lfo_stereo};
}

void ChebyshevTable::set_state(const DistortionState& state) {
    std::tie(enabled, kind, poly_order, gain, scaling, odd, even, mod_freq, mod_depth, mod_shape, lfo_sync, lfo_stereo) = state;

    current_table = kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;
    
    smoothed_order.setCurrentAndTargetValue(poly_order);
    smoothed_gain.setCurrentAndTargetValue(gain);
    smoothed_scaling.setCurrentAndTargetValue(scaling);

    // Make sure state is also set for all LFOs
    set_mod_depth(mod_depth);
    set_mod_rate(mod_freq);;
    set_mod_shape(mod_shape);
    set_stereo(lfo_stereo);
    set_sync(lfo_sync);
}


void ChebyshevTable::set_scaling(float amount)
{
    smoothed_scaling.setTargetValue(amount);
    scaling = amount;
    
}

void ChebyshevTable::process(std::vector<dsp::AudioBlock<float>>& input, std::vector<dsp::AudioBlock<float>>& output) {
    
    int num_samples = input[0].getNumSamples();
    
    lfo_buffer.clear();
    auto subblock = lfo_buffer.getSubBlock(0, num_samples);
    lfo.process(subblock);

    smoothed_order_buffer.fill(1.0f);
    smoothed_order_buffer *= smoothed_order;
    
    smoothed_gain_buffer.fill(1.0f);
    smoothed_gain_buffer *= smoothed_gain;
    
    smoothed_scaling_buffer.fill(1.0f);
    smoothed_scaling_buffer *= smoothed_scaling;
    
    auto* smoothed_order_ptr = smoothed_order_buffer.getChannelPointer(0);
    
    for(int b = 0; b < input.size(); b++) {
            
        if(!enabled) continue;
        
        buffer.copyFrom(input[b]);
        
        for(int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto channel_block = buffer.getSingleChannelBlock(ch);
            auto* channel_ptr = buffer.getChannelPointer(ch);
            
            channel_block *= smoothed_scaling_buffer;

            for(int n = 0; n < num_samples; n++) {
                float mod_source = lfo_buffer.getSample(ch, n);
                
                float order = smoothed_order_ptr[n] + mod_source;
                
                order = std::clamp(order, 0.0f, 12.0f);
                
                int first_order = order;
                int second_order = first_order + 1;
                
                float amp = (order - (float)first_order);
                
                if(even != odd) {
                    
                    first_order = first_order * 2 + odd;
                    second_order = second_order * 2 + odd;
                }
    
                float y1 = (*current_table)[first_order].processSample(channel_ptr[n]) * (1.0f - amp);
                float y2 = (*current_table)[second_order].processSample(channel_ptr[n]) * amp;
                
                channel_ptr[n] = (y1 + y2);
            
            }
            channel_block *= smoothed_gain_buffer;
        }
        
        output[b] += buffer;
    }
    
    
}



void ChebyshevTable::set_stereo(bool stereo) {
    lfo_stereo = stereo;
    
    lfo.set_stereo(stereo);
}

void ChebyshevTable::set_mod_depth(float depth) {
    mod_depth = depth;
    lfo.set_depth(depth);
}

void ChebyshevTable::set_mod_rate(float rate) {
    mod_freq = rate;
    
    lfo.set_frequency(rate);
}

void ChebyshevTable::set_mod_shape(int shape_flag) {
    mod_shape = shape_flag;
    lfo.set_voice(shape_flag);
}

void ChebyshevTable::set_table(float order, float g, bool second_kind)
{
    // Apply gain scaling
    gain =  pow((g + 1.0f), 2.0f) - 1.0f;
    
    kind = second_kind;
    poly_order = order;
    
    smoothed_gain.setTargetValue(gain);
    smoothed_order.setTargetValue(order);
    
    current_table = second_kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;
}

void ChebyshevTable::set_even(bool enable_even) {
    even = enable_even;
}

void ChebyshevTable::set_odd(bool enable_odd) {
    odd = enable_odd;
}

void ChebyshevTable::sync_with_playhead(AudioPlayHead* playhead) {
    lfo.sync_with_playhead(playhead);
}

void ChebyshevTable::set_sync(bool sync) {
    lfo_sync = sync;
    lfo.set_sync(sync);
}
