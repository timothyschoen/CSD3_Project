//
//  main.cpp
//  WindowedSineFit
//
//  Created by Timothy Schoen on 24/08/2021.
//

#include "MonoDistortion.hpp"

#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>


MonoDistortion::MonoDistortion(){
    block.resize(block_size);
    amp_channel.resize(block_size);
    amp_history.resize(block_size);
    freq_buffer.resize(block_size);
    history.resize(block_size, 0.0f);
    out_history.resize(block_size, 0.0f);
    current_window.resize(block_size, 0.0f);
    input_buffer.resize(block_size, 0.0f);
    future_buffer.resize(block_size, 0.0f);
    
    amp_delay_line.resize(block_size * 2.0f, 0.0f);
    delay_line.resize(block_size * 2.0f, 0.0f);
    
    poly_filtered_peak.resize(128, 0.0f);
    
    rate_shifter.prepare({sample_rate, block_size, 1});
    
    for(auto& group : svf) {
        for(auto& filter : group) {
            filter.prepare({sample_rate, block_size, 8});
            filter.setType(StateVariableTPTFilterType::bandpass);
            filter.setResonance(1.0f / sqrt(2.0f));
        }
    }
    
    
    downsample_filter.setCoefficients(IIRCoefficients::makeLowPass(sample_rate, 22050.0f / 4.0f, 1.0f / sqrt(2.0f)));
    
}


void MonoDistortion::receive_message(const Identifier& id, float value, int idx)  {
    
    auto& [harmonic, amp, phase] = harmonics[idx];
    if(id == Identifier("X")) {
        //bool kind = changed_tree.getProperty("Kind");
        //order = value * 8.1 + 0.12;
        //smoothed_order.setTargetValue(order);
        harmonic = value * 8.1 + 0.12;
        
        rate_shifter.set_ratio(harmonic);
        
    }
    else if(id == Identifier("Y")) {
        amp = 1.0f - value;
        //smoothed_scaling.setTargetValue(new_gain);
        //scaling = new_gain;
    }
    else if(id == Identifier("Disharmonic")) {
        disharmonic = value;
    }
    else if(id == Identifier("Kind")) {
        poly = value;
    }
    else if(id == Identifier("Volume")) {
        compression_amt = value;
    }
    else if(id == Identifier("MinFreq")) {
        int new_value = jmap<float>(value, 20, 110);
        if(new_value != min_freq) {
            min_freq = new_value;
            //chroma_filter = ChromaFilter(min_freq, max_freq);
        }
        
        
    }
    else if(id == Identifier("MaxFreq")) {
        int new_value = jmap<float>(value, 20, 110);
        if(new_value != max_freq) {
            max_freq = new_value;
            //chroma_filter = ChromaFilter(min_freq, max_freq);
        }
    }
    /*
     
     else if(id == Identifier("Kind")) {
     kind = value;
     current_table = kind ? &ChebyshevFactory::second_tables : &ChebyshevFactory::first_tables;
     }
     else if(id == Identifier("Phase")) {
     invert_phase = value;
     }
     else if(id == Identifier("ModDepth")) {
     mod_depth = value;
     lfo.set_depth(value);
     }
     else if(id == Identifier("ModSettings")) {
     bool sync = (int)value & 1;
     lfo_sync = sync;
     lfo.set_sync(sync);
     
     bool stereo = (int)value & 2;
     lfo_stereo = stereo;
     lfo.set_stereo(stereo);
     }
     else if(id == Identifier("ModShape")) {
     mod_shape = (int)value;
     lfo.set_voice(mod_shape);
     }
     else if(id == Identifier("ModRate")) {
     mod_freq = value;
     lfo.set_frequency(mod_freq);
     }
     else if(id == Identifier("Enabled")) {
     enabled = value;
     }
     else if(id == Identifier("Volume")) {
     float new_volume = value * 1.5f;
     // Apply volume scaling
     volume = pow((new_volume + 1.0f), 2.0f) - 1.0f;
     smoothed_volume.setTargetValue(volume);
     }
     */
}

