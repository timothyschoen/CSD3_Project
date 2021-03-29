#pragma once

#include <JuceHeader.h>
#include "Graphs.hpp"
#include "LookAndFeel.hpp"

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
        
        
        Path lp_path = Graphs::draw_filter((0.5f - (selection * 0.1)) * 0.9, 0.0f, bounds.getWidth(), bounds.getHeight(), 0, 0.6);

        Path hp_path = Graphs::draw_filter((0.50 + (selection * 0.1)) * 1.1f, 0.0f, bounds.getWidth(), bounds.getHeight(), 2, 0.6);
        
        float alpha = button.getToggleState() ? 0.5f : 0.25f;
        
        g.setColour(Colours::white.withAlpha(alpha));
        g.fillPath(lp_path);
        g.setColour(Colours::white);
        g.strokePath(lp_path, PathStrokeType(1.0f));
        
        if(selection == 1) {
            Path bp_path = Graphs::draw_filter(0.5f, 0.0f, bounds.getWidth(), bounds.getHeight(), 1, 0.6);
            
            g.setColour(Colours::white.withAlpha(alpha));
            g.fillPath(bp_path);
            g.setColour(Colours::white);
            g.strokePath(bp_path, PathStrokeType(1.0f));
        }
        if(selection == 2) {
            Path bp_path1 = Graphs::draw_filter(0.4, 0.0f, bounds.getWidth(), bounds.getHeight(), 1, 0.6);
            
            g.setColour(Colours::white.withAlpha(alpha));
            g.fillPath(bp_path1);
            g.setColour(Colours::white);
            g.strokePath(bp_path1, PathStrokeType(1.0f));
            
            Path bp_path2 = Graphs::draw_filter(0.6, 0.0f, bounds.getWidth(), bounds.getHeight(), 1, 0.6);
            
            g.setColour(Colours::white.withAlpha(alpha));
            g.fillPath(bp_path2);
            g.setColour(Colours::white);
            g.strokePath(bp_path2, PathStrokeType(1.0f));
        }
        
        g.setColour(Colours::white.withAlpha(alpha));
        g.fillPath(hp_path);
        g.setColour(Colours::white);
        g.strokePath(hp_path, PathStrokeType(1.0f));
        
    }
    
};
    

struct SelectorComponent : public Component, private Value::Listener
{
    
    std::function<void(int)> callback = [](int){};
    
    OwnedArray<SelectorButton> buttons;
    
    Value current_selection;
    
    SelectorComponent(StringArray titles) {
        int num_options = titles.size();
        
        current_selection.addListener(this);
        
        if(num_options > 1) {
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
            buttons.getFirst()->setToggleState(true, sendNotification);
        }
        else {
            auto* button = buttons.add(new SelectorButton(titles[0]));
            button->onClick = [this, button]() mutable {
                bool value = button->getToggleState();
                current_selection.setValue(value);
                callback(value);
            };
            addAndMakeVisible(button);
        }
        
        current_selection.setValue(0);

    }
    
    Value& getValueObject() {
        return current_selection;
    }
    
    void valueChanged(Value& value) override {
        if(buttons.size() > 1) {
            buttons[(int)value.getValue()]->setToggleState(true, dontSendNotification);
        }
        else {
            buttons[0]->setToggleState((bool)value.getValue(), dontSendNotification);
        }
    }
    
    void resized() override {
        
        int num_items = buttons.size();
        int width = getWidth() / num_items;
        int height = getHeight();
        
        for(int i = 0; i < num_items; i++) {
            
            buttons[i]->setBounds(i * width, 0, width + (num_items != (i+1)), height);
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
    
    void set_colour(int new_colour) {
        for(auto& button : buttons) {
            button->setColour(TextButton::buttonOnColourId, ColourTheme::highlights[new_colour]);
        }
    }
    
    void set_tooltips(StringArray tooltips) {
        assert(tooltips.size() == buttons.size());
        
        for(int i = 0; i < tooltips.size(); i++) {
            buttons[i]->setTooltip(tooltips[i]);
        }
    }
    
};


struct MultipleSelectorComponent : public Component, private Value::Listener
{
    
    std::function<void(std::vector<int>)> callback = [](std::vector<int>){};
    
    OwnedArray<SelectorButton> buttons;
    
    Value current_selection;
    
    std::vector<int> state;
    
    MultipleSelectorComponent(StringArray titles) {
        int num_options = titles.size();
        state.resize(num_options, 0.0f);
        
        current_selection.addListener(this);
        
        for(int i = 0; i < num_options; i++) {
            auto* button = buttons.add(new SelectorButton(titles[i]));
            button->setConnectedEdges(3);
            button->setClickingTogglesState(true);
            button->onClick = [this, i, button]() mutable {
                int binary_state = 0;
                for(int j = 0; j < state.size(); j++) {
                    state[j] = buttons[j]->getToggleState();
                    binary_state |= state[j] * (1<<j);
                }
                current_selection.setValue(binary_state);
                callback(state);
            };
            addAndMakeVisible(button);
        }
        
        if(num_options > 1) {
            buttons.getFirst()->setConnectedEdges(2);
            buttons.getLast()->setConnectedEdges(1);
        }
    }
    
    Value& getValueObject() {
        return current_selection;
    }
    
    void set_tooltips(StringArray tooltips) {
        assert(tooltips.size() == buttons.size());
        
        for(int i = 0; i < tooltips.size(); i++) {
            buttons[i]->setTooltip(tooltips[i]);
        }
    }
    
    void valueChanged(Value& value) override {
        int flag = (int)value.getValue();
        for(int i = 0; i < state.size(); i++) {
            state[i] = flag & (1<<i);
            buttons[i]->setToggleState(state[i], dontSendNotification);
        }
    }

    void resized() override {
        
        int num_items = buttons.size();
        int width = getWidth() / num_items;
        int height = getHeight();
        
        for(int i = 0; i < num_items; i++) {
            buttons[i]->setBounds(i * width, 0, width + (num_items != (i+1)), height);
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
    
    void set_colour(int new_colour) {
        for(auto& button : buttons) {
            button->setColour(TextButton::buttonOnColourId, ColourTheme::highlights[new_colour]);
        }
    }
    
    
};
