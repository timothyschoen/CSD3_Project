//
//  XYPad.hpp
//  Zircon
//
//  Created by Tim Schoen on 18/02/2021.
//

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
