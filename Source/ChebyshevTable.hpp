/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SequenceLFO.hpp"

inline static const int num_polynomials = 40;

class ChebyshevFactory : public LookupTableTransform<float>
{
    
public:
    
    static bool fillTables();
    
    inline static std::array<LookupTableTransform<float>, num_polynomials> first_tables;
    inline static std::array<LookupTableTransform<float>, num_polynomials> second_tables;
    
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
    bool,    // LFO Stereo
    bool,    // Smooth mode
    std::vector<float> // Filter frequencies
>;

class ChebyshevTable
{
public:
    //==============================================================================
    
    ChebyshevTable(const ProcessSpec& spec, std::vector<float> centre_freqs);
    
    ChebyshevTable(const ProcessSpec& spec, const ChebyshevTable& to_copy);
    
    void process(std::vector<AudioBlock<float>>& input, std::vector<AudioBlock<float>>& output);

    void sync_with_playhead(AudioPlayHead* playhead);
    
    void set_centre_freqs(std::vector<float> centre_freqs);
    
    void receive_message(const Identifier& id, float value);

private:

    SequenceLFO lfo;
    
    std::vector<StateVariableTPTFilter<float>> noise_filters;
    
    bool enabled = true, kind = false;
    bool lfo_stereo = false, lfo_sync = false;
    bool high_mode = false, invert_phase = false;
    
    float order = 2.0f, volume = 0.9f, scaling = 1.0f;
    
    float mod_freq = 2.0f;
    float mod_depth = 0.25;
    int mod_shape = 0;
    
    float sample_rate;
    int num_channels;
    
    ProcessSpec process_spec;
    
    std::vector<float> filter_freqs;
    
    SmoothedValue<float> smoothed_volume, smoothed_scaling, smoothed_order;

    std::array<LookupTableTransform<float>, num_polynomials>* current_table;
    
    AudioBlock<float> buffer, lfo_buffer, smoothed_order_buffer, smoothed_volume_buffer, smoothed_scaling_buffer, temp_buffer;
    HeapBlock<char> buffer_data, lfo_buffer_data, smoothed_volume_data, smoothed_order_data, smoothed_scaling_data, temp_data;
};
