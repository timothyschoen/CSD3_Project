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
    
    int start, end;
    
    ChromaFilter(int midi_start, int midi_end) : start(midi_start), end(midi_end) {
        
        if(midi_start <= midi_end) {
            midi_start = midi_end - 12;
        }
        
        num_notes = midi_end - midi_start;
        frequencies.resize(num_notes);
        
        for (int m = midi_start; m < midi_end; m++)
        {
            frequencies[m - midi_start] = pow(2, (m - 69.0f) / 12.0f) * 440.0f;
        }
        
        output_buffer.resize(num_notes, Samples(block_size));
        
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
        
        for(int freq = 0; freq < num_notes; freq++) {
            filters[freq]->process(channel.data(), output_buffer[freq].data(), channel.size());
            
            for(auto& sample : output_buffer[freq]) {
                delays[freq]->pushSample(0, sample);
                sample = delays[freq]->popSample(0);
            }
        }
        
        return output_buffer;
    }
    
    int get_latency() {
        return latency;
    }
    
    
private:
    
    int num_notes = 36;

    std::vector<Samples> output_buffer;
    
    int latency = 800;
    std::vector<int> latencies;
    
    OwnedArray<GammatoneFilter> filters;
    OwnedArray<dsp::DelayLine<float>> delays;
};
