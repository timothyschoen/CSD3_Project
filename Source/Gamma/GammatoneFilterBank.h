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
    
    unsigned InitWithFreqRangeOverlap(float _lowFreq, float _highFreq, float _overlap);
    
    float InitWithFreqRangeNumFilters(float _lowFreq, float _highFreq, unsigned _numFilters);
    
    void AddFilter(unsigned _order, float _freq, float _erb);

    void RemoveFilters();

    int GetNumFilters();
    
    void process(dsp::AudioBlock<float> inBuffer, std::vector<dsp::AudioBlock<float>> outBuffer);
    
    std::vector<std::vector<GammatoneFilter*>> filters;            // Hold the filters in the Bank.
private:

    float sample_rate;							// Default sampling freq for adding filters
    float q;
    float min_width;
    float bw;
    int block_size;
    int num_channels;
};
