/*
 * \class GammatoneFilter
 *
 * \brief Definition of GammatoneFilter class.
 *
 * Class to implement a Gammatone Digital Filter.
 * Equation numnber throughout this code refer to the following paper:
 * Implementing a GammaTone Filter Bank
 * Annex C of the SVOS Final Report (Part A: The Auditory Filter Bank)
 * John Holdsworth, Ian Nimmo-Smith, Roy Patterson, Peter Rice
 * 26th February 1988
 *
 * \authors 3DI-DIANA Research Group (University of Malaga), in alphabetical order: M. Cuevas-Rodriguez, C. Garre, D. Gonzalez-Toledo, E.J. de la Rubia-Cuestas, L. Molina-Tanco ||
 * Coordinated by , A. Reyes-Lecuona (University of Malaga) and L.Picinali (Imperial College London) ||
 * \b Contact: areyes@uma.es and l.picinali@imperial.ac.uk
 *
 * \b Contributions: (additional authors/contributors can be added here)
 * \b The gammatone filter was implemented by Michael Krzyzaniak: m.krzyzaniak@surrey.ac.uk
 *
 * \b Project: 3DTI (3D-games for TUNing and lEarnINg about hearing aids) ||
 * \b Website: http://3d-tune-in.eu/
 *
 * \b Copyright: University of Malaga and Imperial College London - 2018
 *
 * \b Licence: This copy of 3dti_AudioToolkit is licensed to you under the terms described in the 3DTI_AUDIOTOOLKIT_LICENSE file included in this distribution.
 *
 * \b Acknowledgement: This project has received funding from the European Union's Horizon 2020 research and innovation programme under grant agreement No 644051
 */

#define _USE_MATH_DEFINES // TODO: Test in windows! Might also be problematic for other platforms??
#include <JuceHeader.h>
#include <cmath>
#include "GammatoneFilter.hpp"

#define M_TWO_PI (2 * M_PI)



//////////////////////////////////////////////
GammatoneFilter::GammatoneFilter(double rate, int block_size, unsigned filter_order, float center_freq, float band_width)
{
    order = filter_order;
    
    prev_z_real = new float[order]();
    prev_z_imag = new float[order]();
    prev_w_real = new float[order]();
    prev_w_imag = new float[order]();
    
    temp_buffer_real = new float[order]();
    temp_buffer_imag = new float[order]();
    
    an = an_table[filter_order];
    cn = 2.0f * sqrt(pow(2, 1.0/(double)filter_order) - 1.0f);
    
    
    f0 = center_freq;
    sample_rate = rate;
    
    double phase_increment = f0 * M_TWO_PI / sample_rate;
    cos_phase_increment = cos(phase_increment);
    sin_phase_increment = sin(phase_increment);
    
    b = band_width / an;
    eq_constant = 1.0f - exp(-M_TWO_PI * b / sample_rate);
    
    cos_phase.resize(block_size, 0.0f);
    sin_phase.resize(block_size, 0.0f);
    negated_sin.resize(block_size, 0.0f);
    z_real.resize(block_size, 0.0f);
    z_imag.resize(block_size, 0.0f);
    
}

//////////////////////////////////////////////
GammatoneFilter::~GammatoneFilter()
{
    if(prev_z_real) delete[] prev_z_real;
    if(prev_z_imag) delete[] prev_z_imag;
    if(prev_w_real) delete[] prev_w_real;
    if(prev_w_imag) delete[] prev_w_imag;
    if(temp_buffer_real) delete[] temp_buffer_real;
    if(temp_buffer_imag) delete[] temp_buffer_imag;
}

//////////////////////////////////////////////
void GammatoneFilter::process(const float* inBuffer, float* outBuffer, int num_samples)
{
    
    for (int k = 0; k < num_samples; k++)
    {
        float old_cos_phase = k == 0 ? last_cos : cos_phase[k-1];
        float old_sin_phase = k == 0 ? last_sin : sin_phase[k-1];
        
        cos_phase[k] = cos_phase_increment * old_cos_phase + sin_phase_increment * old_sin_phase;
        sin_phase[k] = cos_phase_increment * old_sin_phase - sin_phase_increment * old_cos_phase;
    }
    
    last_cos = cos_phase[num_samples - 1];
    last_sin = sin_phase[num_samples - 1];
    
    FloatVectorOperations::negate(sin_phase.data(), sin_phase.data(), num_samples);
    
    FloatVectorOperations::multiply(z_real.data(), cos_phase.data(), inBuffer, num_samples);
    FloatVectorOperations::multiply(z_imag.data(), sin_phase.data(), inBuffer, num_samples);
    
    for (int k = 0; k < num_samples; k++)
    {
        for (int n = 0; n < order; n++)
        {
            prev_w_real[n] = z_real[k] = (z_real[k] - prev_w_real[n]) * eq_constant + prev_w_real[n];
            prev_w_imag[n] = z_imag[k] = (z_imag[k] - prev_w_imag[n]) * eq_constant + prev_w_imag[n];
        }
    }
    
    FloatVectorOperations::multiply(outBuffer, z_real.data(), cos_phase.data(), num_samples);
    FloatVectorOperations::addWithMultiply(outBuffer, z_imag.data(), sin_phase.data(), num_samples);
    
}

//////////////////////////////////////////////
float GammatoneFilter::get_centre_freq()
{
    return f0;
}
