//
//  main.cpp
//  WindowedSineFit
//
//  Created by Timothy Schoen on 24/08/2021.
//

#include "MovingAverage.hpp"
#include "Hilbert.hpp"
#include "Rate.hpp"

#include "PitchDetection/pitch_detection.h"
#include "DynamicFilter.hpp"
#include "ChromaFilter.hpp"

#include "ChebyshevTable.hpp"

#include <JuceHeader.h>

#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <deque>

using Sample = float;
using Samples = std::vector<float>;


struct MonoDistortion
{
    
    Thread* audio_thread = nullptr;
        
    IIRFilter downsample_filter;
    
    MonoDistortion();
    
    void process(Samples channel, Samples& output);
    
    void receive_message(const Identifier& id, float value, int idx);
    
    void mute(int idx);
    

    ChromaFilter chroma_filter = ChromaFilter(43, 79); // temporarily public
    
private:
    
    pitch_alloc::Mpm<float> pya = pitch_alloc::Mpm<float>(block_size);
    
    void process_block(Samples channel, Samples& output);
    void process_poly(Samples channel, Samples& output);
    
    static constexpr int block_size = 2048;
    static constexpr int step = 1024;
    
    static constexpr int overlap = 2;
    

    static constexpr int avg_window_1 = 512;
    static constexpr int avg_window_2 = 64;
    
    bool poly = true;
    
    int min_freq = 43, max_freq = 79;
    
    
    Samples block;
    Samples amp_channel;
    Samples amp_history;
    Samples freq_buffer;
    Samples current_window;
    Samples out_history;
    
    Samples input_buffer;
    Samples future_buffer;
    
    int fifo_idx = 0;
    int samples_ready = 0;
    
    std::vector<float> history;
    std::deque<float> delay_line;
    
    std::deque<float> chroma_delay;
    std::deque<float> amp_delay_line;
        
    std::vector<std::tuple<float, float, float>> harmonics = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    
    float sample_rate = 44100.0f;
    
    float compression_amt = 0.5;
    
    float peak_amp = 0.0f;
    float release_ms = 500.0f;
    float exp_factor = -2.0f * M_PI * 1000.0f / sample_rate;
    float peak_release_scalar = std::exp(exp_factor / release_ms);

    
    float filtered_peak = 0.0f;
    std::vector<float> poly_filtered_peak;
    
    std::vector<float> current_phase = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float last_frequency = 0.0f;
    float last_amplitude = 0.0f;
    
    bool disharmonic = true;
    
    std::array<std::array<dsp::StateVariableTPTFilter<float>, 4>, 6> svf;
    
    Hilbert hilbert;
    Rate rate_shifter;
    
    DynamicFilter dyn_filter;
    
    
};
