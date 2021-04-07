/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SequenceLFO.hpp"

inline static const int num_polynomials = 40;

class ChebyshevFactory : public dsp::LookupTableTransform<float>
{
    
public:
    
    static bool fillTables();
    
    inline static std::array<dsp::LookupTableTransform<float>, num_polynomials> first_tables;
    inline static std::array<dsp::LookupTableTransform<float>, num_polynomials> second_tables;
    
    inline static bool initialised = fillTables();
    
    
private:
    
    ChebyshevFactory(int order, bool second_kind);
};


// Tuple that holds tot full state
// This makes it easier to restore the state when we change the number of filter bands
using DistortionState = std::tuple<
    bool,   // Enabled
    bool,   // Kind
    float,  // Polynomial order
    float,  // Gain
    float,  // Scaling
    float,  // Modulation frequency
    float,  // Modulation Depth
    int,     // Modulation shape (binary flag)
    bool,    // LFO Sync
    bool    // LFO Stereo
>;

class ChebyshevTable
{
public:
    //==============================================================================
    
    DistortionState get_state();
    
    void set_state(const DistortionState& state);
    
    ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind = false);
    
    void set_scaling(float amount);
    
    void process(std::vector<dsp::AudioBlock<float>>& input, std::vector<dsp::AudioBlock<float>>& output, std::vector<dsp::AudioBlock<float>>& amplitude);
    
    void set_mod_depth(float depth);
    void set_mod_rate(float rate);
    void set_mod_shape(int shape_flag);
    
    
    void set_enabled(bool enabled);
    void set_stereo(bool stereo);
    
    void set_table(float order, float gain, bool second_kind = false);
    
    void set_sync(bool sync);
    void sync_with_playhead(AudioPlayHead* playhead);

private:

    SequenceLFO lfo;
    
    bool enabled = true, kind = false;
    bool lfo_stereo = false, lfo_sync = false;
    
    float poly_order, gain, scaling;
    
    float mod_freq = 2.0f;
    float mod_depth = 0.25;
    int mod_shape = 0;
    
    float sample_rate;
    int num_channels;
    
    SmoothedValue<float> smoothed_gain, smoothed_scaling, smoothed_order;

    std::array<dsp::LookupTableTransform<float>, num_polynomials>* current_table;
    
    dsp::AudioBlock<float> buffer, lfo_buffer, smoothed_order_buffer, smoothed_gain_buffer, smoothed_scaling_buffer, clean_buffer, temp_buffer;
    HeapBlock<char> buffer_data, lfo_buffer_data, smoothed_gain_data, smoothed_order_data, smoothed_scaling_data, clean_data, temp_data;
};
