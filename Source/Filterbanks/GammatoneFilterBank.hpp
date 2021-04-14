#pragma once

#include "Filterbank.hpp"
#include "GammatoneFilter.hpp"
#include <vector>
#include <memory>
#include <JuceHeader.h>

class GammatoneFilterBank final : public Filterbank 
{
public:    
    
    GammatoneFilterBank(ProcessSpec& spec);
    
    ~GammatoneFilterBank();
    
    int init_with_overlap(float low_freq, float high_freq, float overlap);
    float init_with_num_filters(float low_freq, float high_freq, unsigned n_filters);
    
    void add_filter(unsigned _order, float _freq, float _erb);

    void remove_filters();

    int get_num_filters() override;
    
    void process(const AudioBlock<float>& in_buffer, std::vector<AudioBlock<float>>& out_buffer) override;
    
    float get_centre_freq(int idx) override;
    
    std::vector<std::vector<std::unique_ptr<GammatoneFilter>>> filters;            // Hold the filters in the Bank.
private:

    float sample_rate;							// Default sampling freq for adding filters
    float q = 8;
    float min_width = 125;
    float bw;
    int block_size;
    int num_channels;
};
