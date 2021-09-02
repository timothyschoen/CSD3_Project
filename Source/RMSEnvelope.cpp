/**********************************************************************
*          Copyright (c) 2020, Hogeschool voor de Kunsten Utrecht
*                      Utrecht, the Netherlands
*                          All rights reserved
***********************************************************************
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.
*  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************
*
*  Author             : Timothy Schoen
*  E-mail             : timschoen123@gmail.com
*
**********************************************************************/

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

void RMSEnvelope::process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, std::vector<AudioBlock<float>>& phase_bands, int num_samples) {
    
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