void MonoDistortion::mute(int idx)
{
    harmonics[idx] = {0, 0, 0};
}


void MonoDistortion::process(Samples channel, Samples& output) {
    
    // TODO: fix for blocks that are too small!
    int num_blocks = (int)channel.size() / block_size;
    
    bool output_satisfied = false;
    
    if(samples_ready >= channel.size()) {
        samples_ready -= channel.size();
        std::copy(future_buffer.begin(), future_buffer.begin(), output.begin());
        std::copy(future_buffer.begin() + channel.size(), future_buffer.end(), future_buffer.begin());
        output_satisfied = true;
    }
    
    
    for(int n = 0; n < channel.size(); n++) {
        input_buffer[fifo_idx] = channel[n];
        fifo_idx++;
        
        // Just create circular input and output buffers...
        
        if(fifo_idx >= block_size) {
            fifo_idx -= block_size;
            
            auto out_samples = Samples(block_size);
            
            if(poly) {
                process_poly(input_buffer, out_samples);
            }
            else {
                process_block(input_buffer, out_samples);
            }
            
            if(output_satisfied) {
                std::copy(out_samples.begin(), out_samples.end(), future_buffer.begin() + samples_ready);
                
                samples_ready += out_samples.size();
            }
            else {
                

                std::copy(out_samples.begin(), out_samples.begin() + output.size(), output.begin());
                
                
                std::copy(out_samples.begin() + output.size(), out_samples.end(), future_buffer.begin() + samples_ready);
                
                samples_ready += out_samples.size() - output.size();
            }
            
            
        }
    }
}

void MonoDistortion::process_poly(Samples channel, Samples& output)
{
    //auto fft = dsp::FFT(11);
    
    auto filtered = chroma_filter.process(channel);
    
    
    for(int peak = 0; peak < filtered.size(); peak++) {
        for(int n = 0; n < filtered[peak].size(); n++) {
            float filter_out = filtered[peak][n];
            
            poly_filtered_peak[peak] *= peak_release_scalar;
            poly_filtered_peak[peak] = std::max({poly_filtered_peak[peak], abs(filter_out), 1e-8f});
            
            for(auto& [harmonic, amplitude, phase] : harmonics) {
                if(harmonic == 0 || amplitude == 0) continue;
                
                int lower = harmonic;
                int upper = lower + 1;
                
                float mix = harmonic - (int)harmonic;
                
                float offset_1 = (lower - 1 & 1) - (((lower & 3) == 0) * 2);
                float offset_2 = (upper - 1 & 1) - (((upper & 3) == 0) * 2);
                
                float compression = jmap(jmap(compression_amt, 0.95f, 1.0f), 1.0f, std::max(poly_filtered_peak[peak], 1e-5f));
                
                
                float in_value = acos(std::clamp(filter_out / compression, -1.0f, 1.0f));
                
                
                float out_1 = (cos(in_value * (float)lower) + offset_1) * compression * amplitude;
                float out_2 = (cos(in_value * (float)upper) + offset_2) * compression * amplitude;
                
                /*
                 float out_1 = ChebyshevFactory::second_tables[lower].processSample(in_value) * compression * amplitude;
                 
                 float out_2 = ChebyshevFactory::second_tables[upper].processSample(in_value) * compression * amplitude;
                 */
                
                output[n] += jmap(mix, out_1, out_2);
                
            }
                    
                   
        }
    }
    
    
    
}

