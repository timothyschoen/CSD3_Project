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
#include "AnimatedSlider.hpp"

struct XYInspector : public Component
{
    XYInspector();
    
    void resized() override;
    
    void paint(Graphics& g) override;
    
    void set_selection(XYSlider* slider);
    
    void attach_to_tree(ValueTree tree);
    
    void allow_stereo(bool allow_stereo);
    
    ValueTree current_tree;
    SafePointer<XYSlider> selection = nullptr;
    
    AnimatedSlider volume;
    AnimatedSlider drive;
    AnimatedSlider mod_depth;
    AnimatedSlider mod_rate;
    
    TextButton delete_slider = TextButton("x");
    
    SelectorComponent kind_select = SelectorComponent({"I", "II"});
    
    MultipleSelectorComponent shape_select = MultipleSelectorComponent({"SIN", "TRI", "SQR", "SAW"});
    MultipleSelectorComponent phase_select = MultipleSelectorComponent({juce::CharPointer_UTF8 ("\xc3\x98")});
    MultipleSelectorComponent mod_settings = MultipleSelectorComponent({"SYNC", "STEREO"});
    
    TextButton enabled_button = TextButton("E");
    
    
    
    Component settings;
};
