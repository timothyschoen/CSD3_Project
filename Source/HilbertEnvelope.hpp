#pragma once
#include "EnvelopeFollower.hpp"
#include <array>
#include <JuceHeader.h>


const float ANTI_DENORMAL_float = 1e-15f;

// hilbert transformer: generates two orthogonal output signals
// max. phase error for f = 0.00015..0.49985fs is 0.017 degrees

using HilbertState = std::tuple<float, float, std::array<float, 33>, int, float>;

class HilbertEnvelope : public EnvelopeFollower {
    
    int num_channels;
    int num_bands;
    int oversamp;
    
    float release_ms = 80.0f;
    float alpha_release;
    
public:
    HilbertEnvelope(ProcessSpec& spec, int bands, int oversample_factor);
    
    void GetAntiDenormalTable(float* d, int size);
    
    void clear();
    
    void process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, int num_samples) override;
    
private:
    std::vector<std::vector<HilbertState>> states;
    float adtab[16];
};