void MonoDistortion::process_block(Samples channel, Samples& output) {
    
    audio_thread = Thread::getCurrentThread();
    
    std::vector<std::complex<float>> hilbert_output(channel.size());
    auto phase_block = Samples(channel.size());
    
    //downsample_filter.processSamples(channel.data(), (int)channel.size());
    
    hilbert.process(channel, hilbert_output);
    
    // First get amplitude information
    auto amp_channel = channel;
    for (int i = 0; i < amp_channel.size(); i++)
    {
        peak_amp *= peak_release_scalar;
        peak_amp = std::max({peak_amp, abs(hilbert_output[i]), 1e-7f});
        amp_channel[i] = peak_amp;
        phase_block[i] = (arg(hilbert_output[i]) / (2.0 * M_PI)) + 0.5;
    }
    
    // Then get raw pitch information
    for(int n = 0; n < channel.size(); n += step) {
        for(int i = 0; i < block_size; i++) {
            int idx = jmap(i, 0, block_size,  n - block_size,  n);
            block[i] = idx < 0 ?  history[idx + block_size] : channel[idx];
            block[i] /= idx < 0 ?  amp_history[idx + block_size] : amp_channel[idx];
        }
        
        float frequency = pya.probabilistic_pitch(block, 44100.0f);
        //float frequency = pitch::swipe<float>(block, sample_rate);
        
        if(!std::isfinite(frequency) || frequency == -1) frequency = 0.0f;
        
        for(int s = 0; s < step; s++) {
            freq_buffer[n + s] = frequency;
        }
        
        for(int i = 0; i < step; i++) {
            amp_delay_line.push_back(channel[n + i]);
            channel[n + i] = amp_delay_line[0];
            amp_delay_line.pop_front();
        }
        
    }
    
    for(int i = 0; i < step; i++) {
        output[i] += out_history[i];
    }
    
    for(int n = 0; n < freq_buffer.size(); n += step) {
        
        for(int i = 0; i < block_size; i++) {
            int idx = jmap(i, 0, block_size,  n - block_size,  n);
            block[i] = idx < 0 ?  history[idx + block_size] : channel[idx];
        }
        
        float frequency = freq_buffer[n];
        
        int window_idx = n / step;
        
        for(auto& filter : svf[window_idx]) {
            filter.setCutoffFrequency(std::max(frequency, 80.0f));
        }
    
        for(auto& [harmonic, amplitude, phase] : harmonics) {
            if(harmonic == 0 || amplitude == 0) continue;
            
            for(int i = 0; i < block_size; i++) {
                
                float filter_out = block[i];
                
                for(auto& filter : svf[window_idx]) filter_out = filter.processSample(0, filter_out);
                
                filtered_peak *= peak_release_scalar;
                filtered_peak = std::max({filtered_peak, abs(filter_out), 1e-8f});
                
                
                int lower = harmonic;
                int upper = lower + 1;
                
                float mix = harmonic - (int)harmonic;
                
                float offset_1 = (lower - 1 & 1) - (((lower & 3) == 0) * 2);
                float offset_2 = (upper - 1 & 1) - (((upper & 3) == 0) * 2);

                float compression = jmap(jmap(compression_amt, 0.95f, 1.0f), 1.0f, std::max(filtered_peak, 1e-5f));
                
                
                float in_value = acos(std::clamp(filter_out / compression, -1.0f, 1.0f));

                
                float out_1 = (cos(in_value * (float)lower) + offset_1) * compression * amplitude;
                float out_2 = (cos(in_value * (float)upper) + offset_2) * compression * amplitude;

                current_window[i] = jmap(mix, out_1, out_2) * 2.0f;
                
                
                //std::cout << filtered_peak << std::endl;
                
            }
        }
        
        apply_window(current_window, hanning_window);
        
        
        for(int i = 0; i < block_size; i++) {
            int idx = n + i;
            
            if(idx >= block_size) {
                out_history[idx - block_size] = current_window[i];
            }
            else {
                output[idx] += current_window[i];
            }
        }
        
        
        for(int i = 0; i < step; i++) {
            delay_line.push_back(channel[n + i]);
            channel[n + i] = delay_line[0];
            delay_line.pop_front();
        }
        
        for(int i = 0; i < step; i++) {
            //output[n + i] += channel[n + i];
        }
        
        
        /*
         if(n % step == 0)  {
         averager.add(frequency);
         delta_average.add((delta < 1.0f && delta) ? (1.0f / delta) : delta);
         }
         
         
         // Transient detection
         float amp_delta = abs(amp_channel[n > 0 ? n - 1 : 0] - amp_channel[n]) * 140.0f;
         float freq_delta = delta_average.avg();
         
         // TODO: experiment with weighting these conditions
         bool out_of_range = frequency > 1000 || frequency < 50;
         bool wobbly = freq_delta > 1.25;
         bool divergent = (diff > 2.5f || 1.0f / diff < 2.5f) && avg;
         
         if(out_of_range || wobbly) {
         amp_delta += 1.0f;
         } */
        
        
        /*
         for(auto& [harmonic, amplitude, phase] : harmonics) {
         if(harmonic == 0 || amplitude == 0) continue;
         
         int closest_harmonic = round(harmonic);
         
         while(phase > 1.0f) phase -= 1.0f;
         while(phase < 0.0f) phase += 1.0f;
         
         int offset = (closest_harmonic - 1 & 1) - (((closest_harmonic & 3) == 0) * 2);
         
         float distortion = (offset + cos(acos(tanh(delayed / delayed_amp)) * (float)closest_harmonic)) * amplitude * delayed_amp;
         
         
         
         float mix = harmonic - (int)harmonic;
         //distorted += isfinite(distortion) ? distortion : 0.0f;
         
         if(disharmonic) {
         //distorted += hilbert_output[n].real() * sin(phase * 2.0 * M_PI) + hilbert_output[n].imag() * cos(phase * 2.0 * M_PI);
         distorted = delayed;
         //distorted += cos(disharmonic_block.getSample(0, n) * 2.0 * M_PI) * abs(hilbert_output[n]);
         
         phase += (frequency * harmonic) / sample_rate;
         
         sine += cos(phase * M_PI * 2.0f) * delayed_amp * amplitude * 3.0f;
         
         }
         else {
         
         distorted = delayed;
         
         int rounded = harmonic;
         
         float out_1 = cos(phase * M_PI * 2.0f * (float)rounded) * delayed_amp * amplitude * 3.0f;
         float out_2 = cos(phase * M_PI * 2.0f * ((float)rounded + 1.0f)) * delayed_amp * amplitude * 3.0f;
         
         float filter_out = delayed * delayed_amp;
         
         for(auto& filter : svf) filter_out = filter.processSample(0, filter_out);
         
         filtered_peak *= peak_release_scalar;
         filtered_peak = std::max({filtered_peak, abs(filter_out), 1e-8f});
         
         sine += cos(acos(filter_out / filtered_peak) * (float)closest_harmonic) * filtered_peak; //jmap(mix, out_1, out_2);
         
         phase += frequency / sample_rate;
         
         }
         
         
         
         //if(!std::isfinite(sine)) sine = 0.0f;
         //sine = std::clamp(sine, -1.0f, 1.0f);
         
         
         
         //distorted += tanh((delayed / delayed_amp) * 2.0f);
         
         
         //if(!(closest_harmonic & 1)) distorted *= 2.0f;
         
         
         }*/
        
        // TODO:: apply filter on sine
        
        //sine /= harmonics.size();
        
        //output[n] = jmap<float>(transient, sine, distorted);
        //output[n] = distorted + (sine * (1.0f - transient));
        // + distorted;
        //output[n] = transient;
        //output[n] = jmap<float>(transient, sine, 0.0f);
        
        
        
        last_frequency = frequency;
        
        
    }
    
    //std::copy(output.begin(), output.end(), out_history.begin());
    std::copy(channel.begin(), channel.end(), history.begin());
    std::copy(amp_channel.begin(), amp_channel.end(), amp_history.begin());
    
}
