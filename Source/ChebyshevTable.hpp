/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PeakScaler.hpp"
#include "SequenceLFO.hpp"

inline static const int num_polynomials = 25;

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


using DistortionState = std::tuple<
    bool,   // Enabled
    bool,   // Kind
    float,  // Polynomial order
    float,  // Gain
    float,  // Scaling
    bool,   // Enable odd harmonics
    bool,   // Enable even harmonics
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
    
    bool enabled = true;
    bool kind = false;
    
    float poly_order;
    float gain;
    float scaling;
    
    SmoothedValue<float> smoothed_order;
    
    bool odd = true, even = true;
    
    float mod_freq = 2.0f;
    float mod_depth = 0.25;
    int mod_shape = 0;
    bool lfo_stereo = false, lfo_sync = false;
    
    std::array<dsp::LookupTableTransform<float>, num_polynomials>* current_table;
    float sample_rate;
    int num_channels;
    
    SequenceLFO lfo;

    DistortionState get_state();
    
    void set_state(const DistortionState& state);
    
    ChebyshevTable(const dsp::ProcessSpec& spec, float order, float gain, bool second_kind = false);
    
    void set_scaling(float amount);
    
    void process(std::vector<dsp::AudioBlock<float>>& input, std::vector<dsp::AudioBlock<float>>& output);
    
    dsp::AudioBlock<float> buffer;
    HeapBlock<char> buffer_data;
    
    dsp::AudioBlock<float> lfo_buffer;
    HeapBlock<char> lfo_buffer_data;
    
    dsp::AudioBlock<float> smoothed_block;
    HeapBlock<char> smoothed_data;
    
    void set_mod_depth(float depth);
    void set_mod_rate(float rate);
    void set_mod_shape(int shape_flag);
    
    void set_stereo(bool stereo);
    
    void set_even(bool enable_even);
    void set_odd(bool enable_odd);
    
    void set_table(float order, float gain, bool second_kind = false);
    
    void set_sync(bool sync);
    void sync_with_playhead(AudioPlayHead* playhead);

};
