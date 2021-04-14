#pragma once
#include <JuceHeader.h>


// Interface class for envelope followers
struct EnvelopeFollower {
    
    virtual ~EnvelopeFollower() {};

    virtual void process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, int num_samples) = 0;
    
};
