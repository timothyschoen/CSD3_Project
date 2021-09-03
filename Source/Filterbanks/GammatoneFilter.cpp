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
 * \b Licence: This copy of 3dti_AudioToolkit is licensed to you under the terms described in the LICENCE file included in this distribution.
 *
 * \b Acknowledgement: This project has received funding from the European Union's Horizon 2020 research and innovation programme under grant agreement No 644051
 */

#define _USE_MATH_DEFINES
#include <JuceHeader.h>
#include <cmath>
#include "GammatoneFilter.hpp"

//////////////////////////////////////////////
GammatoneFilter::GammatoneFilter(double rate, int block_size, unsigned filter_order, float center_freq, float band_width, bool prepare_ir)
{
    order = filter_order;
    
   
    prev_z_real.resize(order);
    prev_z_imag.resize(order);
    prev_w_real.resize(order);
    prev_w_imag.resize(order);
    
    temp_buffer_real.resize(order);
    temp_buffer_imag.resize(order);
    
    an = an_table[filter_order];
    cn = 2.0f * sqrt(pow(2, 1.0/(double)filter_order) - 1.0f);
    
    
    f0 = center_freq;
    sample_rate = rate;
    
    
    
    double phase_increment = f0 * MathConstants<float>::twoPi / sample_rate;
    cos_phase_increment = cos(phase_increment);
    sin_phase_increment = sin(phase_increment);
    
    b = band_width / an;
    eq_constant = 1.0f - exp(-MathConstants<float>::twoPi * b / sample_rate);
    
    cos_phase.resize(block_size, 0.0f);
    sin_phase.resize(block_size, 0.0f);
    negated_sin.resize(block_size, 0.0f);
    z_real.resize(block_size, 0.0f);
    z_imag.resize(block_size, 0.0f);
    
    if(!prepare_ir) return;
    
    // Calculate latency and prepare IR for freq domain processing
    int max_delay = 8192;
    auto test_filter = GammatoneFilter(sample_rate, max_delay, order, f0, b * an, false);

    auto in_ir = std::vector<float>(max_delay);
    auto out_ir = std::vector<float>(max_delay);
    in_ir[1] = 1.0f;

    
    test_filter.process(in_ir.data(), out_ir.data(), max_delay);
    
    for(auto& sample : out_ir) sample = std::abs(sample);
    
    auto max_elt = std::max_element(out_ir.begin(), out_ir.end());

    latency = max_elt - out_ir.begin();
    
#if ENABLE_FREQDOMAIN
    apply_window(out_ir, hamming_window);
    
    AudioBuffer<float> ir_buffer(1, max_delay);
    ir_buffer.addFrom(0, 0, out_ir.data(), max_delay);
   
    convolution.loadImpulseResponse(AudioBuffer<float>(ir_buffer), sample_rate,  Convolution::Stereo::no,  Convolution::Trim::no,  Convolution::Normalise::no);
    
    convolution.prepare({sample_rate, (uint32)block_size, 1});
#endif
    
    
}

//////////////////////////////////////////////
GammatoneFilter::~GammatoneFilter()
{
}

//////////////////////////////////////////////
void GammatoneFilter::process(const float* inBuffer, float* outBuffer, int num_samples)
{
    
    for (int k = 0; k < num_samples; k++)
    {
        // TODO: fix with circular buffer instead of branching
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

#if ENABLE_FREQDOMAIN
void GammatoneFilter::process_freq(const float* inBuffer, float* outBuffer, int num_samples)
{
    AudioBuffer<float> in_buf(1, num_samples);
    AudioBlock<float> in_block(in_buf);
    
    std::copy(inBuffer, inBuffer + num_samples, in_block.getChannelPointer(0));
    
    convolution.process(ProcessContextReplacing<float>(in_block));
    
    std::copy(in_block.getChannelPointer(0), in_block.getChannelPointer(0) + num_samples, outBuffer);
}
#endif

//////////////////////////////////////////////
float GammatoneFilter::get_centre_freq()
{
    return f0;
}



int GammatoneFilter::calculate_latency() {
    return latency;
}
