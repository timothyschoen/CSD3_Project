/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "Filterbanks/GammatoneFilterBank.hpp"
#include "Filterbanks/ResonBands.hpp"
#include "concurrentqueue.hpp"
#include "ChebyshevTable.hpp"
#include "HilbertAmplitude.hpp"
#include <JuceHeader.h>


class ZirconAudioProcessor  : public AudioProcessor, public ValueTree::Listener, public AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    ZirconAudioProcessor();
    ~ZirconAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void valueTreeChildRemoved(ValueTree &parent_tree, ValueTree &removed_child, int idx) override;
    void valueTreeChildAdded(ValueTree &parent_tree, ValueTree &added_child) override;
    void valueTreePropertyChanged (ValueTree &changed_tree, const Identifier &property) override;
    
    void parameterChanged (const String &parameter_id, float new_value) override;

    AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    ValueTree main_tree = ValueTree("Main");
    
private:
    
    dsp::ProcessSpec last_spec;
    dsp::ProcessSpec oversampled_spec;
    
    
    moodycamel::ConcurrentQueue<std::function<void()>> queue;
    
    std::unique_ptr<Filterbank> filter_bank;
    OwnedArray<ChebyshevTable> chebyshev_distortions;


    void set_harmonic(int idx, float shift, float gain, bool kind);
    void delete_harmonic(int idx);
    
    void update_bands(int freq_range_overlap, bool reset = false);
    void set_oversample_rate(int new_oversample_factor);
    
    float sample_rate;
    int num_bands;
    int block_size;
    
    const int max_bands = 50;
    const int max_voices = 5;
    
    
    int oversample_factor = 1;
    SmoothedValue<float> master_volume;
    SmoothedValue<float> tone_cutoff;
    SmoothedValue<float> saturation;
    SmoothedValue<float> gain;
    
    bool smooth_mode = false;
    bool heavy_mode = false;
    

    std::unique_ptr<HilbertAmplitude> hilbert;
    
    dsp::AudioBlock<float> tone_block, saturation_block, gain_block;
    HeapBlock<char> tone_data, saturation_data, gain_data;
    
    std::vector<HeapBlock<char>> band_data, iamp_data, band_tone_data, write_data, inv_data;
    std::vector<dsp::AudioBlock<float>> inv_scaling, instant_amp, split_bands, write_bands, band_tone, read_bands;
    
    std::vector<dsp::StateVariableTPTFilter<float>> noise_filters;
    
    std::unique_ptr<dsp::Oversampling<float>> oversampler;
    
    AudioProcessorValueTreeState proc_valuetree;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZirconAudioProcessor)
};
