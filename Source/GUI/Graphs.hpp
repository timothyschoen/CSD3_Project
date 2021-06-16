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


struct Graphs
{
    static Path sine_to_square(float scalar_value, float amplitude, int width, int height, int margin) {
        
        scalar_value += 0.1;
        
        amplitude -= 0.1;
        amplitude = std::max(0.0f, amplitude);
        
        int start_point = margin;
        int end_point = width - margin;
        float sine_width = width - (2.474 * margin);
        float half_height = height / 2.0f;
        
        Path shape;
        shape.startNewSubPath(0, half_height);
        shape.lineTo(start_point, half_height);
        
        for(int x = start_point; x < end_point; x += 1) {
            float y = FastMathApproximations::sin(((x - (float)start_point) / sine_width) * MathConstants<float>::twoPi);
            
            y = FastMathApproximations::tanh(y / scalar_value);
            
            y += 1.0f;
            y *= 0.5;
            
            y *= amplitude;
            y += (1.0f - amplitude) * 0.5;
            
            Point<float> sine_point(x, y * height);
            shape.lineTo(sine_point);
        }
        
        shape.lineTo(width - margin, half_height);
        shape.lineTo(width, half_height);
        
        return shape;
    }
    
    static Path waveshape_hz(float scalar_value, float waveshape, int width, int height, int margin) {
        
        int start_point = margin;
        int end_point = width - margin;
        float sine_width = (end_point - start_point) + 1.0f;
        float half_height = height / 2.0f;
        
        Path shape;
        shape.startNewSubPath(0, half_height);
        shape.lineTo(start_point, half_height);
        
        float phase = 0;
        
        for(float x = start_point; x < end_point; x += 0.5) {
            phase += (scalar_value / 2.0f) / sine_width;
            if(phase >= 1.0f) phase -= 1.0f;
            if(phase < 0.0f) phase += 1.0f;

            float y;
            // Sine
            if(waveshape == 0.0f) {
                y = FastMathApproximations::sin(phase * MathConstants<float>::twoPi);
            }
            // Square
            else if(waveshape == 1.0f) {
                y = phase < 0.5f ? -1.0f : 1.0f;
            }
            // Triangle
            else if(waveshape == 2.0f) {
                // Shift phase to align with other shapes
                float shifted_phase = phase + 0.25;
                if(shifted_phase >= 1.0f) shifted_phase -= 1.0f;
                
                y = ((shifted_phase < 0.5) * (4.0f * shifted_phase - 1.0f)) + ((shifted_phase >= 0.5) * (1.0f - 4.0f * (shifted_phase - 0.5)));
            }
            // Sawtooth
            else {
                y = -1.0f + (2.0f * phase);
            }
            
            float amplitude = 0.6;

            y *= -0.5;
            y += 0.5;
            
            y *= amplitude;
            y += (1.0f - amplitude) * 0.5;
            
            Point<float> sine_point(x, y * height);
            shape.lineTo(sine_point);
        }
        
        shape.lineTo(width - margin, half_height);
        
        shape.lineTo(width, half_height);
        
        return shape;
    }
    
    
    
    static Path draw_filter(float cutoff, float resonance, int width, int height, int type, float amp = 0.6) {
        
        std::vector<float> y_pos;
        std::vector<float> x_pos;

        float q = resonance / 3.0f;
        
        Path filter_shape;
        // Draw flat when LPF or HPF is at max
        if((type == 0 && cutoff >= 0.97f) ||
           (type == 2 && cutoff <= 0.04f)) {
            
            filter_shape.startNewSubPath(0.0f, height);
            filter_shape.lineTo(0.0f, (1.0f - amp) * (float)height);
            filter_shape.lineTo(width, (1.0f - amp) * (float)height);
            filter_shape.lineTo(width, height);
            return filter_shape;
        }
        
        if(type == 0) {
            y_pos = {amp, amp, amp + q, 0.0f};
            x_pos = {0.02, 0.8f * cutoff, 1.0f * cutoff, std::min(1.2f * cutoff, 1.0f)};
            filter_shape.startNewSubPath(0.02f, height);
        }
        else if(type == 1) {
            
            cutoff = std::clamp(cutoff, 0.15f, 0.85f);
            
            y_pos = {0.0f, amp + q, amp + q, 0.0f};
            x_pos = {cutoff - 0.15f, cutoff - 0.1f, cutoff + 0.1f, cutoff + 0.15f};
            
            filter_shape.startNewSubPath(0.5f * width, height);
        }
        else {
            y_pos = {amp, amp, amp + q, 0.0f};
            x_pos = {0.98, std::min(1.2f * cutoff, 1.0f), 1.0f * cutoff, 0.8f * cutoff};
            
            filter_shape.startNewSubPath(width - 0.02f, height);
        }
        
        for(int i = 0; i < 4; i++) {
            filter_shape.lineTo(Point<float>(x_pos[i] * width, (1.0f - y_pos[i]) * height));
        }
        
        filter_shape = filter_shape.createPathWithRoundedCorners(2.0f);
        
        return filter_shape;
    }
};
