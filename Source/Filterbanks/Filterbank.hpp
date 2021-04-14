#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>


struct Filterbank
{
    
    virtual ~Filterbank () {};
    
    virtual void process(const AudioBlock<float>& in_buffer, std::vector<AudioBlock<float>>& out_buffer) {};
    
    virtual float get_centre_freq(int idx) {return 0; };
    
    virtual int get_num_filters() {return 0; };
};
