//
//  XYPad.hpp
//  Zircon
//
//  Created by Tim Schoen on 18/02/2021.
//

#pragma once
#include <JuceHeader.h>


struct XYSlider : public TextButton, public ValueTree::Listener
{
    ComponentDragger myDragger;
    ComponentBoundsConstrainer constrainer;
    
    ValueTree slider_tree;

    bool enabled = true;
    
    XYSlider(ValueTree tree);
    
    int get_index();
    
    void init_valuetree();
    
    void valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    
    std::pair<float, float> get_xy_values();
    
    void paint(Graphics& g) override;
    
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
};
