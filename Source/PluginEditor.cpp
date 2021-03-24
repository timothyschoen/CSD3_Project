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
    : AudioProcessorEditor (&p), audioProcessor (p), main_tree(p.main_tree), xy_pad(main_tree)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 345);
    
    setLookAndFeel(&lnf);

    addAndMakeVisible(tone_slider);
    addAndMakeVisible(gain_slider);
    
    addAndMakeVisible(volume_slider);
    addAndMakeVisible(saturation_slider);
    
    addAndMakeVisible(smooth_button);
    addAndMakeVisible(heavy_button);
    
    addAndMakeVisible(xy_pad);
    
    tone_slider.setRange(0.0, 1.0);
    gain_slider.setRange(0.0, 1.0);
    volume_slider.setRange(0.0, 1.0);
    saturation_slider.setRange(0.0, 1.0);
    
    saturation_slider.setSkewFactor(0.25);
    tone_slider.setSkewFactor(0.3);
    
    saturation_slider.set_colour(1);
    tone_slider.set_colour(1);
    
    gain_slider.set_colour(3);
    volume_slider.set_colour(3);

    
    tone_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::draw_filter(value, 0.0, bounds.getWidth(), bounds.getHeight(), 0);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setFont(Font(7));
        g.setColour(Colours::white);
        g.drawText("TONE", bounds.getX() + 3, bounds.getY() + 2, 17, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    gain_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(0.8 - (value / 2.0), value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        g.setColour(Colours::white);
        
        g.drawText("IN", bounds.getX() + 3, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    volume_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(0.5, value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        g.setColour(Colours::white);
        g.drawText("OUT", bounds.getX() + 3, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    saturation_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(1.0 - value, 0.7, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("SAT", bounds.getX() + 3, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0));
    };

    
    //intermodulation_slider.setRange(0.05, 1.0);
    
    addAndMakeVisible(nfilter_selector);
    addAndMakeVisible(quality_selector);
    


    
    nfilter_selector.getValueObject().referTo(main_tree.getPropertyAsValue("Intermodulation", nullptr));
    smooth_button.getValueObject().referTo(main_tree.getPropertyAsValue("Smooth", nullptr));
    heavy_button.getValueObject().referTo(main_tree.getPropertyAsValue("Heavy", nullptr));
    quality_selector.getValueObject().referTo(main_tree.getPropertyAsValue("Quality", nullptr));
    
    
    tone_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Tone", nullptr));
    gain_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Gain", nullptr));
    volume_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Volume", nullptr));
    saturation_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Saturation", nullptr));
    
    xy_pad.update_tree(main_tree.getChildWithName("XYPad"));
   
    nfilter_selector.set_custom_draw(SelectorButton::paint_filters);
    
    smooth_button.callback = [this](int selection){
        smooth_mode.setValue(selection);
    };
    
    heavy_button.callback = [this](int selection){
        heavy_mode.setValue(selection);
    };
    
    
    
    quality_selector.callback = [this](int selection){
       
    };
    
    nfilter_selector.set_colour(0);
    quality_selector.set_colour(0);
    smooth_button.set_colour(4);
    heavy_button.set_colour(4);
    
}

Distortion_ModellerAudioProcessorEditor::~Distortion_ModellerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void Distortion_ModellerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(ColourTheme::bg_lighter);
}

void Distortion_ModellerAudioProcessorEditor::resized()
{
    nfilter_selector.setBounds(20, 270, 80, 24);
    quality_selector.setBounds(20, 305, 80, 24);
    
    saturation_slider.setBounds(120, 270, 170, 24);
    tone_slider.setBounds(120, 305, 170, 24);
    
    gain_slider.setBounds(315, 270, 170, 24);
    volume_slider.setBounds(315, 305, 170, 24);
    
    smooth_button.setBounds(getWidth() - 90, 270, 72, 24);
    heavy_button.setBounds(getWidth() - 90, 305, 72, 24);
    
    xy_pad.setBounds(0, 0, 600, 255);
}
