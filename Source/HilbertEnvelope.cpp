#include "HilbertEnvelope.hpp"
#include <complex>
#include <random>
#include <algorithm>

HilbertEnvelope::HilbertEnvelope(ProcessSpec& spec, int bands, int oversample_factor) {
    oversamp = oversample_factor;
    num_channels = spec.numChannels;
    num_bands = bands;
    
    alpha_release = std::exp ((-2.0 * MathConstants<double>::pi * 1000.0f / spec.sampleRate) / release_ms);
    
    states.resize(num_bands, std::vector<HilbertState>(num_channels));
    
    clear();
    GetAntiDenormalTable(adtab, 16);
    
    
}

void HilbertEnvelope::GetAntiDenormalTable(float* d, int size) {
    static auto generator = std::default_random_engine();  // Generates random integers
    static auto distribution = std::uniform_real_distribution<float>(-0.999, +0.999);
    auto gen = []() {
        return distribution(generator);
    };
    std::generate(d, d + size, gen);
    
    for (int i = 0; i < size; i++) {
        d[i] = abs(d[i]) + 0.9f;
        for (int j = 1; j < size; j += 2) d[j] *= -1.0f;
        d[i] += 1.1f;
        d[i] *= 0.5f * ANTI_DENORMAL_float;
    }
}

void HilbertEnvelope::clear() {
    for(auto& channel : states) {
        for(auto& [xn1, yn1, s, adidx, release_state] : channel) {
            xn1 = 0.0f;
            yn1 = 0.0f;
            std::fill(s.begin(), s.end(), 0.0);
            adidx = 0;
            release_state = 0.0f;
        }
    }
}

void HilbertEnvelope::process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, int num_samples) {
    
    num_samples /= oversamp;
    
    for (int b = 0; b < num_bands; b++) {
        for (int ch = 0; ch < num_channels; ch++) {
            auto& [xn1, yn1, s, adidx, release_state] = states[b][ch];
            
            float adn[2];
            adn[0] = adtab[adidx];
            adn[1] = adtab[adidx + 1];
            adidx = (adidx + 2) & 0xe;
            float xa, xb, adin;
            
            for(int i = 0; i < num_samples; i++) {
                auto* input = in_bands[b].getChannelPointer(ch);
                auto* output = out_bands[b].getChannelPointer(ch);
                auto inverse = inverse_bands[b].getChannelPointer(ch);
                
                float r_out, i_out;
                adin = input[i * oversamp] + adn[i & 1];
                
                // out1 filter chain: 8 allpasses + 1 unit delay
                xa = s[1] - 0.999533593f * adin;
                s[1] = s[0];
                s[0] = adin + 0.999533593f * xa;
                xb = s[3] - 0.997023120f * xa;
                s[3] = s[2];
                s[2] = xa + 0.997023120f * xb;
                xa = s[5] - 0.991184054f * xb;
                s[5] = s[4];
                s[4] = xb + 0.991184054f * xa;
                xb = s[7] - 0.975597057f * xa;
                s[7] = s[6];
                s[6] = xa + 0.975597057f * xb;
                xa = s[9] - 0.933889435f * xb;
                s[9] = s[8];
                s[8] = xb + 0.933889435f * xa;
                xb = s[11] - 0.827559364f * xa;
                s[11] = s[10];
                s[10] = xa + 0.827559364f * xb;
                xa = s[13] - 0.590957946f * xb;
                s[13] = s[12];
                s[12] = xb + 0.590957946f * xa;
                xb = s[15] - 0.219852059f * xa;
                s[15] = s[14];
                s[14] = xa + 0.219852059f * xb;
                r_out = s[32];
                s[32] = xb;
                
                // out2 filter chain: 8 allpasses
                xa = s[17] - 0.998478404f * adin;
                s[17] = s[16];
                s[16] = adin + 0.998478404f * xa;
                xb = s[19] - 0.994786059f * xa;
                s[19] = s[18];
                s[18] = xa + 0.994786059f * xb;
                xa = s[21] - 0.985287169f * xb;
                s[21] = s[20];
                s[20] = xb + 0.985287169f * xa;
                xb = s[23] - 0.959716311f * xa;
                s[23] = s[22];
                s[22] = xa + 0.959716311f * xb;
                xa = s[25] - 0.892466594f * xb;
                s[25] = s[24];
                s[24] = xb + 0.892466594f * xa;
                xb = s[27] - 0.729672406f * xa;
                s[27] = s[26];
                s[26] = xa + 0.729672406f * xb;
                xa = s[29] - 0.413200818f * xb;
                s[29] = s[28];
                s[28] = xb + 0.413200818f * xa;
                i_out = s[31] - 0.061990080f * xa;
                s[31] = s[30];
                s[30] = xa + 0.061990080f * i_out;
                
                float out_value = std::abs(std::complex(r_out, i_out));
                
                if(out_value < release_state) {
                    out_value = out_value + alpha_release * (release_state - out_value);
                }
                
                release_state = out_value;
                
                for(int o = 0; o < oversamp; o++) {
                    output[i * oversamp + o] = 1.0f / out_value;
                    inverse[i * oversamp + o] = out_value;
                }
            }
        }
    }
}
