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

struct ColourTheme
{
    inline static Colour main_bg = Colour(33, 34, 43);
    inline static Colour bg_light = Colour(40, 40, 55);
    inline static Colour bg_lighter = Colour(46, 46, 57);
    
    static ColourGradient add_shadow(Colour base, float height) {
        return ColourGradient(base, 0.0f, 0.0f, base.darker(0.3), 0.0f, height, false);
        
    }
    inline static Colour highlights[5] = {Colour(114, 37, 202), Colour(253, 80, 100),  Colour(85, 149, 218),  Colour(60, 193, 83), Colour( 253, 159, 77)};

};


struct Dark_LookAndFeel : public LookAndFeel_V4
{
    
    Dark_LookAndFeel() {
        setColour(TextButton::buttonColourId, ColourTheme::main_bg);
        setColour(TextButton::buttonOnColourId, ColourTheme::highlights[0]);
        
        setColour(Slider::thumbColourId, Colour(131, 37, 251));
        setColour(Slider::trackColourId, Colour(41, 41, 41));
        setColour(Slider::backgroundColourId, Colour(51, 51, 51));
        setColour(ComboBox::outlineColourId, Colours::white);
    }
    
    
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto cornerSize = 2.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        auto baseColour = backgroundColour.withMultipliedSaturation (1.3f)
                                          .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setGradientFill(ColourTheme::add_shadow(baseColour, bounds.getHeight()));

        auto flatOnLeft   = button.isConnectedOnLeft();
        auto flatOnRight  = button.isConnectedOnRight();
        auto flatOnTop    = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
        {
            Path path;
            path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                      bounds.getWidth(), bounds.getHeight(),
                                      cornerSize, cornerSize,
                                      ! (flatOnLeft  || flatOnTop),
                                      ! (flatOnRight || flatOnTop),
                                      ! (flatOnLeft  || flatOnBottom),
                                      ! (flatOnRight || flatOnBottom));

            g.fillPath (path);

            g.setColour (button.findColour (ComboBox::outlineColourId));
            g.strokePath (path, PathStrokeType (1.0f));
        }
        else
        {
            g.fillRoundedRectangle (bounds, cornerSize);

            g.setColour (button.findColour (ComboBox::outlineColourId));
            g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
        }
    }
    
};
