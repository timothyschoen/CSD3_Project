//
//  XYPad.hpp
//  Distortion_Modeller - Shared Code
//
//  Created by Tim Schoen on 18/02/2021.
//

#pragma once

#include <JuceHeader.h>
#include "SelectorComponent.hpp"
#include "XYSlider.hpp"
#include "XYInspector.hpp"

struct XYPad : public Component, public ValueTree::Listener
{
    
    Component pad;
    ValueTree pad_tree = ValueTree("XYPad");
    
    XYPad(ValueTree parent_tree);
    
    void mouseDown(const MouseEvent& e) override;
    
    void resized() override;
    
    void paint(Graphics& g) override;
    
    void set_selection(XYSlider* slider);
    
    void valueTreeChildRemoved (ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    
    XYSlider* get_selection();
    
    TextButton new_slider = TextButton("+");
    
    SafePointer<XYSlider> selection = nullptr;
    OwnedArray<XYSlider> sliders;
    
    XYInspector inspector;
    
    std::function<void(XYSlider* slider)> update_callback = [](XYSlider* slider){
    };
    
    int num_lines = 9;
    
};
