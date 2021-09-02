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
#pragma once
#include <JuceHeader.h>


// Interface class for envelope followers
struct EnvelopeFollower {
    
    virtual ~EnvelopeFollower() {};

    virtual void process(const std::vector<AudioBlock<float>>& in_bands, std::vector<AudioBlock<float>>& out_bands, std::vector<AudioBlock<float>>& inverse_bands, std::vector<AudioBlock<float>>& phase_bands, int num_samples) = 0;
    
};
