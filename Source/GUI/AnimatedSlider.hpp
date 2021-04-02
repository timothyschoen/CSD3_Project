#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.hpp"

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

            g.setGradientFill(ColourTheme::add_shadow(track_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(0.0f, 0.0f, position, height), 3.0f);
            
            g.setGradientFill(ColourTheme::add_shadow(bg_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(position, 0.0f, width - position, height), 3.0f);
            
            Rectangle<float> bounds = {position - (thumb_width / 2.0f), 0.0f, thumb_width, height};
            
            g.setGradientFill(ColourTheme::add_shadow(theme_colour, height));
            //g.setColour();
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(Colours::white);
            g.drawRoundedRectangle(bounds.reduced(0.0f, 0.5f), 3.0f, 1.0f);
            
            draw_image(g, proportion, bounds);
            
        }
        else {
            //g.setColour(track_colour);
            
            g.setGradientFill(ColourTheme::add_shadow(track_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(0.0f, 0.0f, width, position), 3.0f);
                        
            g.setGradientFill(ColourTheme::add_shadow(bg_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(0.0f, position, width, height - position), 3.0f);
            
            Rectangle<float> bounds = {0.0f, position - (thumb_width / 2.0f), width, thumb_width};
            
            g.setGradientFill(ColourTheme::add_shadow(theme_colour, height));
            g.fillRoundedRectangle(bounds, 3.0f);
            
            
            
            g.setColour(Colours::white);
            g.drawRoundedRectangle(bounds.reduced(2.0f, 2.0f), 3.0f, 1.0f);
            
            draw_image(g, proportion, bounds);

        }
    }
    
    void set_colour(int colour) {
        theme_colour = ColourTheme::highlights[colour];
    }
    
    
    Colour theme_colour = ColourTheme::highlights[0];
    Value filter_type;
        
    AnimatedSlider() {
        
        setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    }
};
