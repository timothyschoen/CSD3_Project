#include "Rate.hpp"
#define QUANT(f1,f2)            float(floor((f1)*(f2)+0.5)/(f2))

void Rate::prepare(const dsp::ProcessSpec& spec) {
    phase.resize(spec.numChannels, 0.0);
    mult.resize(spec.numChannels, 1.0);
    invmult.resize(spec.numChannels, 1.0);
    prev.resize(spec.numChannels, 0.0);
    wantlock.resize(spec.numChannels, 1);
    
    ratio.reset(spec.sampleRate, 0.1);
}

void Rate::set_ratio(float new_ratio) {
    
    ratio.setTargetValue(new_ratio);
}

void Rate::set_stereo(bool stereo) {
    is_stereo = stereo;
}

void Rate::process(dsp::AudioBlock<float>& input_block, dsp::AudioBlock<float>& output_block, int mode) {
    
    if(mode == 0){
        process_cycle(input_block, output_block);
    }
    else {
        process_lock(input_block, output_block);
    }
}


void Rate::process_lock(dsp::AudioBlock<float>& input_block, dsp::AudioBlock<float>& output_block) {
    
    for(int n = 0; n < input_block.getNumSamples(); n++) {
        float current_ratio = ratio.getNextValue();
        
        for(int ch = 0; ch < input_block.getNumChannels(); ch++) {
            float ch_ratio = (is_stereo && (ch & 1)) ? current_ratio : 1.0f / current_ratio;
            float input = input_block.getSample(ch, n);
            
            // did multiplier change?
            if (ch_ratio != mult[ch] && std::isfinite(ch_ratio) && ch_ratio != 0) {
                mult[ch] = ch_ratio;
                invmult[ch] = 1.0f / mult[ch];
                wantlock[ch] = 1;
            }
            float diff = input - prev[ch];
            
            if (diff < -0.5f) {
                diff += 1.0f;
            } else if (diff > 0.5f) {
                diff -= 1.0f;
            }
            
            if (wantlock[ch]) {
                // recalculate phase
                phase[ch] = (input - QUANT(input, quant)) * invmult[ch]
                + QUANT(input, quant * mult[ch]);
                diff = 0;
                wantlock[ch] = 0;
            } else {
                // diff is always between -0.5 and 0.5
                phase[ch] += diff * invmult[ch];
            }
            
            if (phase[ch] > 1.0f || phase[ch] < 0.0f) {
                phase[ch] = phase[ch] - (long)(phase[ch]);
            }
            
            prev[ch] = input;
            
            output_block.setSample(ch, n, phase[ch]);
        }
    }
    //return phase;
    
    
}


void Rate::process_cycle(dsp::AudioBlock<float>& input_block, dsp::AudioBlock<float>& output_block) {
    
    
    for(int n = 0; n < input_block.getNumSamples(); n++) {
        float current_ratio = ratio.getNextValue();
        
        for(int ch = 0; ch < input_block.getNumChannels(); ch++) {
            float ch_ratio = (is_stereo && (ch & 1)) ? current_ratio : 1.0f / current_ratio;
            float input = input_block.getSample(ch, n);
            
            if (ch_ratio != mult[ch] && std::isfinite(ch_ratio) && ch_ratio != 0.0f) {
                mult[ch] = ch_ratio;
                invmult[ch] = 1.0f / mult[ch];
                wantlock[ch] = 1;
            }
            float diff = input - prev[ch];
            
            if (diff < -0.5f) {
                if (wantlock[ch]) {
                    wantlock[ch] = 0;
                    phase[ch] = input * invmult[ch];
                    diff = 0.0f;
                } else {
                    diff += 1.0f;
                }
            } else if (diff > 0.5f) {
                if (wantlock[ch]) {
                    wantlock[ch] = 0;
                    phase[ch] = input * invmult[ch];
                    diff = 0.0f;
                } else {
                    diff -= 1.0f;
                }
            }
            
            // diff is always between -0.5 and 0.5
            phase[ch] += diff * invmult[ch];
            
            if (phase[ch] > 1.0f || phase[ch] < 0.0f) {
                phase[ch] = phase[ch] - (long)(phase[ch]);
            }
            
            prev[ch] = input;
            
            output_block.setSample(ch, n, phase[ch]);
        }
    }
}
