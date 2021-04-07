#include <array>
#include <vector>

#include "ResonBands.hpp"


ResonBands::ResonBands(dsp::ProcessSpec& spec){
    sample_rate = spec.sampleRate;
    num_channels = spec.numChannels;
};


void ResonBands::create_bands(int n_bands, std::pair<float, float> range, float band_width, float g) {
    num_bands = n_bands;
    filter_feedback_y.resize(num_bands, std::vector<std::array<float, 2>>(num_channels));
    filter_feedback_x.resize(num_channels);
    
    filters.clear();
    filters.resize(num_bands);
    
    float midi_low = ftom(range.first);
    float midi_high = ftom(range.second);
    
    float midi_diff = midi_high - midi_low;
    
    for(int i = 0; i < num_bands; i++) {
        float x = (midi_diff * ((i - 1.0f) / (num_bands - 1.0f)) / 1.0f) + midi_low;
        
        std::get<0>(filters[i]) = mtof(x);
    }
    
    float q = (std::get<0>(filters[0]) / ((std::get<0>(filters[1]) - std::get<0>(filters[0])) / 2.0f) + 1.0f) * (1.0f / band_width);
    
    for(int i = 0; i < num_bands; i++) {
        float twopi_over_sr = MathConstants<float>::twoPi / sample_rate;
        auto& [cutoff, gain, r, r_scale, c1, c2] = filters[i];
        
        gain = g;
        float bw = cutoff * (1.0f / q);
        r = exp(-(twopi_over_sr * bw));
        r_scale = 1.0f - r;
        c1 = 2.0f * r * cos(cutoff * twopi_over_sr);
        c2 = - (r * r);
    }
    
}


void ResonBands::process(const dsp::AudioBlock<float>& input, std::vector<dsp::AudioBlock<float>>& output) {
    jassert(output.size() == num_bands);
    
    int num_samples = input.getNumSamples();
    
    for(int ch = 0; ch < input.getNumChannels(); ch++) {
        auto* in_ptr = input.getChannelPointer(ch);
        
        for(int i = 0; i < num_bands; i++) {
            auto& [cutoff, gain, r, r_scale, c1, c2] = filters[i];
            auto* out_ptr = output[i].getChannelPointer(ch);
            
            for(int n = 0; n < num_samples; n++) {
                // TODO: use circular buffer
                double ym1 = n >= 1 ? out_ptr[n-1] : filter_feedback_y[i][ch][n + 1];
                double ym2 = n >= 2 ? out_ptr[n-2] : filter_feedback_y[i][ch][n];
                double xm2 = n >= 2 ? in_ptr[n-2]  : filter_feedback_x[ch][n];
                
                out_ptr[n] = r_scale * gain * (in_ptr[n] - r * xm2) + c1 * ym1 + c2 * ym2;
            }
            
            filter_feedback_y[i][ch][0] = out_ptr[num_samples - 2];
            filter_feedback_y[i][ch][1] = out_ptr[num_samples - 1];
        }
        
        filter_feedback_x[ch][0] = in_ptr[num_samples - 2];
        filter_feedback_x[ch][1] = in_ptr[num_samples - 1];
    }
}
