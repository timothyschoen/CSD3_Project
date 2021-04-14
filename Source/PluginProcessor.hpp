/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "EnvelopeFollower.hpp"
#include "Filterbanks/Filterbank.hpp"
#include "concurrentqueue.hpp"
#include "ChebyshevTable.hpp"

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
    
    ProcessSpec last_spec;
    ProcessSpec oversampled_spec;
    
    moodycamel::ConcurrentQueue<std::function<void()>> queue;
    
    std::unique_ptr<Filterbank> filter_bank;
    OwnedArray<ChebyshevTable> chebyshev_distortions;

    void set_harmonic_order(int idx, float shift, bool kind);
    void set_harmonic_volume(int idx, float volume);
    
    void add_harmonic();
    void delete_harmonic(int idx);
    void clear_harmonics();
    
    void set_num_bands(int selection, bool reset = false);
    void set_oversample_rate(int new_oversample_factor);
    
    std::vector<float> get_centre_freqs();
    
    float sample_rate;
    int num_bands;
    int block_size;
    
    const int max_bands = 50;
    const int max_voices = 5;
    
    
    int oversample_factor = 1;
    SmoothedValue<float> master_volume;
    SmoothedValue<float> tone_cutoff;
    SmoothedValue<float> gain;
    
    bool high_mode = false;
    bool smooth_mode = false;
    

    std::unique_ptr<EnvelopeFollower> envelope_follower;
    
    AudioBlock<float> tone_block, gain_block;
    HeapBlock<char> tone_data, gain_data;
    
    std::vector<HeapBlock<char>> band_data, iamp_data, band_tone_data, write_data, inv_data;
    std::vector<AudioBlock<float>> inv_scaling, instant_amp, split_bands, write_bands, band_tone, read_bands;
    
    std::vector<StateVariableTPTFilter<float>> noise_filters;
    
    std::unique_ptr<Oversampling<float>> oversampler;
    
    DryWetMixer<float> mixer;
    
    AudioProcessorValueTreeState proc_valuetree;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZirconAudioProcessor)
};
