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
#include "GUI/LookAndFeel.hpp"
//==============================================================================
/**
*/
class ZirconAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ZirconAudioProcessorEditor (ZirconAudioProcessor&);
    ~ZirconAudioProcessorEditor() override;

    
    Value num_filters, high_mode, smooth_mode;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    TooltipWindow tooltipWindow { this, 800 };

    ValueTree main_tree;
    XYPad xy_pad;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ZirconAudioProcessor& audioProcessor;
    
    AnimatedSlider gain_slider, tone_slider, volume_slider, wet_slider;
    
    SelectorComponent nfilter_selector = SelectorComponent({"12", "18"});
    SelectorComponent quality_selector = SelectorComponent({"L", "M", "H"});

    SelectorComponent high_button = SelectorComponent({"High"});
    SelectorComponent smooth_button = SelectorComponent({"Smooth"});

    Dark_LookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZirconAudioProcessorEditor)
};
