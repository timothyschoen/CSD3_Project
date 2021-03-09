//
//  XYPad.cpp
//  Distortion_Modeller - Shared Code
//
//  Created by Tim Schoen on 18/02/2021.
//

#include "XYPad.hpp"

XYSlider::XYSlider(ValueTree tree) : slider_tree(tree)
{
    setSize(14, 14);
    
    constrainer.setMinimumOnscreenAmounts(10, 10, 10, 10);
    
    setClickingTogglesState(true);
    setRadioGroupId(1);
    
    init_valuetree();
    
}

int XYSlider::get_index() {
    return slider_tree.getParent().indexOf(slider_tree);
}

void XYSlider::init_valuetree()
{
    // check if uninitialised
    if(slider_tree.getNumProperties() == 0)
    {
        auto position_values = get_xy_values();
        slider_tree.setProperty("X", position_values.first, nullptr);
        slider_tree.setProperty("Y", position_values.second, nullptr);
        slider_tree.setProperty("Enabled", true, nullptr);
        slider_tree.setProperty("Kind", 0, nullptr);
        slider_tree.setProperty("FilterType", 0, nullptr);
        slider_tree.setProperty("FilterQ", 0.1, nullptr);
        slider_tree.setProperty("FilterHz", 10000, nullptr);
        slider_tree.setProperty("Clarity", 1.0, nullptr);
        slider_tree.addListener(this);
    }
    else
    {
        slider_tree.addListener(this);
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
    auto background_colour = Colour(131, 37, 251);
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
