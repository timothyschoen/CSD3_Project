#pragma once

#include <vector>

/*
 Source: https://www.pdn.cam.ac.uk/other-pages/cnbh/files/publications/SVOSAnnexC1988.pdf
 */
class GammatoneFilter
{
public:
    
    GammatoneFilter(double sample_rate, int block_size, unsigned filter_order, float center_freq, float band_width);
    
    ~GammatoneFilter();
    
    void process(const float* inBuffer, float* outBuffer, int num_samples);
    
    float get_centre_freq();
    
    
private:
    
    double sample_rate;              // Keep the sampling rate at which audio samples were taken
    unsigned order;                   // Keep the filter order
    double b;                         // scale param of gamma distribution
    double an;                        // filter impluse response proportinality constant
    double cn;                        // constant for bandwidth calculation
    double f0;                        // center freq in Hz (also freq of impulse response tone)
    double cos_phase_increment;       // phase increment of the filter
    double sin_phase_increment;       // phase increment of the filter
    double eq_constant;
    float* prev_z_real;               // store previous samples between audio buffers
    float* prev_z_imag;
    float* prev_w_real;
    float* prev_w_imag;
    
    float* temp_buffer_real;
    float* temp_buffer_imag;

    
    std::vector<float> cos_phase;
    std::vector<float> sin_phase;
    
    float sinphase = 0.0;
    float cosphase = 1.0;
    
    std::vector<float> z_real;
    std::vector<float> z_imag;
    std::vector<float> negated_sin;
    
    float last_cos = 1;
    float last_sin = 0;
    
    int read_pos = 0;
    
    //precomputed using python3's builtin bignums
    inline static double an_table[] =
    {
        0, //there is no zero order filter, but pad this for ease of access
        3.141592653589793,
        1.5707963267948966,
        1.1780972450961724,
        0.9817477042468103,
        0.8590292412159591,
        0.7731263170943632,
        0.7086991240031661,
        0.65807775800294,
        0.6169478981277563,
        0.5826730148984365,
        0.5535393641535147,
        0.5283784839647185,
        0.5063627137995219,
        0.48688722480723273,
        0.4694983953498315,
        0.45384844883817044,
        0.43966568481197754,
        0.4267343411410371,
        0.414880609442675,
        0.4039626986678677,
        0.393863631201171,
        0.38448592569638124,
        0.3757476092032817,
        0.3675791829162538,
        0.35992128327216516,
        0.35272285760672184,
        0.3459397257296695,
        0.33953343451245344,
        0.3334703374675882,
        0.3277208488905608,
    };
};
