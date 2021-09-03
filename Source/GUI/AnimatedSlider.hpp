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
    std::function<void(Graphics&, float, Rectangle<float>)> draw_image_max;
    
    void paint(Graphics& g) override {
        float height = getHeight();
        float width = getWidth();
        
        Colour track_colour = findColour(Slider::trackColourId);
        Colour bg_colour = findColour(Slider::backgroundColourId);
        
        if(isHorizontal() && !isTwoValue()) {
            auto [position, proportion] = value_to_position(getValue());

            
            g.setGradientFill(ColourTheme::add_shadow(track_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(0.0f, 0.0f, position, height), 3.0f);
            
            g.setGradientFill(ColourTheme::add_shadow(bg_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(position, 0.0f, width - position, height), 3.0f);
            
            draw_horizontal_slider(g, getValue(), draw_image);
        }
        else if(isHorizontal() && isTwoValue()) {
            auto [pos_1, prop_1] = value_to_position(getMinValue());
            auto [pos_2, prop_2] = value_to_position(getMaxValue());
            
            g.setGradientFill(ColourTheme::add_shadow(bg_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(0.0f, 0.0f, pos_1, height), 3.0f);
            
            g.setGradientFill(ColourTheme::add_shadow(track_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(pos_1, 0.0f, pos_2, height), 3.0f);
            
            g.setGradientFill(ColourTheme::add_shadow(bg_colour, height));
            g.fillRoundedRectangle(Rectangle<float>(pos_2, 0.0f, width, height), 3.0f);
            
            draw_horizontal_slider(g, getMinValue(), draw_image);
            draw_horizontal_slider(g, getMaxValue(), draw_image_max);
        }
    }
    
    std::pair<float, float> value_to_position(float value) {
        float width = getWidth();
        
        float proportion = valueToProportionOfLength(value);
        
        float thumb_extension = (thumb_width - (getHeight() * 0.5)) / 3.25;
        
        if(proportion * width < thumb_extension) {
            proportion = thumb_extension / width;
        }
        if(proportion * width > width - thumb_extension) {
            proportion = (width - thumb_extension) / width;
        }
    
        value = proportionOfLengthToValue(proportion);

        return {getPositionOfValue(value), proportion};
    }
    
    void set_colour(int colour) {
        theme_colour = ColourTheme::highlights[colour];
    }
    
    void draw_horizontal_slider(Graphics& g, float value, std::function<void(Graphics&, float, Rectangle<float>)>& draw_func) {
        
        auto [position, proportion] = value_to_position(value);
        
        float height = getHeight();
        float width = getWidth();

        
        Rectangle<float> bounds = {position - (thumb_width / 2.0f), 0.0f, thumb_width, height};
        
        g.setGradientFill(ColourTheme::add_shadow(theme_colour, height));
        g.fillRoundedRectangle(bounds, 3.0f);
        
        g.setColour(Colour(178, 178, 178));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f, 0.5f), 3.0f, 1.0f);
        
        g.setColour(Colours::white);
        g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), 3.0f, 1.0f);

        
        draw_func(g, proportion, bounds);
    }
    
    
    Colour theme_colour = ColourTheme::highlights[0];
    Value filter_type;
    
    static constexpr float thumb_width = 35.0;
        
    AnimatedSlider() {
        setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    }
};
