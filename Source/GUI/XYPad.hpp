//
//  XYPad.hpp
//  Zircon
//
//  Created by Tim Schoen on 18/02/2021.
//

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
#include "SelectorComponent.hpp"
#include "XYSlider.hpp"
#include "XYInspector.hpp"

struct XYPad : public Component, public ValueTree::Listener
{
    
    bool exclude_parameter_change = false;
    String selection_name;
    String selection_value;
    
    Component pad;
    ValueTree pad_tree = ValueTree("XYPad");
    
    XYPad(ValueTree parent_tree);
    
    void mouseDown(const MouseEvent& e) override;
    
    void resized() override;
    
    void paint(Graphics& g) override;
    
    void set_selection(XYSlider* slider);
    
    void valueTreeChildRemoved (ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    
    void update_tree(ValueTree tree);
    
    XYSlider* get_selection();
    
    TextButton new_slider = TextButton("+");
    
    SafePointer<XYSlider> selection = nullptr;
    OwnedArray<XYSlider> sliders;
    
    XYInspector inspector;
    
    std::function<void(XYSlider* slider)> update_callback = [](XYSlider* slider){
    };
    
    int num_lines = 8;
    
};
