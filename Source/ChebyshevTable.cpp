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
            if(order == 0) return 0.0f;
                        
            float offset = (order - 1 & 1) - (((order & 3) == 0) * 2);
            float y = cos(acos(x) * order) + offset;
            
            jassert(std::isfinite(y));
            
            return y;
            
        }, -1.0f, 1.0f, 1<<8);
        
        
        second_tables[i].initialise([order](float x) mutable {
            if(x == 0.0f) return 0.0f;
            
            if(order == 0) return (float)tanh(x * 2.0f);
            if(x == -1.0f) x += 1e-5;
            if(x == 1.0f) x -= 1e-5;
            
            float offset = (order - 1 & 1) - (((order & 3) == 0) * 2);
            float y = sin((order + 1.0f) * acos(x)) / sin(acos(x)) + offset;

            jassert(std::isfinite(y));
            
            return y;
            
        }, -1.0f, 1.0f, 1<<8);
    }
    return true;
}


ChebyshevTable::ChebyshevTable(const ProcessSpec& spec, float order, float new_volume, bool second_kind) :  lfo(spec)

{
    
    process_spec = spec;
    
    sample_rate = spec.sampleRate;
    
    //set_table(order, volume, second_kind);
    
    set_order(order, second_kind);
    set_volume(new_volume);
    
    num_channels = spec.numChannels;
    
    smoothed_order.reset(sample_rate, 0.02f);
    smoothed_volume.reset(sample_rate, 0.02f);
    smoothed_scaling.reset(sample_rate, 0.02f);

    set_stereo(true);
    
    buffer = AudioBlock<float>(buffer_data, num_channels, spec.maximumBlockSize);
    lfo_buffer = AudioBlock<float>(lfo_buffer_data, num_channels, spec.maximumBlockSize);
    
    smoothed_order_buffer = AudioBlock<float>(smoothed_order_data, 1, spec.maximumBlockSize);
    smoothed_volume_buffer = AudioBlock<float>(smoothed_volume_data, 1, spec.maximumBlockSize);
    smoothed_scaling_buffer = AudioBlock<float>(smoothed_scaling_data, num_channels, spec.maximumBlockSize);
    temp_buffer = AudioBlock<float>(temp_data, num_channels, spec.maximumBlockSize);
}

DistortionState ChebyshevTable::get_state() {
    return {enabled, kind, poly_order, volume, scaling, mod_freq, mod_depth, mod_shape, lfo_sync, lfo_stereo, high_mode, filter_freqs};
}

void ChebyshevTable::set_state(const DistortionState& state) {
    std::tie(enabled, kind, poly_order, volume, scaling, mod_freq, mod_depth, mod_shape, lfo_sync, lfo_stereo, high_mode, filter_freqs) = state;

    current_table = kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;
    
    smoothed_order.setCurrentAndTargetValue(poly_order);
    smoothed_volume.setCurrentAndTargetValue(volume);
    smoothed_scaling.setCurrentAndTargetValue(scaling);

    // Make sure state is also set for all LFOs
    set_mod_depth(mod_depth);
    set_mod_rate(mod_freq);;
    set_mod_shape(mod_shape);
    set_stereo(lfo_stereo);
    set_sync(lfo_sync);
    
    set_centre_freqs(filter_freqs);
}


void ChebyshevTable::set_scaling(float amount)
{
    smoothed_scaling.setTargetValue(amount);
    scaling = amount;
    
}

void ChebyshevTable::set_enabled(bool is_enabled)
{
    enabled = is_enabled;
}

