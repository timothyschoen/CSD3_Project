/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

/**********************************************************************
*          Copyright (c) 2020, Hogeschool voor de Kunsten Utrecht
*                      Utrecht, the Netherlands
*                          All rights reserved
***********************************************************************
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.
*  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************
*
*  Author             : Timothy Schoen
*  E-mail             : timschoen123@gmail.com
*
**********************************************************************/
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.hpp"
#include "GUI/XYPad.hpp"
#include "GUI/AnimatedSlider.hpp"
#include "GUI/LookAndFeel.hpp"
//==============================================================================
/**
*/
class ZirconAudioProcessorEditor  : public juce::AudioProcessorEditor, private ValueTree::Listener, public Timer
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
    
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier &     property) override;
    
    void timerCallback() override;
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
