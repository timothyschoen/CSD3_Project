#pragma once

#include <JuceHeader.h>
#include "Hilbert.hpp"

// Time-scaling for phasor ramps, similar to Max/MSP's rate~ object

struct Rate
{
    void prepare(const dsp::ProcessSpec& spec);
    
    void set_ratio(float new_ratio);
    void set_stereo(bool stereo);
    
    void process(dsp::AudioBlock<float>& input_block, dsp::AudioBlock<float>& output_block, int mode = 0);
    
    void process_lock(dsp::AudioBlock<float>& input_block, dsp::AudioBlock<float>& output_block);
    void process_cycle(dsp::AudioBlock<float>& input_block, dsp::AudioBlock<float>& output_block);
    
    
    bool is_stereo = false;
    SmoothedValue<float> ratio = 1;
    std::vector<float> phase, mult, invmult, prev;
    std::vector<int> wantlock;
    int quant = 1;
    
};


/*
 inline float process_cycle(float input, float ratio) {
 
 if (ratio != mult && std::isfinite(ratio) && ratio != 0.0f) {
 mult = ratio;
 invmult = 1.0f / mult;
 wantlock = 1;
 }
 float diff = input - prev;
 
 if (diff < -0.5f) {
 if (wantlock) {
 wantlock = 0;
 phase = input * invmult;
 diff = 0.0f;
 } else {
 diff += 1.0f;
 }
 } else if (diff > 0.5f) {
 if (wantlock) {
 wantlock = 0;
 phase = input * invmult;
 diff = 0.0f;
 } else {
 diff -= 1.0f;
 }
 }
 
 // diff is always between -0.5 and 0.5
 phase += diff * invmult;
 
 if (phase > 1.0f || phase < 0.0f) {
 phase = phase - (long)(phase);
 }
 
 prev = input;
 
 return phase;
 }
 
 
 
 inline float process_lock(float input, float ratio) {
 // did multiplier change?
 if (ratio != mult && std::isfinite(ratio) && ratio != 0) {
 mult = ratio;
 invmult = 1.0f / mult;
 wantlock = 1;
 }
 float diff = input - prev;
 
 if (diff < -0.5f) {
 diff += 1.0f;
 } else if (diff > 0.5f) {
 diff -= 1.0f;
 }
 
 if (wantlock) {
 // recalculate phase
 phase = (input - QUANT(input, quant)) * invmult
 + QUANT(input, quant * mult);
 diff = 0;
 wantlock = 0;
 } else {
 // diff is always between -0.5 and 0.5
 phase += diff * invmult;
 }
 
 if (phase > 1.0f || phase < 0.0f) {
 phase = phase - (long)(phase);
 }
 
 prev = input;
 
 return phase;
 }
 
 inline float process_off(float input, float ratio) {
 // did multiplier change?
 if (ratio != mult && std::isfinite(ratio) && ratio != 0.0) {
 mult = ratio;
 invmult = 1.0 / mult;
 wantlock = 1;
 }
 float diff = input - prev;
 
 if (diff < float(-0.5)) {
 diff += float(1);
 } else if (diff > float(0.5)) {
 diff -= float(1);
 }
 
 phase += diff * invmult;
 
 if (phase > float(1.) || phase < float(-0.)) {
 phase = phase - (long)(phase);
 }
 
 prev = input;
 
 return phase;
 } */
