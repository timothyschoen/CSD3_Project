/**********************************************************************
*          Copyright (c) 2015, Hogeschool voor de Kunsten Utrecht
*                      Hilversum, the Netherlands
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
#pragma once
#include "EnvelopeFollower.hpp"
#include <array>
#include <JuceHeader.h>


class RMSEnvelope : public EnvelopeFollower {
    
    int num_channels;
    int num_bands;
    int oversamp;
    
    OwnedArray<BallisticsFilter<float>> rms_filters;
    
public:
    RMSEnvelope(ProcessSpec& spec, int bands, int oversample_factor);
    
    void process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, int num_samples) override;

};
