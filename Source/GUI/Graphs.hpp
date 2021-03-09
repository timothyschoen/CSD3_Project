#pragma once
#include <JuceHeader.h>


struct Graphs
{
    static Path sine_to_square(float scalar_value, float amplitude, int width, int height, int margin) {
        
        scalar_value += 0.1;
        
        int start_point = margin;
        int end_point = width - margin;
        float sine_width = width - (2.474 * margin);
        float half_height = height / 2.0;
        
        Path shape;
        shape.startNewSubPath(0, half_height);
        shape.lineTo(start_point, half_height);
        
        for(int x = start_point; x < end_point; x += 1) {
            float y = dsp::FastMathApproximations::sin(((x - (float)start_point) / sine_width) * 2.0 * M_PI);
            
            y = dsp::FastMathApproximations::tanh(y / scalar_value);
            
            y += 1.0;
            y *= 0.5;
            
            y *= amplitude;
            y += (1.0 - amplitude) * 0.5;
            
            Point<float> sine_point(x, y * height);
            shape.lineTo(sine_point);
        }
        
        shape.lineTo(width - margin, half_height);
        
        shape.lineTo(width, half_height);
        
        return shape;
    }
    
    
    
    static Path draw_filter(float cutoff, float resonance, int width, int height, int type) {
        
        std::vector<float> y_pos;
        std::vector<float> x_pos;
        
        float amp = 0.6f;
        float q = resonance / 3.0;
        
        Path filter_shape;
        // Draw flat when LPF or HPF is at max
        if((type == 0 && cutoff >= 0.97f) ||
           (type == 2 && cutoff <= 0.04f)) {
            
            filter_shape.startNewSubPath(0.0, height);
            filter_shape.lineTo(0.0, (1.0 - amp) * (float)height);
            filter_shape.lineTo(width, (1.0 - amp) * (float)height);
            filter_shape.lineTo(width, height);
            return filter_shape;
        }
        
        if(type == 0) {
            y_pos = {amp, amp, amp + q, 0.0f};
            x_pos = {0.02f, 0.4f * cutoff, 0.7f * cutoff, 0.95f * cutoff};
            filter_shape.startNewSubPath(0.02f, height);
            
        }
        else if(type == 1) {
            cutoff *= 1.3f;
            y_pos = {0.0f, amp + q, amp + q, 0.0f};
            x_pos = {0.7f*cutoff, 0.6f * cutoff, 0.4f * cutoff, 0.3f * cutoff};
            
            filter_shape.startNewSubPath(0.5 * width, height);
            
        }
        else {
            y_pos = {amp, amp, amp + q, 0.0f};
            x_pos = {0.98f, 0.8f * cutoff, 0.7f * cutoff, 0.5f * cutoff};
            
            filter_shape.startNewSubPath(width - 0.02f, height);
        }
        
        for(int i = 0; i < 4; i++) {
            filter_shape.lineTo(Point<float>(x_pos[i] * width, (1.0 - y_pos[i]) * height));
        }
        
        filter_shape = filter_shape.createPathWithRoundedCorners(2.0);
        
        return filter_shape;
    }

};