void ChebyshevTable::process(std::vector<AudioBlock<float>>& input, std::vector<AudioBlock<float>>& output) {
    
    int num_samples = input[0].getNumSamples();
    
    lfo_buffer.clear();
    auto subblock = lfo_buffer.getSubBlock(0, num_samples);
    lfo.process(subblock);

    // Calculate smoothed parameters
    smoothed_order_buffer.fill(1.0f);
    smoothed_order_buffer *= smoothed_order;
    
    smoothed_volume_buffer.fill(1.0f);
    smoothed_volume_buffer *= smoothed_volume;
    
    smoothed_scaling_buffer.fill(1.0f);
    smoothed_scaling_buffer *= smoothed_scaling;
    
    auto* smoothed_order_ptr = smoothed_order_buffer.getChannelPointer(0);
    
    for(int b = 0; b < input.size(); b++) {
        // Don't process the expected target region is above either nyquist or the human hearing limit!
        if(!enabled || filter_freqs[b] * smoothed_order.getTargetValue() > std::min<float>(sample_rate / 2 - 1, 20000.0f)) {
            output[b].fill(0.0f);
            continue;
        }
        
        buffer.copyFrom(input[b]);
        
        // Get and apply scaling
        temp_buffer.copyFrom(smoothed_scaling_buffer);
        
        buffer *= temp_buffer.getSubBlock(0, num_samples);
        
        for(int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto channel_block = buffer.getSingleChannelBlock(ch);
            auto* channel_ptr = buffer.getChannelPointer(ch);
            
            for(int n = 0; n < num_samples; n++) {
                // Get LFO value
                float mod_source = lfo_buffer.getSample(ch, n);
                
                // Calculate final polynomial order
                float order = smoothed_order_ptr[n] + mod_source;
                
                // 0th order polynomial is silence so start at 1
                order = std::clamp(order + 1.0f, 1.0f, 20.0f);
                
                // Find neighboring integer polynomials
                int first_order = order;
                int second_order = first_order + 1;
                
                // Calculate mix
                float amp = order - (float)first_order;

                // Get values from wavetables and mix together
                float y1 = (*current_table)[first_order].processSample(channel_ptr[n]) * (1.0f - amp);
                float y2 = (*current_table)[second_order].processSample(channel_ptr[n]) * amp;
                
                channel_ptr[n] = (y1 + y2);
                
                jassert(std::isfinite(channel_ptr[n]));
            
            }
            // Apply smoothed polynomial volume (y-axis value)
            channel_block *= smoothed_volume_buffer.getSubBlock(0, num_samples);
        }
        
        // Noise filter:
        // This filter is set to the expected output frequency range, which order * filter_freq
        // Normally a bandpass filter, or a highpass filter in high mode
        auto final_buffer = buffer.getSubBlock(0, num_samples);
        noise_filters[b].process(ProcessContextReplacing<float>(final_buffer));
        
        output[b] += final_buffer;
    }
}
void ChebyshevTable::set_high(bool low_mode) {
    high_mode = !low_mode;
    
    // Apply mode to filters
    for(int b = 0; b < filter_freqs.size(); b++) {
        float centre_freq = std::clamp<float>(filter_freqs[b] * smoothed_order.getTargetValue(), 0, sample_rate / 2 - 1);
        noise_filters[b].setType(high_mode ? StateVariableTPTFilterType::bandpass : StateVariableTPTFilterType::highpass);
        noise_filters[b].setCutoffFrequency(high_mode ? centre_freq : (centre_freq * (2.0f / 3.0f)));
    }
}

void ChebyshevTable::set_centre_freqs(std::vector<float> centre_freqs) {
    filter_freqs = centre_freqs;
    
    noise_filters.clear();
    noise_filters.resize(centre_freqs.size());
    
    // Prepare filters at centre frequencies multiplied by polynomial order
    for(int b = 0; b < filter_freqs.size(); b++) {
        float centre_freq = std::clamp<float>(filter_freqs[b] * smoothed_order.getTargetValue(), 0, sample_rate / 2 - 1);
        noise_filters[b].reset();
        noise_filters[b].prepare(process_spec);
        noise_filters[b].setType(high_mode ? StateVariableTPTFilterType::bandpass : StateVariableTPTFilterType::highpass);
        noise_filters[b].setCutoffFrequency(high_mode ? centre_freq : (centre_freq * (2.0f / 3.0f)));
        //noise_filters[b].setResonance(1.0f / sqrt(2));
        noise_filters[b].setResonance(1.2f);
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

void ChebyshevTable::set_volume(float new_volume) {
    // Apply volume scaling
    volume = pow((new_volume + 1.0f), 2.0f) - 1.0f;
    smoothed_volume.setTargetValue(volume);
}
void ChebyshevTable::set_order(float order, bool second_kind) {
    kind = second_kind;
    poly_order = order;
    
    smoothed_order.setTargetValue(order);
    
    for(int b = 0; b < filter_freqs.size(); b++) {
        float centre_freq = std::clamp<float>(filter_freqs[b] * smoothed_order.getTargetValue(), 0, sample_rate / 2 - 1);
        noise_filters[b].setType(high_mode ? StateVariableTPTFilterType::bandpass : StateVariableTPTFilterType::highpass);
        noise_filters[b].setCutoffFrequency(high_mode ? centre_freq : (centre_freq * (2.0f / 3.0f)));
    }
    
    current_table = second_kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;
}

void ChebyshevTable::sync_with_playhead(AudioPlayHead* playhead) {
    lfo.sync_with_playhead(playhead);
}

void ChebyshevTable::set_sync(bool sync) {
    lfo_sync = sync;
    lfo.set_sync(sync);
}
