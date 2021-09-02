//
//  main.cpp
//  WindowedSineFit
//
//  Created by Timothy Schoen on 24/08/2021.
//

#include "MovingAverage.hpp"


#include <JuceHeader.h>

#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <deque>

using Sample = float;
using Samples = std::vector<float>;


using WindowFunc = std::function<Sample(float)>;

static inline Sample no_window(float progress) {
    return 0.5;
}

static inline Sample hamming_window(float progress) {
    return 0.54 - 0.46 * cos(2.0 * M_PI * progress);
}

static inline Sample hanning_window(float progress) {
    return 0.5 - 0.5 * cos(2.0 * M_PI * progress);
}

static void apply_window(std::vector<Sample>& samples, WindowFunc window_func) {
    for (int i = 0; i < samples.size(); i++) {
        const auto progress = (i - 1.0) / (samples.size() - 1.0);
        samples[i] *= window_func(progress);
    }
}


struct DynamicFilter
{
    
    static constexpr int num_voices = 6;
    
    static constexpr int block_size = 2048;
    static constexpr int step = 1024;
    static constexpr float sample_rate = 44100.0f;
    
    static constexpr float release_ms = 500.0f;
    static constexpr float exp_factor = -2.0f * M_PI * 1000.0f / sample_rate;
    float peak_release_scalar = std::exp(exp_factor / release_ms);
    float filtered_peak = 0.0f;
    
    static constexpr float block_exp_factor = -2.0f * M_PI * 1000.0f / (sample_rate / block_size);
    static constexpr float block_release = 2200.0f;
    static constexpr float block_attack = 300.0f;
    float block_release_scalar = std::exp(block_exp_factor / block_release);
    float block_attack_scalar = std::exp(block_exp_factor / block_attack);
    std::vector<float> freq_decay;
    
    std::vector<Samples> output_buffer;
    
    Samples block;
    Samples amp_channel;
    Samples freq_buffer;
    std::vector<Samples> current_window;
    std::vector<Samples> out_history;
    Samples history;
    
   
    
            
    DynamicFilter() {
        block.resize(block_size);
        freq_buffer.resize(block_size);
        history.resize(block_size, 0.0f);
        out_history.resize(num_voices, Samples(block_size, 0.0f));
        current_window.resize(num_voices, Samples(block_size, 0.0f));
        
        freq_decay.resize(1<<13, 0.0f);
        
        output_buffer.resize(num_voices, Samples(block_size, 0.0f));
        
        for(auto& group : svf) {
            for(auto& filter : group) {
                filter.prepare({sample_rate, block_size, 8});
                filter.setType(StateVariableTPTFilterType::bandpass);
                filter.setResonance(0.96f);
            }
        }
    }
    
