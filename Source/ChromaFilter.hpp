//
//  main.cpp
//  WindowedSineFit
//
//  Created by Timothy Schoen on 24/08/2021.
//

#pragma once
#include "MovingAverage.hpp"
#include "Chroma/Chromagram.h"
#include "Filterbanks/GammatoneFilter.hpp"


#include <JuceHeader.h>

#include <math.h>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <deque>

using Sample = float;
using Samples = std::vector<float>;

struct ChromaFilter
{
    static constexpr int block_size = 2048;
    static constexpr float sample_rate = 44100.0f;
    
    
    static constexpr int min_width = 3;
    static constexpr int order = 6;
    static constexpr float q = 12;
    
    std::vector<float> frequencies;
    
    int skip_size = 1;
    
    int start = 43, end = 79;
    
    int m_start = 20;
    
    ChromaFilter() {
        
        int midi_start = m_start;
        int midi_end = 110;
        
        if(midi_start >= midi_end) {
            midi_start = midi_end - 12;
        }
        
        num_notes = midi_end - midi_start;
        frequencies.resize(num_notes);
        
        for (int m = midi_start; m < midi_end; m++)
        {
            frequencies[m - m_start] = pow(2, (m - 69.0f) / 12.0f) * 440.0f;
        }
        
        output_buffer.resize(num_notes / skip_size, Samples(block_size));
        
        for(int freq = 0; freq < num_notes; freq++) {
            
            float erb = (frequencies[freq] / q) + min_width;
            filters.add(new GammatoneFilter(sample_rate, block_size, order, frequencies[freq], erb));
            
            int current_latency = filters.getLast()->calculate_latency();
            latencies.push_back(current_latency);
            
        }
        
        // Total latency
        latency = *std::max_element(latencies.begin(), latencies.end());
        
        // Delay to add to each filter to sync them up
        for(auto& value : latencies) value = latency - value;
        
        for(auto& delay : latencies) {
            delays.add(new dsp::DelayLine<float>(latency));
            delays.getLast()->setDelay(delay);
            delays.getLast()->prepare({sample_rate, block_size, 1});
        }
    }
    
    
    std::vector<Samples> process(Samples channel) {
        
        for(int freq = start; freq < end; freq += skip_size) {
            int filter_idx = freq - m_start;
            int band_idx = (freq - start) / skip_size;
            
            jassert(filter_idx >= 0);
            jassert(band_idx >= 0);
            
            filters[filter_idx]->process(channel.data(), output_buffer[band_idx].data(), channel.size());
            
            for(auto& sample : output_buffer[band_idx]) {
                delays[filter_idx]->pushSample(0, sample);
                sample = delays[filter_idx]->popSample(0);
            }
        }
        
        return output_buffer;
    }
    
    int get_latency() {
        return latency;
    }
    
    void set_density(int density) {
        skip_size = density;
        output_buffer.resize(num_notes / skip_size, Samples(block_size, 0.0f));
    }
    
    void set_start(int new_start) {
        start = new_start;
        num_notes = end - start;
        
        output_buffer.resize(num_notes / skip_size, Samples(block_size, 0.0f));
    }
    
    void set_end(int new_end) {
        end = new_end;
        num_notes = end - start;
        
        output_buffer.resize(num_notes / skip_size, Samples(block_size, 0.0f));
    }
    
    
private:
    
    int num_notes = 36;

    std::vector<Samples> output_buffer;
    
    int latency = 800;
    std::vector<int> latencies;
    
    OwnedArray<GammatoneFilter> filters;
    OwnedArray<dsp::DelayLine<float>> delays;
};
