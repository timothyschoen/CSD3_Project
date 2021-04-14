#include <array>
#include <vector>

#include "Filterbank.hpp"

using ResonCoeffs = std::tuple< float, // cutoff
                                float, //gain;
                                float, //r;
                                float, //r_scale;
                                float, //c1;
                                float //c;
>;


struct ResonBands final : public Filterbank 
{
    std::vector<ResonCoeffs> filters;
    std::vector<std::vector<std::array<float, 2>>> filter_feedback_y;
    std::vector<std::array<float, 2>> filter_feedback_x;
    
    float sample_rate;
    int num_bands;
    int num_channels;
    
    
    ResonBands(ProcessSpec& spec);

    static inline float mtof(float midi_note) { return 440.0f * pow(2.0f, (midi_note - 69.0f) / 12.0f); };
    static inline float ftom(float freq)      { return  69.0f + (12.0f * log2(freq / 440.0f));          };
    
    
    void create_bands(int n_bands, std::pair<float, float> range = {80.0f, 20000.0}, float band_width = 1.0f, float g = 1.0f);
        
    
    void process(const AudioBlock<float>& input, std::vector<AudioBlock<float>>& output) override;
    
    float get_centre_freq(int idx) override {
        return std::get<0>(filters[idx]);
    }
    
    int get_num_filters() override {return num_bands; };
    
};
