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
#include "XYPad.hpp"

XYSlider::XYSlider(ValueTree tree) : slider_tree(tree)
{
    setSize(14, 14);
    
    constrainer.setMinimumOnscreenAmounts(10, 10, 10, 10);
    
    setClickingTogglesState(true);
    setRadioGroupId(1);
}

int XYSlider::get_index() {
    return slider_tree.getParent().indexOf(slider_tree);
}

void XYSlider::init_valuetree()
{
    slider_tree.addListener(this);
    
    // check if uninitialised
    if(slider_tree.getNumProperties() == 0)
    {
        auto position_values = get_xy_values();
        slider_tree.setProperty("X", position_values.first, nullptr);
        slider_tree.setProperty("Y", position_values.second, nullptr);
        slider_tree.setProperty("Enabled", true, nullptr);
        slider_tree.setProperty("Kind", 0, nullptr);
        slider_tree.setProperty("ModShape", 0, nullptr);
        slider_tree.setProperty("ModDepth", 0.1, nullptr);
        slider_tree.setProperty("ModRate", 5.0f, nullptr);
        slider_tree.setProperty("ModSettings", 0, nullptr);
        slider_tree.setProperty("Volume", 1.0f, nullptr);
        slider_tree.setProperty("Phase", false, nullptr);
        
    }
    else {
        slider_tree.sendPropertyChangeMessage("X");
        slider_tree.sendPropertyChangeMessage("Y");
        slider_tree.sendPropertyChangeMessage("Kind");
        slider_tree.sendPropertyChangeMessage("Phase");
        slider_tree.sendPropertyChangeMessage("Volume");
        slider_tree.sendPropertyChangeMessage("ModRate");
        slider_tree.sendPropertyChangeMessage("ModDepth");
        slider_tree.sendPropertyChangeMessage("ModShape");
        slider_tree.sendPropertyChangeMessage("ModSettings");
        slider_tree.sendPropertyChangeMessage("Enabled");
    }
}

void XYSlider::valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property)
{
    if(property == Identifier("X"))
    {
        setTopLeftPosition((float)treeWhosePropertyHasChanged.getProperty("X") * getParentWidth(), getY());
    }
    if(property == Identifier("Y"))
    {
        setTopLeftPosition(getX(), (float)treeWhosePropertyHasChanged.getProperty("Y") * getParentHeight());
    }
    if(property == Identifier("Enabled"))
    {
        enabled = (bool)treeWhosePropertyHasChanged.getProperty("Enabled");
        repaint();
    }
}

std::pair<float, float> XYSlider::get_xy_values()
{
    
    float x_value = (float)getX() / (float)getParentWidth();
    float y_value = (float)getY() / (float)getParentHeight();
    return {x_value, y_value};
}

void XYSlider::paint(Graphics& g)
{
    auto background_colour = ColourTheme::highlights[get_index()];
    auto bounds = getLocalBounds().toFloat();

    auto base_colour = background_colour.withMultipliedSaturation(1.3f).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);

    bool button_down = isDown() || getToggleState();
    
    if (!enabled) base_colour = base_colour.withMultipliedSaturation(0.2f).darker().darker();
    
    if (button_down || isOver())
        base_colour = base_colour.brighter();
    
    Path path;
    path.addEllipse (bounds);
    
    g.setColour (base_colour);
    g.fillPath (path);
    
    if (button_down || isOver()) {
        g.setColour(Colours::white);
        g.drawFittedText(String(get_index() + 1), bounds.reduced(4).toNearestInt(), Justification::centred, 1);
    }

}
void XYSlider::mouseDown (const MouseEvent& e)
{
    myDragger.startDraggingComponent (this, e);
    TextButton::mouseDown(e);
    
}

void XYSlider::mouseDrag (const MouseEvent& e)
{
    myDragger.dragComponent (this, e, &constrainer);
    TextButton::mouseDrag(e);
    
    auto position_values = get_xy_values();
    slider_tree.setProperty("X", position_values.first, nullptr);
    slider_tree.setProperty("Y", position_values.second, nullptr);
}
