#pragma once

#include <JuceHeader.h>

struct AnimatedSlider : public Slider
{
    
    std::function<void(Graphics&, float, Rectangle<float>)> draw_image;
    
    void paint(Graphics& g) override {
        float height = getHeight();
        float width = getWidth();
        
        float thumb_width = (isHorizontal() ? height : width) * 1.3;
        
        float value = getValue();
        float proportion = valueToProportionOfLength(value);
        
        // Since our handle is a little bigger than juce accounts for, we need to clip 0.03 off the edges
        proportion = std::clamp<float>(proportion, 0.03f, 0.97f);
        
        value = proportionOfLengthToValue(proportion);

        float position = getPositionOfValue(value);
        
        Colour track_colour = findColour(Slider::trackColourId);
        Colour bg_colour = findColour(Slider::backgroundColourId);
        
        if(isHorizontal()) {
            g.setColour(track_colour);
            g.fillRect(Rectangle<float>(0.0, 0.0, position, height));
            
            g.setColour(bg_colour);
            g.fillRect(Rectangle<float>(position, 0.0, width - position, height));
            
            Rectangle<float> bounds = {position - (thumb_width / 2.0f), 0.0f, thumb_width, height};
            
            g.setColour(findColour(Slider::thumbColourId));
            g.fillRect(bounds);
            
            g.setColour(Colours::white);
            g.drawRect(bounds, 1.0);
            
            draw_image(g, proportion, bounds);
            
        }
        else {
            g.setColour(track_colour);
            g.fillRect(Rectangle<float>(0.0, 0.0, width, position));
                        
            g.setColour(bg_colour);
            g.fillRect(Rectangle<float>(0.0, position, width, height - position));
            
            Rectangle<float> bounds = {0.0f, position - (thumb_width / 2.0f), width, thumb_width};
            
            g.setColour(findColour(Slider::thumbColourId));
            g.fillRect(bounds);
            
            g.setColour(Colours::white);
            g.drawRect(bounds, 1.0);
            
            draw_image(g, proportion, bounds);

        }
    }
    
    
    Value filter_type;
        
    AnimatedSlider() {
        
        setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    }
};
