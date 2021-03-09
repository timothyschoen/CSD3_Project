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
 * \b Licence: This copy of 3dti_AudioToolkit is licensed to you under the terms described in the 3DTI_AUDIOTOOLKIT_LICENSE file included in this distribution.
 *
 * \b Acknowledgement: This project has received funding from the European Union's Horizon 2020 research and innovation programme under grant agreement No 644051
 */

#include <cmath>
#include <assert.h>
#include "GammatoneFilterBank.h"

#define GAMMATONE_FILTER_ORDER 4

GammatoneFilterBank::GammatoneFilterBank(double rate, int size, int channels, float filter_q, float filter_min_width)
{
    
    num_channels = channels;
    block_size = size;
    q = filter_q;
    min_width = filter_min_width;
    
    sample_rate = rate;
    
    filters.resize(num_channels, std::vector<GammatoneFilter*>());
}

GammatoneFilterBank::~GammatoneFilterBank()
{
}

unsigned GammatoneFilterBank::InitWithFreqRangeOverlap(double _lowFreq, double _highFreq, double _overlap)
{
    double stepfactor = 1.0 - _overlap;
    if(_overlap >= 1)
    {
        //SET_RESULT(RESULT_ERROR_OUTOFRANGE, "overlap must be less than 1");
        return -1;
    }
    if(_lowFreq >= _highFreq)
    {
        //SET_RESULT(RESULT_ERROR_INVALID_PARAM, "low freq should be less than high freq");
        return -1;
    }
    
    RemoveFilters();
    
    double num_filters;
    
    num_filters = -(q * log(_lowFreq + min_width)) / stepfactor;
    num_filters += (q * log(_highFreq + min_width)) / stepfactor;
    
    //round to 2 decimal places to get rid of small numerical errors
    num_filters = round(num_filters * 100) * 0.01;
    num_filters = ceil(num_filters);
    
    for(int i = 0; i <= num_filters; i++)
    {
        double denominator = exp(i * stepfactor / q);
        double center_freq = -min_width;
        center_freq += (_highFreq + min_width) / denominator;
        double bandwidth = (center_freq / q) + min_width;
        AddFilter(GAMMATONE_FILTER_ORDER, center_freq, bandwidth);
    }
    
    return num_filters;
}

//////////////////////////////////////////////
double GammatoneFilterBank::InitWithFreqRangeNumFilters(double _lowFreq, double _highFreq, unsigned _numFilters)
{
    
    _numFilters -= 1; //there will be an extra one at the nyquist freq
    double stepfactor = log(_highFreq + min_width) - log(_lowFreq + min_width);
    stepfactor *= q / _numFilters;
    double overlap = 1-stepfactor;
    InitWithFreqRangeOverlap(_lowFreq, _highFreq, overlap);
    return overlap;
}


//////////////////////////////////////////////
void GammatoneFilterBank::AddFilter(unsigned _order, double _freq, double _erb)
{
    for(int ch = 0; ch < num_channels; ch++) {
        filters[ch].push_back(new GammatoneFilter(sample_rate, block_size, _order, _freq, _erb));
    }
   
}


//////////////////////////////////////////////
void GammatoneFilterBank::RemoveFilters()
{
    for(int ch = 0; ch < num_channels; ch++) {
        for(int b = 0; b < filters[ch].size(); b++) {
            delete filters[ch][b];
        }
        filters[ch].clear();
    }
    
}

//////////////////////////////////////////////
int GammatoneFilterBank::GetNumFilters()
{
    return filters[0].size();
}

//////////////////////////////////////////////
void GammatoneFilterBank::process(dsp::AudioBlock<float> inBuffer, std::vector<dsp::AudioBlock<float>> outBuffer)
{
    int size = inBuffer.getNumSamples();
    
    assert(inBuffer.getNumChannels() == num_channels);
    
    for (int ch = 0; ch < num_channels; ch++)
    {
    
        for (int n = 0; n < filters[ch].size(); n++)
        {
            GammatoneFilter* f = filters[ch][n];
            
            if (f != nullptr && n < outBuffer.size())
            {
                f->process(inBuffer.getChannelPointer(ch), outBuffer[n].getChannelPointer(ch), size);
            }
        }
        
    }
}
