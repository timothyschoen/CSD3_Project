//
//  XYPad.cpp
//  Zircon
//
//  Created by Tim Schoen on 18/02/2021.
//

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
        slider->setCentrePosition(100, 100);
        
        slider->onClick = [this, slider, xy_pad]() mutable {
            xy_pad->set_selection(slider);
        };
        
        pad.addAndMakeVisible(slider);
        slider->init_valuetree();
        
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
        //if((!even && (i & 1)) || (!odd && !(i & 1))) continue;
        
        g.setColour((i & 1) ? Colours::white.withAlpha((float)0.6) : Colours::white.withAlpha((float)0.3));
        g.drawLine(i * line_spacing, 0, i * line_spacing, getHeight());
    }
}

void XYPad::set_selection(XYSlider* slider)
{
    selection = slider;
    inspector.set_selection(slider);
    update_callback(slider);
    
}


void XYPad::valueTreeChildRemoved (ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {
    inspector.set_selection(nullptr);
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
}

void XYPad::valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {
    std::cout << "check" << std::endl;
}

XYSlider* XYPad::get_selection()
{
    return selection;
}

