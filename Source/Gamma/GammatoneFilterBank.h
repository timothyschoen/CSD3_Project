#pragma once

#include "GammatoneFilter.h"
#include <vector>
#include <memory>
#include <JuceHeader.h>

class GammatoneFilterBank
{
public:    
    
    GammatoneFilterBank(double rate, int block_size, int channels, float filter_q = 8, float filter_min_width = 125);
    
    ~GammatoneFilterBank();
    
    unsigned InitWithFreqRangeOverlap(double _lowFreq, double _highFreq, double _overlap);
    
    double InitWithFreqRangeNumFilters(double _lowFreq, double _highFreq, unsigned _numFilters);
    
    void AddFilter(unsigned _order, double _freq, double _erb);

    void RemoveFilters();

    int GetNumFilters();
    
    void process(dsp::AudioBlock<float> inBuffer, std::vector<dsp::AudioBlock<float>> outBuffer);
    
    std::vector<std::vector<GammatoneFilter*>> filters;            // Hold the filters in the Bank.
private:

    double sample_rate;							// Default sampling freq for adding filters
    float q;
    float min_width;
    float bw;
    int block_size;
    int num_channels;
};
