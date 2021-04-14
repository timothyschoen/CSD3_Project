#include "RMSEnvelope.hpp"
#include <complex>
#include <random>
#include <algorithm>

RMSEnvelope::RMSEnvelope(ProcessSpec& spec, int bands, int oversample_factor) {
    oversamp = oversample_factor;
    num_channels = spec.numChannels;
    num_bands = bands;
    
    rms_filters.ensureStorageAllocated(num_bands);
    
    for(int b = 0; b < num_bands; b++) {
        rms_filters.add(new BallisticsFilter<float>);
        rms_filters[b]->prepare(spec);
        rms_filters[b]->setAttackTime(2.0f);
        rms_filters[b]->setReleaseTime(300.0f);
        rms_filters[b]->setLevelCalculationType(BallisticsFilterLevelCalculationType::peak);
    }
    
}

void RMSEnvelope::process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, int num_samples) {
    
    for(int b = 0; b < num_bands; b++) {
        auto out_band = inverse_bands[b].getSubBlock(0, num_samples);
        rms_filters[b]->process(ProcessContextNonReplacing<float>(in_bands[b].getSubBlock(0, num_samples), out_band));
        
        for(int ch = 0; ch < num_channels; ch++) {
            auto* out_ptr = out_bands[b].getChannelPointer(ch);
            auto* inv_ptr = inverse_bands[b].getChannelPointer(ch);
            for(int n = 0; n < num_samples; n++) {
                // Leave a bit of headroom and prevent divide by 0
                out_ptr[n] = 0.8f / std::max(inv_ptr[n], 1e-7f);
            }
        }
    }
    
}
