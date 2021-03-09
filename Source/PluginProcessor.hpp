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
    
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved);
    
    void valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    
    void updateParameters()
    {
        
    }

private:
    
    moodycamel::ConcurrentQueue<std::function<void()>> queue;
    std::unique_ptr<GammatoneFilterBank> filter_bank;
        
    std::vector<std::vector<std::unique_ptr<ChebyshevTable>>> chebyshev_distortions;
    
    void set_harmonic(int idx, HarmonicResponse response);
    void delete_harmonic(int idx);
    
    void update_bands(double freq_range_overlap);
    
    double sample_rate;
    int num_bands;
    int block_size;
    
    std::vector<HeapBlock<char>> band_data;
    
    HeapBlock<char> temp_storage;
    dsp::AudioBlock<float> temp_buffer;
    
    AudioBuffer<float> harmonics_buffer;
    std::vector<dsp::AudioBlock<float>> split_bands;
    
    dsp::ProcessorDuplicator<dsp::StateVariableFilter::Filter<float>, dsp::StateVariableFilter::Parameters<float>> tone_filter;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion_ModellerAudioProcessor)
};
