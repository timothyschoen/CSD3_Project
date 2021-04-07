#pragma once

#include <JuceHeader.h>


const float ANTI_DENORMAL_float = 1e-15f;

// hilbert transformer: generates two orthogonal output signals
// max. phase error for f = 0.00015..0.49985fs is 0.017 degrees

using HilbertState = std::tuple<float, float, float[33], int>;

class HilbertAmplitude {
    
    int num_channels;
    int num_bands;
    int oversamp;
    
public:
    HilbertAmplitude(dsp::ProcessSpec& spec, int bands, int oversample_factor);
    
    void GetAntiDenormalTable(float* d, int size);
    
    void clear();
    
    void process(const std::vector<dsp::AudioBlock<float>>& in_bands, std::vector<dsp::AudioBlock<float>>& out_bands, std::vector<dsp::AudioBlock<float>>& inverse_bands, int num_samples);
    
private:
    std::vector<std::vector<HilbertState>> states;
    float adtab[16];
};
