/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "Gamma/GammatoneFilterBank.h"
#include "../Libraries/gammatone/filterbank.hpp"
#include "concurrentqueue.hpp"
#include "ChebyshevTable.hpp"
#include <JuceHeader.h>

using HarmonicResponse = std::tuple<float, float, bool>;

class Distortion_ModellerAudioProcessor  : public juce::AudioProcessor, public ValueTree::Listener
{
public:
    //==============================================================================
    Distortion_ModellerAudioProcessor();
    ~Distortion_ModellerAudioProcessor() override;

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
    
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    
    void valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    
    void updateParameters()
    {
        
    }

    ValueTree main_tree = ValueTree("Main");
    
private:
    
    
    
    moodycamel::ConcurrentQueue<std::function<void()>> queue;
    std::unique_ptr<GammatoneFilterBank> filter_bank;
    
    std::vector<std::vector<Hilbert>> hilbert;
        
    std::vector<std::vector<std::unique_ptr<ChebyshevTable>>> chebyshev_distortions;
    
    void set_harmonic(int idx, HarmonicResponse response);
    void delete_harmonic(int idx);
    
    void update_bands(float freq_range_overlap);
    
    float sample_rate;
    int num_bands;
    int block_size;
    
    int oversample_factor = 2;
    float master_volume;
    
    bool smooth_mode = false;
    
    std::vector<HeapBlock<char>> band_data;
    std::vector<std::unique_ptr<PeakScaler>> peak_scalers;
    
    HeapBlock<char> temp_storage, temp_storage2, temp_storage3, temp_storage4, temp_storage5,  temp_storage6;
    
    dsp::AudioBlock<float> temp_buffer, temp_buffer2, instant_amp, inv_scaling, filter_buffer, inv_filter;

    
    AudioBuffer<float> harmonics_buffer;
    
    
    std::vector<dsp::AudioBlock<float>> split_bands;

    
    std::vector<dsp::StateVariableTPTFilter<float>> noise_filters;
    
    std::unique_ptr<dsp::Oversampling<float>> oversampler;
    
    float tone_cutoff;
    float saturation = 1.0;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion_ModellerAudioProcessor)
};
