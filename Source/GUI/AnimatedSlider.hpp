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
        
        // Since our thumb is a little bigger than juce accounts for, we need to clip 0.05 off the edges
        proportion = std::clamp<float>(proportion, 0.05f, 0.95f);
        
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
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(Colours::white);
            g.drawRoundedRectangle(bounds.reduced(0.0f, 0.5f), 3.0f, 1.0f);
            
            draw_image(g, proportion, bounds);
        }
        else {
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
