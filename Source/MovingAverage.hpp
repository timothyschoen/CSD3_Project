/*
 * Golden Hearing - Golden Booster (2021)
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cassert>

template<int period>
struct MovingAverage {
    
    MovingAverage() : head(NULL), tail(NULL), total(0) {
        assert(period >= 1);
        
        std::fill(window, window + period, 0.0f);
    }
 
    // Adds a value to the average, pushing one out if nescessary
    void add(float val) {
        // Special case: Initialization
        if (head == NULL) {
            head = window;
            *head = val;
            tail = head;
            inc(tail);
            total = val;
            return;
        }
 
        // Were we already full?
        if (head == tail) {
            // Fix total-cache
            total -= *head;
            // Make room
            inc(head);
        }
 
        // Write the value in the next spot.
        *tail = val;
        inc(tail);
 
        // Update our total-cache
        total += val;
    }
 
    // Returns the average of the last P elements added to this SMA.
    // If no elements have been added yet, returns 0.0
    float avg() const {
        ptrdiff_t size = this->size();
        if (size == 0) {
            return 0; // No entries => 0 average
        }
        return total / (float) size; // Cast to float for floating point arithmetic
    }
    
    float stdev()
    {
        float mean = avg();
        float var = 0.0f;
        
        for(int n = 0; n < period; n++) {
            var += (window[n] - mean) * (window[n] - mean);
        }
        
        var /= period;
        return sqrt(var);
    }
 
    void clear() {
        std::fill(window, window + period, 0.0f);
        head = NULL;
        tail = NULL;
        total = 0.0;
    }

private:
    float window[period]; // Holds the values to calculate the average of.
 
    // Logically, head is before tail
    float* head; // Points at the oldest element we've stored.
    float* tail; // Points at the newest element we've stored.
 
    double total; // Cache the total so we don't sum everything each time.
 
    // Bumps the given pointer up by one.
    // Wraps to the start of the array if needed.
    void inc(float * & p) {
        if (++p >= window + period) {
            p = window;
        }
    }
 
    // Returns how many numbers we have stored.
    ptrdiff_t size() const {
        if (head == NULL)
            return 0;
        if (head == tail)
            return period;
        return (period + tail - head) % period;
    }
};