    std::vector<Samples> process(Samples channel) {
        int fft_size = 1<<11;
        
        auto fft = dsp::FFT(log2(fft_size));
        
        for(int g = 0; g < output_buffer.size(); g++) {
            std::fill(output_buffer[g].begin(), output_buffer[g].end(), 0.0f);
            std::fill(current_window[g].begin(), current_window[g].end(), 0.0f);
        }
        

       
        auto freq_domain = channel;
        for(auto& sample : freq_domain)  {
            filtered_peak *= peak_release_scalar;
            filtered_peak = std::max({filtered_peak, abs(sample), 1e-8f});
            sample /= filtered_peak;
        }
        
        apply_window(freq_domain, hamming_window);
        freq_domain.resize(2 * fft_size, 0.0f);
        
        fft.performFrequencyOnlyForwardTransform(freq_domain.data());
        
        std::vector<std::pair<float, float>> peaks;
        
        for(int bin = 2; bin < fft_size / 2; bin++)
        {
            freq_domain[bin] /= fft_size;
            
            
            float frequency = (float)bin * (sample_rate / (fft_size / 2.0f));
            

            /*
            float freq = bin * ((float)sample_rate / (float)(1<<11));
            if(freq_domain[bin] > 0.01) {
                peaks.push_back({freq, freq_domain[bin]});
            } */
            
            float cte = freq_domain[bin] > freq_decay[bin] ? block_attack_scalar : block_release_scalar;
            
            freq_decay[bin] = jmap(cte, freq_domain[bin], freq_decay[bin]);
            peaks.push_back({frequency, freq_decay[bin]});
            
        }
                
        // Sort by amplitude
        std::sort(peaks.begin(), peaks.end(),
                  [](const std::pair<float, float> & a, const std::pair<float, float> & b) -> bool
                  {
            return a.second > b.second;
        });
        
        
          
          for (int i = 0; i < std::min<int>(num_voices, peaks.size()); i++) {
              float weight = peaks[i].second;
              float frequency = peaks[i].first;

              for(int bin = i + 1; bin < peaks.size(); bin++) {
                  float comp_weight = peaks[bin].second;
                  float comp_frequency = peaks[bin].first;
                  
                  float ratio = comp_frequency / frequency;
                  if(ratio > 0.9f && ratio < 1.1f) {
                      peaks.erase(peaks.begin() + bin);
                      weight += comp_weight;
                      freq_decay[i] += comp_weight;
                  }
              }
              
              peaks[i].second = weight;
              
          }

        
        if(peaks.size() > num_voices) peaks.resize(num_voices);

        //if(!peaks.size()) return;
        
        // Sort by frequency
        std::sort(peaks.begin(), peaks.end(),
                  [](const std::pair<float, float> & a, const std::pair<float, float> & b) -> bool
                  {
            return a.first < b.first;
        });
        
        //peaks.erase(peaks.begin());
        
        //std::cout << peaks[0].first << std::endl;
        
        //std::cout << peaks[0].first << std::endl;
        //std::cout << peaks.size() << std::endl;
        
        for(int g = 0; g < peaks.size(); g++) {
            for(int i = 0; i < step; i++) {
                output_buffer[g][i] += out_history[g][i];
                out_history[g][i] = 0.0f;
            }
        }

        
        for(int n = 0; n < channel.size(); n += step) {
            for(int g = 0; g < peaks.size(); g++) {
                std::fill(current_window[g].begin(), current_window[g].end(), 0.0f);
            }
            
            for(int i = 0; i < block_size; i++) {
                int idx = jmap(i, 0, block_size,  n - step,  n + step);
                block[i] = idx < 0 ?  history[idx + block_size] : channel[idx];
            }
            
            for(int g = 0; g < peaks.size(); g++) {
                float frequency = peaks[g].first;
                    for(auto& filter : svf[g]) {
                        filter.setCutoffFrequency(std::clamp(frequency, 80.0f, 20000.0f));
                    }
                
                    for(int i = 0; i < block_size; i++) {
                        
                        int window_idx = n / step;
                        
                        float filter_out =  block[i];// jmap<float>(std::clamp(peaks[g].second, 0.0f, 1.0f), block[i] * 0.02f, block[i]);
                 
                        for(auto& filter : svf[g]) filter_out = filter.processSample(window_idx, filter_out);

                        current_window[g][i] += filter_out * 3.0f;
                        
                    }
            }
            
            for(int g = 0; g < peaks.size(); g++) {
                
                apply_window(current_window[g], hanning_window);
            
                for(int i = 0; i < block_size; i++) {
                    int idx = n + i;
                    
                    if(idx >= block_size) {
                        out_history[g][idx - block_size] += current_window[g][i];
                    }
                    else {
                        output_buffer[g][idx] += current_window[g][i];
                    }
                }
            }
        }
        
        std::copy(channel.begin(), channel.end(), history.begin());
        
        return output_buffer;
    }


private:
    
    std::array<std::array<dsp::StateVariableTPTFilter<float>, 4>, num_voices> svf;
    
};
