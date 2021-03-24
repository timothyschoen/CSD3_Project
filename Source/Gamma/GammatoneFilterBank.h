#pragma once

#include "GammatoneFilter.h"
#include <vector>
#include <memory>
#include <JuceHeader.h>

class GammatoneFilterBank
{
public:    
    
    GammatoneFilterBank(float rate, int block_size, int channels, float filter_q = 8, float filter_min_width = 125);
    
    ~GammatoneFilterBank();
    
    int init_with_overlap(float low_freq, float high_freq, float overlap);
    float init_with_num_filters(float low_freq, float high_freq, unsigned n_filters);
    
    void add_filter(unsigned _order, float _freq, float _erb);

    void remove_filters();

    int get_num_filters();
    
    void process(dsp::AudioBlock<float> in_buffer, std::vector<dsp::AudioBlock<float>> out_buffer);
    
    std::vector<std::vector<GammatoneFilter*>> filters;            // Hold the filters in the Bank.
private:

    float sample_rate;							// Default sampling freq for adding filters
    float q;
    float min_width;
    float bw;
    int block_size;
    int num_channels;
};
