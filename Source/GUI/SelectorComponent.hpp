#pragma once

#include <JuceHeader.h>
#include "Graphs.hpp"

struct SelectorButton : public TextButton
{
    SelectorButton(String text) : TextButton(text) {
        setClickingTogglesState(true);
    }
    
    bool has_custom_draw = false;
    std::function<void(Graphics&, SelectorButton&)> draw_function;
    
    
    void paint(Graphics& g) override {
        
        paintButton(g, isOver(), isDown());
        
    }
    
    void paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto& lf = getLookAndFeel();

        lf.drawButtonBackground (g, *this,
                                 findColour (getToggleState() ? buttonOnColourId : buttonColourId),
                                 shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        
        if(has_custom_draw) {
            draw_function(g, *this);
        }
        else {
            lf.drawButtonText (g, *this, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }
    
    static void paint_filters(int selection, Graphics& g, SelectorButton& button)
    {
        auto bounds = button.getBounds();
        Path lp_path = Graphs::draw_filter(selection == 1 ? 0.5 : 0.65, 0.0, bounds.getWidth(), bounds.getHeight(), 0);

        Path hp_path = Graphs::draw_filter(selection == 1 ? 1.0 : 0.75 , 0.0, bounds.getWidth(), bounds.getHeight(), 2);
        
        float alpha = button.getToggleState() ? 0.5f : 0.25f;
        
        g.setColour(Colours::white.withAlpha(alpha));
        g.fillPath(lp_path);
        g.setColour(Colours::white);
        g.strokePath(lp_path, PathStrokeType(1.0));
        
        if(selection == 1) {
            Path bp_path = Graphs::draw_filter(0.8, 0.0, bounds.getWidth(), bounds.getHeight(), 1);
            
            g.setColour(Colours::white.withAlpha(alpha));
            g.fillPath(bp_path);
            g.setColour(Colours::white);
            g.strokePath(bp_path, PathStrokeType(1.0));
        }
        
        g.setColour(Colours::white.withAlpha(alpha));
        g.fillPath(hp_path);
        g.setColour(Colours::white);
        g.strokePath(hp_path, PathStrokeType(1.0));
        
    }
    
};
    

struct SelectorComponent : public Component
{
    
    std::function<void(int)> callback = [](int){};
    
    OwnedArray<SelectorButton> buttons;
    
    Value current_selection;
    
    SelectorComponent(int num_options, StringArray titles) {
        
        
        for(int i = 0; i < num_options; i++) {
            auto* button = buttons.add(new SelectorButton(titles[i]));
            button->setRadioGroupId(1110);
            button->setConnectedEdges(3);
            button->onClick = [this, i]() mutable {
                current_selection.setValue(i);
                callback(i);
            };
            addAndMakeVisible(button);
        }
        
        buttons.getFirst()->setConnectedEdges(2);
        buttons.getLast()->setConnectedEdges(1);
        
        current_selection.setValue(0);
        buttons.getFirst()->setToggleState(true, sendNotification);
    }
    
    Value& getValueObject() {
        return current_selection;
    }
    
    void resized() override {
        
        int num_items = buttons.size();
        int width = getWidth() / num_items;
        int height = getHeight();
        
        for(int i = 0; i < num_items; i++) {
            
            buttons[i]->setBounds(i * width, 0, width + 1, height);
        }
    }
    
    void set_custom_draw(std::function<void(int, Graphics&, SelectorButton&)> draw_callback) {
        for(int i = 0; i < buttons.size(); i++) {
            buttons[i]->has_custom_draw = true;
            buttons[i]->draw_function = [this, i, draw_callback](Graphics& g, SelectorButton& button) mutable {
                draw_callback(i, g, button);
                
            };
        }
    }
        
    
    
};
