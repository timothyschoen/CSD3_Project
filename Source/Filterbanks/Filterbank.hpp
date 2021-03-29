#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>


struct Filterbank
{
    
    virtual ~Filterbank () {};
    
    virtual void process(const dsp::AudioBlock<float>& in_buffer, std::vector<dsp::AudioBlock<float>>& out_buffer) {};
    
    virtual float get_centre_freq(int idx) {return 0; };
};
