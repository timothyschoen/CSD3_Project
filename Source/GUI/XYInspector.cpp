//
//  XYPad.cpp
//  Distortion_Modeller - Shared Code
//
//  Created by Tim Schoen on 18/02/2021.
//

#include "XYPad.hpp"
#include "Graphs.hpp"
#include "../LookAndFeel.hpp"

XYInspector::XYInspector() {
    
    enabled_button.setClickingTogglesState(true);
    enabled_button.setConnectedEdges(12);
    
    
    clarity.set_colour(0);
    kind_select.set_colour(0);

    mod_depth.set_colour(2);
    mod_rate.set_colour(2);
    filter_select.set_colour(2);
    
    clarity.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::sine_to_square(value, 0.7, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("SHP", bounds.getX() + 2, bounds.getY() + 2, 14, 7, Justification::topLeft);
        
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    
    clarity.setRange(0.0, 1.0);
    
    
    
    filter_select.callback = [this](int option) {
        filter_selection.setValue(option);
        mod_rate.filter_type.setValue(option);
        mod_depth.filter_type.setValue(option);
        repaint();
    };
    
    kind_select.callback = [this](int option) {
        kind_selection.setValue(option);
        repaint();
    };
    
    delete_slider.setConnectedEdges(12);
    
    delete_slider.onClick = [this]() {
        current_tree.getParent().removeChild(current_tree, nullptr);
    };
    
    mod_depth.setRange(0.0, 1.0);
    mod_depth.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){

        auto shape = Graphs::sine_to_square(0.7, value, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("DTH", bounds.getX() + 2, bounds.getY() + 2, 14, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    mod_rate.setRange(1.0, 8.0);
    mod_rate.setSkewFactor(0.25);
    mod_rate.draw_image = [this](Graphics& g, float value, Rectangle<float> bounds){
        auto shape = Graphs::waveshape_hz(value, 0.0, bounds.getWidth(), bounds.getHeight(), 3);
        
        shape.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
        
        g.setColour(Colours::white.withAlpha(0.5f));
        g.fillPath(shape);
        
        g.setFont(Font(7));
        
        g.setColour(Colours::white);
        g.drawText("RT", bounds.getX() + 3, bounds.getY() + 2, 17, 7, Justification::topLeft);
        g.strokePath(shape, PathStrokeType(1.0));
    };
    
    addAndMakeVisible(settings);
    settings.addAndMakeVisible(kind_select);
    settings.addAndMakeVisible(enabled_button);
    settings.addAndMakeVisible(filter_select);
    settings.addAndMakeVisible(clarity);
    settings.addAndMakeVisible(mod_depth);
    settings.addAndMakeVisible(mod_rate);
    settings.addAndMakeVisible(delete_slider);
    
    set_selection(nullptr);
    
    
}

void XYInspector::resized()
{
    int item_width = getWidth() - 20;
    int item_height = 23;
    
    settings.setBounds(getLocalBounds());
    
    enabled_button.setBounds(10, 10, getWidth() - 33, item_height);
    delete_slider.setBounds(getWidth() - 34, 10, item_height, item_height);
    
    kind_select.setBounds(10 + item_width / 6.0, 40, (item_width / 1.5) - 1.0, 20);
    
    clarity.setBounds(10, 65, item_width, item_height);
    

    filter_select.setBounds(10 + item_width / 6.0, 105, item_width / 1.5, 20);
    mod_rate.setBounds(10, 135, item_width, item_height);
    mod_depth.setBounds(10, 165, item_width, item_height);
    
    
}

void XYInspector::paint(Graphics& g)
{
    g.fillAll(ColourTheme::bg_light);
    
    if(!selection) {
        g.setColour(Colours::white);
        g.drawText("No Selection", 0, getHeight() / 2.0, getWidth(), 14, Justification::centred);
    }

}


void XYInspector::set_selection(XYSlider* slider) {
    selection = slider;
    if(selection) {
        settings.setVisible(true);
        attach_to_tree(selection->slider_tree);
        int idx = selection->get_index();
        enabled_button.setButtonText(String(idx + 1));
        //enabled_button.setColour(TextButton::buttonOnColourId, ColourTheme::highlights[idx]);
    }
    else {
        settings.setVisible(false);
    }
    repaint();
}

void XYInspector::attach_to_tree(ValueTree tree)
{
    current_tree = tree;
    clarity.getValueObject().referTo(tree.getPropertyAsValue("Clarity", nullptr));
    
    filter_selection.referTo(tree.getPropertyAsValue("FilterType", nullptr));
    
    mod_depth.getValueObject().referTo(tree.getPropertyAsValue("ModDepth", nullptr));
    
    mod_rate.getValueObject().referTo(tree.getPropertyAsValue("ModRate", nullptr));
    
    
    kind_selection.referTo(tree.getPropertyAsValue("Kind", nullptr));
    enabled_button.getToggleStateValue().referTo(tree.getPropertyAsValue("Enabled", nullptr));
}
