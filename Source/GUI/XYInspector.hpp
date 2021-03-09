//
//  XYPad.hpp
//  Distortion_Modeller - Shared Code
//
//  Created by Tim Schoen on 18/02/2021.
//

#pragma once

#include <JuceHeader.h>
#include "SelectorComponent.hpp"
#include "AnimatedSlider.hpp"

struct XYInspector : public Component
{

    
    XYInspector();
    
    void resized() override;
    
    void paint(Graphics& g) override;
    
    void set_selection(XYSlider* slider);
    
    void attach_to_tree(ValueTree tree);

    ValueTree current_tree;
    SafePointer<XYSlider> selection = nullptr;
    
    AnimatedSlider clarity;
    AnimatedSlider filter_q;
    AnimatedSlider filter_cutoff;
    
    TextButton delete_slider = TextButton("x");
    
    SelectorComponent kind_select = SelectorComponent(2, {"I", "II"});
    SelectorComponent filter_select = SelectorComponent(3, {"LP", "BP", "HP"});
    
    TextButton enabled_button = TextButton("E");
    
    Component settings;
    
    Value filter_selection;
    Value kind_selection;
    
};
