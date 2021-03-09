/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"
#include "GUI/Graphs.hpp"
//==============================================================================
Distortion_ModellerAudioProcessorEditor::Distortion_ModellerAudioProcessorEditor (Distortion_ModellerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), xy_pad(main_tree)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 300);
    
    setLookAndFeel(&lnf);

    addAndMakeVisible(tone_slider);
    addAndMakeVisible(gain_slider);
    addAndMakeVisible(xy_pad);
    
    tone_slider.setRange(0.0, 1.0);
    gain_slider.setRange(0.0, 1.0);
    
    tone_slider.setSkewFactor(0.25);
    
    tone_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::draw_filter(value, 0.0, bounds.getWidth(), bounds.getHeight(), 0);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setColour(Colours::white);
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    gain_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(0.5, value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white);
        g.strokePath(shape, PathStrokeType(1.0));
    };

    
    //intermodulation_slider.setRange(0.05, 1.0);
    
    addAndMakeVisible(nfilter_selector);
    
    
    main_tree.addListener(&audioProcessor);
    
    main_tree.setProperty("Intermodulation", 0.1, nullptr);
    main_tree.setProperty("Tone", 1.0, nullptr);
    main_tree.setProperty("Gain", 1.0, nullptr);
    
    num_filters.referTo(main_tree.getPropertyAsValue("Intermodulation", nullptr));
    tone_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Tone", nullptr));
    gain_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Gain", nullptr));
    
   
    nfilter_selector.set_custom_draw(SelectorButton::paint_filters);
    
    nfilter_selector.callback = [this](int selection){
        double options[2] = {0.0, 0.8};
        num_filters.setValue(options[selection]);
    };
    
    
}

Distortion_ModellerAudioProcessorEditor::~Distortion_ModellerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void Distortion_ModellerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    
    
    g.fillAll(Colour(18, 18, 18));
    
    

}

void Distortion_ModellerAudioProcessorEditor::resized()
{
    nfilter_selector.setBounds(20, 215, 75, 30);
    
    gain_slider.setBounds(110, 215, 150, 24);
    tone_slider.setBounds(110, 250, 150, 24);
    
    
    xy_pad.setBounds(0, 0, 600, 200);
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
