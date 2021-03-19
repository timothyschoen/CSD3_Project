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
    Value smooth_mode;
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    ValueTree main_tree;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Distortion_ModellerAudioProcessor& audioProcessor;
    
    XYPad xy_pad;
    
    AnimatedSlider gain_slider, tone_slider, volume_slider, saturation_slider;
    
    SelectorComponent nfilter_selector = SelectorComponent({"20", "50", "70"});
    SelectorComponent quality_selector = SelectorComponent({"L", "M", "H"});

    SelectorComponent smooth_button = SelectorComponent({"Smooth"});

    Dark_LookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion_ModellerAudioProcessorEditor)
};
