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

    
    Value num_filters, smooth_mode, heavy_mode;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    ValueTree main_tree;
    XYPad xy_pad;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Distortion_ModellerAudioProcessor& audioProcessor;
    
    
    
    AnimatedSlider gain_slider, tone_slider, volume_slider, saturation_slider;
    
    SelectorComponent nfilter_selector = SelectorComponent({"20", "50", "70"});
    SelectorComponent quality_selector = SelectorComponent({"L", "M", "H"});

    SelectorComponent smooth_button = SelectorComponent({"Smooth"});
    SelectorComponent heavy_button = SelectorComponent({"Heavy"});

    Dark_LookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion_ModellerAudioProcessorEditor)
};
