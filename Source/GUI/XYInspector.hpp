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
    AnimatedSlider mod_depth;
    AnimatedSlider mod_rate;
    
    TextButton delete_slider = TextButton("x");
    
    SelectorComponent kind_select = SelectorComponent({"I", "II"});
    MultipleSelectorComponent shape_select = MultipleSelectorComponent({"SIN", "TRI", "SQR", "SAW", });
    
    MultipleSelectorComponent mod_settings = MultipleSelectorComponent({"SYNC", "STEREO"});
    MultipleSelectorComponent even_selector = MultipleSelectorComponent({"EVEN", "ODD"});
    
    TextButton enabled_button = TextButton("E");
    
    Component settings;
};
