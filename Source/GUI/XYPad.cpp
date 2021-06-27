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
#include "LookAndFeel.hpp"


XYPad::XYPad(ValueTree parent_tree)
{
    parent_tree.appendChild(pad_tree, nullptr);
    new_slider.setConnectedEdges(12);
    
    new_slider.onClick = [this]() {
        XYPad* xy_pad = this;
        
        ValueTree child_tree = ValueTree("XYSlider");
        pad_tree.appendChild(child_tree, nullptr);
        XYSlider* slider = sliders.add(new XYSlider(child_tree));
        slider->setCentrePosition(160, 100);
        
        slider->onClick = [this, slider, xy_pad]() mutable {
            xy_pad->set_selection(slider);
        };
        
        pad.addAndMakeVisible(slider);
        slider->init_valuetree();
        
        new_slider.setEnabled(pad_tree.getNumChildren() < 5);
    };
    
    pad_tree.addListener(this);
    
    pad.setInterceptsMouseClicks(false, true);
    pad.addAndMakeVisible(new_slider);
    
    addAndMakeVisible(pad);
    addAndMakeVisible(inspector);
    
}

void XYPad::mouseDown(const MouseEvent& e)
{
    if(!selection) return;
    
    selection->setToggleState(false, sendNotification);
    set_selection(nullptr);
}

void XYPad::resized()
{
    int inspector_width = 185;

    pad.setBounds(0, 0, getWidth() - inspector_width, getHeight());
    new_slider.setBounds(pad.getWidth() - 25, pad.getHeight() - 25, 25, 25);
    inspector.setBounds(getWidth() - inspector_width, 0, inspector_width, getHeight());
}

void XYPad::paint(Graphics& g)
{
    g.fillAll(ColourTheme::main_bg);
    
    int line_spacing = pad.getWidth() / num_lines;
    
    for(int i = 0; i < num_lines; i++) {
        g.setColour((i & 1) ? Colours::white.withAlpha((float)0.6) : Colours::white.withAlpha((float)0.3));
        g.drawLine(i * line_spacing, 0, i * line_spacing, getHeight());
    }

    if(selection_name.isEmpty()) return;
    
    g.setColour(ColourTheme::bg_light.withAlpha(0.9f));
    g.fillRect(0, getHeight() - 25, 140, 25);
    
    g.setColour(Colours::white);
    g.drawFittedText(selection_name + ": " + selection_value , 5, getHeight() - 25, 135, 25, Justification::left, 1);
}

void XYPad::set_selection(XYSlider* slider)
{
    selection = slider;
    inspector.set_selection(slider);
    update_callback(slider);
}


void XYPad::valueTreeChildRemoved (ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {
    inspector.set_selection(nullptr);
    new_slider.setEnabled(pad_tree.getNumChildren() < 5);
    sliders.remove(indexFromWhichChildWasRemoved);
}

void XYPad::update_tree(ValueTree tree) {
    sliders.clear();
    
    pad_tree = tree;
    pad_tree.addListener(this);
    
    for(auto child : pad_tree) {
        auto* slider = sliders.add(new XYSlider(child));
        
        XYPad* xy_pad = this;
        slider->onClick = [this, slider, xy_pad]() mutable {
            xy_pad->set_selection(slider);
        };
        
        pad.addAndMakeVisible(slider);
        
        slider->init_valuetree();

    }
    
    new_slider.setEnabled(pad_tree.getNumChildren() < 5);
}

XYSlider* XYPad::get_selection()
{
    return selection;
}

