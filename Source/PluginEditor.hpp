/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.hpp"
#include "GUI/XYPad.hpp"
#include "GUI/AnimatedSlider.hpp"
#include "LookAndFeel.hpp"
//==============================================================================
/**
*/
class Distortion_ModellerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Distortion_ModellerAudioProcessorEditor (Distortion_ModellerAudioProcessor&);
    ~Distortion_ModellerAudioProcessorEditor() override;

    
    Value num_filters;
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Distortion_ModellerAudioProcessor& audioProcessor;
    
    ValueTree main_tree = ValueTree("Main");
    XYPad xy_pad;
    
    AnimatedSlider gain_slider;
    AnimatedSlider tone_slider;
    
    SelectorComponent nfilter_selector = SelectorComponent(2, {"20", "50"});


    Dark_LookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion_ModellerAudioProcessorEditor)
};
