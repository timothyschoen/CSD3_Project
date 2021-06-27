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
#include "XYPad.hpp"
#include "Graphs.hpp"
#include "LookAndFeel.hpp"

XYInspector::XYInspector() {
    
    enabled_button.setClickingTogglesState(true);
    enabled_button.setConnectedEdges(12);
    
    volume.set_colour(0);
    kind_select.set_colour(0);

    mod_depth.set_colour(2);
    mod_rate.set_colour(2);
    shape_select.set_colour(2);
    mod_settings.set_colour(2);
    
    kind_select.set_tooltips({"First kind", "Second kind"});
    shape_select.set_tooltips({"Sine", "Square", "Triangle", "Sawtooth"});
    mod_settings.set_tooltips({"Sync to DAW tempo", "Enable/disable stereo"});
    
    phase_select.set_tooltips({"Invert Phase"});
    mod_depth.setTooltip("Modulation Depth");
    mod_rate.setTooltip("Modulation Frequency");
    volume.setTooltip("Volume");
     
    volume.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(0.8 - (value / 2.0f), value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("VOL", bounds.getX() + 2, bounds.getY() + 2, 14, 7, Justification::topLeft);
        
        g.strokePath(shape, PathStrokeType(1.0f));
    };
    
    
    shape_select.set_custom_draw([this](int selection, Graphics &g, SelectorButton &b) {
        auto path = Graphs::waveshape_hz(1.0f, selection, b.getWidth(), b.getHeight(), 7.0f);
        
        g.setColour(Colours::white);
        g.strokePath(path, PathStrokeType(1.0f));
    });
    
    
    volume.setRange(0.0f, 1.0f);
    
    delete_slider.setConnectedEdges(12);
    
    delete_slider.onClick = [this]() {
        current_tree.getParent().removeChild(current_tree, nullptr);
        getParentComponent()->repaint();
    };
    
    mod_depth.setRange(0.0f, 1.0f);
    mod_depth.setSkewFactor(0.6);
    
    mod_depth.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds) {

        auto shape = Graphs::sine_to_square(0.7, value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("DTH", bounds.getX() + 2, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0f));
    };
    

    
    mod_rate.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::waveshape_hz(value * -4.0f, 0.0f, bounds.getWidth(), bounds.getHeight(), 8);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("RT", bounds.getX() + 3, bounds.getY() + 2, 17, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0f));
    };
    
    addAndMakeVisible(settings);
    settings.addAndMakeVisible(kind_select);
    settings.addAndMakeVisible(enabled_button);
   
    settings.addAndMakeVisible(volume);
    settings.addAndMakeVisible(phase_select);
    settings.addAndMakeVisible(shape_select);
    settings.addAndMakeVisible(mod_settings);
    settings.addAndMakeVisible(mod_depth);
    settings.addAndMakeVisible(mod_rate);
    settings.addAndMakeVisible(delete_slider);
    
    mod_rate.setSkewFactor(0.6);
    
    mod_settings.callback = [this](std::vector<int> state) {
        if(state[0]) {
            mod_rate.setRange(0, 6, 1);
        }
        else {
            mod_rate.setRange(0.0f, 6.0f, 0.001);
        }
    };
   
    set_selection(nullptr);
}

void XYInspector::allow_stereo(bool allow_stereo) {
    mod_settings.buttons[1]->setEnabled(allow_stereo);
}

void XYInspector::resized()
{
    int margin = 30;
    int item_width = getWidth() - margin;
    int x_pos = margin / 2.0f;
    int item_height = 23;
    
    settings.setBounds(getLocalBounds());
    
    enabled_button.setBounds(0, 0, getWidth() - item_height, item_height);
    delete_slider.setBounds(getWidth() - item_height - 1, 0, item_height, item_height);
    
    kind_select.setBounds(x_pos + item_width / 6.0f - 2, 35, item_width / 1.5f, 20);
    
    volume.setBounds(x_pos, 70, item_width - 27, item_height);
    phase_select.setBounds(x_pos + (item_width - 20), 70, 23, 23);

    shape_select.setBounds(x_pos + item_width / 6.0f, 115, item_width / 1.5f, 20);
    mod_settings.setBounds(x_pos + item_width / 6.0f, 150, item_width / 1.5f, 20);
    
    mod_rate.setBounds(x_pos, 185, item_width, item_height);
    mod_depth.setBounds(x_pos, 220, item_width, item_height);
}

void XYInspector::paint(Graphics& g)
{
    g.fillAll(ColourTheme::bg_light);
    
    if(!selection) {
        g.setColour(Colours::white);
        g.drawText("No Selection", 0, getHeight() / 2.0f, getWidth(), 14, Justification::centred);
    }
}


void XYInspector::set_selection(XYSlider* slider) {
    selection = slider;
    if(selection) {
        settings.setVisible(true);
        attach_to_tree(selection->slider_tree);
        int idx = selection->get_index();
        enabled_button.setButtonText(String(idx + 1));
    }
    else {
        settings.setVisible(false);
    }
    repaint();
}

void XYInspector::attach_to_tree(ValueTree tree)
{
    current_tree = tree;
    volume.getValueObject().referTo(tree.getPropertyAsValue("Volume", nullptr));
    
    auto* parent = findParentComponentOfClass<XYPad>();
    parent->exclude_parameter_change = true;
    
    phase_select.getValueObject().referTo(tree.getPropertyAsValue("Phase", nullptr));
    shape_select.getValueObject().referTo(tree.getPropertyAsValue("ModShape", nullptr));
    kind_select.getValueObject().referTo(tree.getPropertyAsValue("Kind", nullptr));
    
    mod_depth.getValueObject().referTo(tree.getPropertyAsValue("ModDepth", nullptr));
    mod_rate.getValueObject().referTo(tree.getPropertyAsValue("ModRate", nullptr));
    mod_settings.getValueObject().referTo(tree.getPropertyAsValue("ModSettings", nullptr));
    
    enabled_button.getToggleStateValue().referTo(tree.getPropertyAsValue("Enabled", nullptr));

    // Check for sync setting and set range of mod rate appropriately
    if(((int)tree.getProperty("ModSettings")) & 1) {
        mod_rate.setRange(0, 6, 1);
    }
    else {
        mod_rate.setRange(0.0f, 6.0f, 0.001);
    }
    
    parent->exclude_parameter_change = false;

}
