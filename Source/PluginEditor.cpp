/**********************************************************************
*          Copyright (c) 2015, Hogeschool voor de Kunsten Utrecht
*                      Hilversum, the Netherlands
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

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"
#include "GUI/Graphs.hpp"
//==============================================================================
ZirconAudioProcessorEditor::ZirconAudioProcessorEditor (ZirconAudioProcessor& p)
    : AudioProcessorEditor (&p), main_tree(p.main_tree),  xy_pad(p.main_tree), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    setResizable(false, false);
    setSize (655, 345);
    
    setLookAndFeel(&lnf);

    addAndMakeVisible(tone_slider);
    addAndMakeVisible(gain_slider);
    
    addAndMakeVisible(volume_slider);
    addAndMakeVisible(wet_slider);
    
    addAndMakeVisible(high_button);
    addAndMakeVisible(smooth_button);
    
    addAndMakeVisible(xy_pad);
    
    xy_pad.inspector.allow_stereo(p.getTotalNumOutputChannels() > 1);
    
    tone_slider.setRange(0.0f, 1.0f);
    gain_slider.setRange(0.0f, 1.0f);
    volume_slider.setRange(0.0f, 1.0f);
    wet_slider.setRange(0.0f, 1.0f);
    
    tone_slider.setSkewFactor(0.3);
    
    wet_slider.set_colour(1);
    tone_slider.set_colour(1);
    
    gain_slider.set_colour(3);
    volume_slider.set_colour(3);
    
    gain_slider.setTooltip("Gain");
    wet_slider.setTooltip("Wet");
    tone_slider.setTooltip("Tone");
    volume_slider.setTooltip("Volume");
    
    high_button.set_tooltips({"High mode"});
    smooth_button.set_tooltips({"Smooth mode"});
    
    tone_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::draw_filter(value, 0.0f, bounds.getWidth(), bounds.getHeight(), 0, 0.5);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setFont(Font(7));
        g.setColour(Colours::white);
        g.drawText("TONE", bounds.getX() + 3, bounds.getY() + 2, 17, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0f));
    };
    
    gain_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(0.8 - (value / 2.0f), value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        g.setColour(Colours::white);
        
        g.drawText("IN", bounds.getX() + 3, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0f));
    };
    
    volume_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(0.5f, value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        g.setColour(Colours::white);
        g.drawText("OUT", bounds.getX() + 3, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0f));
    };
    
    wet_slider.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(1.0f - value, 0.7, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("WET", bounds.getX() + 3, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0f));
    };

    addAndMakeVisible(nfilter_selector);
    addAndMakeVisible(quality_selector);
    
    
    
    nfilter_selector.set_tooltips({"Filterbank density (12 filters)", "Filterbank density (16 filters)"});
    quality_selector.set_tooltips({"Oversampling (1x)", "Oversampling (2x)", "Oversampling (4x)"});
    
    nfilter_selector.getValueObject().referTo(main_tree.getPropertyAsValue("Intermodulation", nullptr));
    high_button.getValueObject().referTo(main_tree.getPropertyAsValue("High", nullptr));
    smooth_button.getValueObject().referTo(main_tree.getPropertyAsValue("Smooth", nullptr));
    quality_selector.getValueObject().referTo(main_tree.getPropertyAsValue("Quality", nullptr));
    
    tone_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Tone", nullptr));
    gain_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Gain", nullptr));
    volume_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Volume", nullptr));
    wet_slider.getValueObject().referTo(main_tree.getPropertyAsValue("Wet", nullptr));
    
    xy_pad.update_tree(main_tree.getChildWithName("XYPad"));
   
    nfilter_selector.set_custom_draw(SelectorButton::paint_filters);
    
    high_button.callback = [this](int selection){
        high_mode.setValue(selection);
    };
    
    smooth_button.callback = [this](int selection){
        smooth_mode.setValue(selection);
    };
    
    
    
    quality_selector.callback = [this](int selection){
       
    };
    
    nfilter_selector.set_colour(0);
    quality_selector.set_colour(0);
    high_button.set_colour(4);
    smooth_button.set_colour(4);
    
    main_tree.addListener(this);
    
}

ZirconAudioProcessorEditor::~ZirconAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void ZirconAudioProcessorEditor::paint (juce::Graphics& g)
{
    
    auto base = ColourTheme::bg_lighter;
    auto gradient = ColourGradient(base.brighter(0.02f), 0, 245, base.darker(0.2f), 0, 345, false);
    
    g.setGradientFill(gradient);
    g.fillRect(0, 245, getWidth(), 100);
}

void ZirconAudioProcessorEditor::resized()
{
    nfilter_selector.setBounds(20, 270, 80, 24);
    quality_selector.setBounds(20, 305, 80, 24);
    
    wet_slider.setBounds(120, 270, 190, 24);
    tone_slider.setBounds(120, 305, 190, 24);
    
    gain_slider.setBounds(335, 270, 190, 24);
    volume_slider.setBounds(335, 305, 190, 24);
    
    high_button.setBounds(getWidth() - 100, 270, 80, 24);
    smooth_button.setBounds(getWidth() - 100, 305, 80, 24);
    
    xy_pad.setBounds(0, 0, 655, 255);
}

void ZirconAudioProcessorEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier &     property) {
    
    if(xy_pad.exclude_parameter_change) {
        return;
    }
    
    auto property_value = treeWhosePropertyHasChanged.getProperty(property);
    
    bool is_int = property_value.isInt();
    bool is_bool = property_value.isBool();
    
    String name = property.toString();
    String value = String((float)treeWhosePropertyHasChanged.getProperty(property), is_int ? 0 : 2);
    
    if(is_bool) {
        value = (bool)property_value ? "On" : "Off";
    }
    if(name == "Intermodulation") {
        name = "Num. filters";
        value = String(value.getIntValue() ? 16.0 : 12.0, 0);
    }
    if(name == "Quality") {
        value = (String[3]){"Low", "Medium", "High"}[value.getIntValue()];
    }
    if(name == "Kind") {
        value = String(value.getIntValue() + 1.0, 0);
    }
    if(name == "Phase") {
        value = (bool)property_value ? "-1" : "1";
    }
    
    if(treeWhosePropertyHasChanged.getType().toString().contains("XYSlider")) {
        int idx = treeWhosePropertyHasChanged.getParent().indexOf(treeWhosePropertyHasChanged) + 1;
        name = "Slider " + String(idx);
        
        if(property == Identifier("X") || property == Identifier("Y")) {
            
            
            double x = (double)treeWhosePropertyHasChanged.getProperty("X") * 8.1 + 0.12;
            double y = 1.0 - (double)treeWhosePropertyHasChanged.getProperty("Y");
            value = "X:" + String(std::clamp(x, 0.0, 8.0), 2) + ", Y:" +  String(std::clamp(y, 0.0, 1.0), 2);
        }
        else {
            name += ": " + property.toString();
        }
    }

    
    xy_pad.selection_name = name;
    xy_pad.selection_value = value;
    xy_pad.repaint();
    
    startTimer(1500);
}

void ZirconAudioProcessorEditor::timerCallback()
{
    xy_pad.selection_name = "";
    xy_pad.repaint();
}
