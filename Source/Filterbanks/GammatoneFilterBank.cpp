/*
 * \class GammatoneFilterBank
 *
 * \brief Definition of GammatoneFilterBank class.
 *
 *
 * \authors 3DI-DIANA Research Group (University of Malaga), in alphabetical order: M. Cuevas-Rodriguez, C. Garre, D. Gonzalez-Toledo, E.J. de la Rubia-Cuestas, L. Molina-Tanco ||
 * Coordinated by , A. Reyes-Lecuona (University of Malaga) and L.Picinali (Imperial College London) ||
 * \b Contact: areyes@uma.es and l.picinali@imperial.ac.uk
 *
 * \b Contributions: (additional authors/contributors can be added here)
 * \b This module was written by Michael Krzyzaniak m.krzyzaniak@surrey.ac.uk
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

#include <cmath>
#include <assert.h>
#include "GammatoneFilterBank.hpp"

#define GAMMATONE_FILTER_ORDER 4

GammatoneFilterBank::GammatoneFilterBank(ProcessSpec& spec)
{
    
    num_channels = spec.numChannels;
    block_size = spec.maximumBlockSize;
    sample_rate = spec.sampleRate;
    
    filters.resize(num_channels);
}

GammatoneFilterBank::~GammatoneFilterBank()
{
}

int GammatoneFilterBank::init_with_overlap(float low_freq, float high_freq, float overlap)
{
    float stepfactor = 1.0f - overlap;
    
    jassert(overlap < 1);
    jassert(high_freq > low_freq);

    remove_filters();
    
    float num_filters;
    
    num_filters = -(q * log(low_freq + min_width)) / stepfactor;
    num_filters += (q * log(high_freq + min_width)) / stepfactor;
    
    //round to 2 decimal places to get rid of small numerical errors
    num_filters = round(num_filters * 100) * 0.01;
    num_filters = ceil(num_filters);
    
    for(int i = 0; i <= num_filters; i++)
    {
        float denominator = exp(i * stepfactor / q);
        float center_freq = -min_width;
        center_freq += (high_freq + min_width) / denominator;
        float bandwidth = (center_freq / q) + min_width;
        add_filter(GAMMATONE_FILTER_ORDER, center_freq, bandwidth);
    }
    
    return num_filters;
}


float GammatoneFilterBank::init_with_num_filters(float low_freq, float high_freq, unsigned int n_filters)
{
    
    n_filters -= 1; //there will be an extra one at the nyquist freq
    float stepfactor = log(high_freq + min_width) - log(low_freq + min_width);
    stepfactor *= q / n_filters;
    float overlap = 1-stepfactor;
    init_with_overlap(low_freq, high_freq, overlap);
    return overlap;
}



void GammatoneFilterBank::add_filter(unsigned _order, float _freq, float _erb)
{
    for(int ch = 0; ch < num_channels; ch++) {
        filters[ch].push_back(std::unique_ptr<GammatoneFilter>(new GammatoneFilter(sample_rate, block_size, _order, _freq, _erb)));
    }
}



void GammatoneFilterBank::remove_filters()
{
    for(int ch = 0; ch < num_channels; ch++) {
        filters[ch].clear();
    }
}


int GammatoneFilterBank::get_num_filters()
{
    return (int)filters[0].size();
}


void GammatoneFilterBank::process(const AudioBlock<float>& in_buffer, std::vector<AudioBlock<float>>& out_buffer)
{
    int size = (int)in_buffer.getNumSamples();
    
    jassert(in_buffer.getNumChannels() == num_channels);
    
    for (int ch = 0; ch < num_channels; ch++)
    {
    
        for (int n = 0; n < filters[ch].size(); n++)
        {
            auto* f = filters[ch][n].get();
            
            if (f != nullptr && n < out_buffer.size())
            {
                f->process(in_buffer.getChannelPointer(ch), out_buffer[n].getChannelPointer(ch), size);
            }
        }
        
    }
}

float GammatoneFilterBank::get_centre_freq(int idx) {
    return filters[0][idx]->get_centre_freq();
}
