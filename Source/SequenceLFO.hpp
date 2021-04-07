/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct SequenceLFO
{
    
    SequenceLFO(const dsp::ProcessSpec& spec);
    
    
    // Setters for parameters
    void set_frequency(float freq)   {  frequency.setTargetValue(freq); }
    void set_depth(float value)      {  depth.setTargetValue(value);    }
    void set_inverse(bool is_stereo) {  stereo = is_stereo;             }
    void set_sync(bool should_sync)  {  sync = should_sync;             }
    void set_stereo(bool is_stereo)  {  stereo = is_stereo;             }
    void set_voice(int shape) {
        if(shape != 0) enabled = true;
        sequence = {shape&1, shape&2, shape&4, shape&8};
    };
        
    
    
    void process(dsp::AudioBlock<float> output);
    
    void next_shape();
        
    void sync_with_playhead(AudioPlayHead* playhead);
    
private:
    int state = 0;
    std::vector<int> sequence = {0, 0, 0, 0};
    
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
    
    bool enabled = true, sync = false, stereo = false;
    
};
