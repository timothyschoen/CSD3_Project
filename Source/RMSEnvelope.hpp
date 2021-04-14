#pragma once
#include "EnvelopeFollower.hpp"
#include <array>
#include <JuceHeader.h>


class RMSEnvelope : public EnvelopeFollower {
    
    int num_channels;
    int num_bands;
    int oversamp;
    
    OwnedArray<BallisticsFilter<float>> rms_filters;
    
public:
    RMSEnvelope(ProcessSpec& spec, int bands, int oversample_factor);
    
    void process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, int num_samples) override;

};
