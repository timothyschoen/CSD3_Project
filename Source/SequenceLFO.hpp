/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

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
#include <bitset>

struct SequenceLFO
{
    SequenceLFO(const ProcessSpec& spec);
    
    // Setters for parameters
    void set_frequency(float freq)   {  frequency.setTargetValue(freq); }
    void set_depth(float value)      {  depth.setTargetValue(value);    }
    void set_inverse(bool is_stereo) {  stereo = is_stereo;             }
    void set_sync(bool should_sync)  {  sync = should_sync;             }
    void set_stereo(bool is_stereo)  {  stereo = is_stereo;             }
    void set_voice(int new_shape)    {  shape = new_shape;              };

    void process(AudioBlock<float> output);
    
    void next_shape();
        
    void sync_with_playhead(AudioPlayHead* playhead);
    
private:
    int state = 0;
    int shape = 0;
    
    std::vector<std::function<float(float)>> waveshapes;
    
    int num_channels;
    float sample_rate;
    float phase = 0;
    
    SmoothedValue<float> synced_frequency, frequency, depth;
    
    const float note_divisions[7] = { 1.0f / 1.0f * 4.0f,
                                           1.0f / 2.0f * 4.0f,
                                           1.0f / 4.0f * 4.0f,
                                           1.0f / 8.0f * 4.0f,
                                           1.0f / 16.0f * 4.0f,
                                           1.0f / 32.0f * 4.0f,
                                           1.0f / 64.0f * 4.0f };
    
    bool sync = false, stereo = false;
    
    
    static int count_bits (int n) {
        int count = 0;
        while (n) {
            n &= (n - 1);
            count++;
        }
        return count;
    }
    
    
    inline static std::array<LookupTableTransform<float>, 16> lfo_tables;
    
    static bool fill_tables();

    inline static bool initialised = fill_tables();
};
