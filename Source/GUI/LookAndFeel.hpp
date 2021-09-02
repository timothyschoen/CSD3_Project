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

struct ColourTheme
{
    inline static Colour main_bg = Colour(32, 32, 32);
    inline static Colour bg_light = Colour(48, 48, 48);
    inline static Colour bg_lighter = Colour(54, 54, 54);
    
    static ColourGradient add_shadow(Colour base, float height) {
        //return ColourGradient(base, 0.0f, 0.0f, base.darker(0.3), 0.0f, height, false);
        return ColourGradient(base, 0.0f, 0.0f, base, 0.0f, height, false); // disabled for now
        
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
        setColour(ComboBox::outlineColourId, Colour(178, 178, 178));
        
        setColour (ListBox::backgroundColourId, Colour(50, 50, 50));
        setColour(PopupMenu::backgroundColourId, Colour(30, 30, 30).withAlpha(0.9f));
        setColour (PopupMenu::highlightedBackgroundColourId, Colour(41, 41, 41));
        
        setColour(ComboBox::backgroundColourId, Colour(41, 41, 41));
        
        setColour(ResizableWindow::backgroundColourId, Colour(41, 41, 41));
    }
    
    
    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, const Image* icon, bool drawTitleTextOnLeft) override
    {
        if (w * h == 0)
            return;
        
        auto isActive = window.isActiveWindow();
        
        g.setColour (Colour(41, 41, 41));
        g.fillAll();
        
        Font font ((float) h * 0.65f, Font::plain);
        g.setFont (font);
        
        auto textW = font.getStringWidth (window.getName());
        auto iconW = 0;
        auto iconH = 0;
        
        if (icon != nullptr)
        {
            iconH = static_cast<int> (font.getHeight());
            iconW = icon->getWidth() * iconH / icon->getHeight() + 4;
        }
        
        textW = jmin (titleSpaceW, textW + iconW);
        auto textX = drawTitleTextOnLeft ? titleSpaceX
        : jmax (titleSpaceX, (w - textW) / 2);
        
        if (textX + textW > titleSpaceX + titleSpaceW)
            textX = titleSpaceX + titleSpaceW - textW;
        
        if (icon != nullptr)
        {
            g.setOpacity (isActive ? 1.0f : 0.6f);
            g.drawImageWithin (*icon, textX, (h - iconH) / 2, iconW, iconH,
                               RectanglePlacement::centred, false);
            textX += iconW;
            textW -= iconW;
        }
        
        if (window.isColourSpecified (DocumentWindow::textColourId) || isColourSpecified (DocumentWindow::textColourId))
            g.setColour (window.findColour (DocumentWindow::textColourId));
        else
            g.setColour (getCurrentColourScheme().getUIColour (ColourScheme::defaultText));
        
        g.drawText (window.getName(), textX, 0, textW, h, Justification::centredLeft, true);
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
    
    void drawComboBox (Graphics& g, int width, int height, bool,
                       int, int, int, int, ComboBox& box) override
    {
        auto cornerSize = 3.0;
        Rectangle<int> boxBounds (0, 0, width, height);
        
        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
        
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);
        
        Rectangle<int> arrowZone (width - 23, 2, 14, height - 4);
        Path path;
        path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 2.0f);
        path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
        path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 2.0f);
        
        g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath (path, PathStrokeType (2.0f));
    }
    
    Font getComboBoxFont(ComboBox& box) override {
        return { jmin (16.0f, box.getHeight() * 0.7f) };
    }
    
    static void make_transparent(Component& component) {
        component.setColour(ComboBox::backgroundColourId, Colours::transparentBlack);
        component.setColour(ComboBox::outlineColourId, Colours::transparentWhite);
        component.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        component.setColour(TextButton::buttonOnColourId, Colours::transparentWhite);
    }
    
};
