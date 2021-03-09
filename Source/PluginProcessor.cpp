/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"


//==============================================================================
Distortion_ModellerAudioProcessor::Distortion_ModellerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
   
}

Distortion_ModellerAudioProcessor::~Distortion_ModellerAudioProcessor()
{
    ChebyshevFactory::deallocate();
}

//==============================================================================
const juce::String Distortion_ModellerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Distortion_ModellerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Distortion_ModellerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Distortion_ModellerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Distortion_ModellerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Distortion_ModellerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Distortion_ModellerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Distortion_ModellerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Distortion_ModellerAudioProcessor::getProgramName (int index)
{
    return {};
}

void Distortion_ModellerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void Distortion_ModellerAudioProcessor::update_bands(double freq_range_overlap)
{

    num_bands = filter_bank->InitWithFreqRangeOverlap(1, 20000, freq_range_overlap - 0.9);
    
    int old_num_bands = chebyshev_distortions.size();
    int num_channels = getTotalNumInputChannels();
    //band_filter.clear();
    
    
    if(old_num_bands > num_bands) {
            for(int b = num_bands; b < old_num_bands; b++) {
                chebyshev_distortions.resize(num_bands);
            }
    }
    else if(old_num_bands < num_bands) {
            chebyshev_distortions.resize(num_bands);
            for(int b = old_num_bands; b < num_bands; b++) {
                chebyshev_distortions[b].resize(chebyshev_distortions[0].size());
                
                for(int i = 0; i < chebyshev_distortions[0].size(); i++)
                {
                    chebyshev_distortions[b][i].reset(new ChebyshevTable({sample_rate, (juce::uint32)block_size, (juce::uint32)getTotalNumInputChannels()}, chebyshev_distortions[0][i]->shift, chebyshev_distortions[0][i]->g));
                }
            }
    }
    
    for(int ch = 0; ch < num_channels; ch++) {
        //auto* filter = band_filter.add(new ResonBands(sample_rate));
        //filter->create_bands(num_bands, {80, 12000});
    };

    band_data.clear();
    band_data.resize(num_bands);
    
    split_bands.resize(num_bands);
    
    for(int i = 0; i < num_bands; i++) {
        split_bands[i] = dsp::AudioBlock<float>(band_data[i], num_channels, block_size);
    }

   harmonics_buffer.setSize(num_channels, block_size);
    
}

//==============================================================================
void Distortion_ModellerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    block_size = samplesPerBlock;
    sample_rate = sampleRate;
    //FloatVectorOperations::enableFlushToZeroMode(true);
    
    int num_channels = getTotalNumInputChannels();
    
    temp_buffer = dsp::AudioBlock<float>(temp_storage, num_channels, block_size);
    
    filter_bank.reset(new GammatoneFilterBank(sample_rate, samplesPerBlock, num_channels));
    
    tone_filter.state->type = dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
    
    tone_filter.state->setCutOffFrequency(sample_rate, 15000);
    
    tone_filter.prepare({sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumInputChannels()});
    

    
    update_bands(0.0);

}

void Distortion_ModellerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Distortion_ModellerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Distortion_ModellerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Check for parameter changes
    std::function<void()> action;
    while(queue.try_dequeue(action)) {
        action();
    }

    harmonics_buffer.clear();
    
    dsp::AudioBlock<float> in_block(buffer);
    
    int num_samples = buffer.getNumSamples();
    
    filter_bank->process(in_block, split_bands);
    
    
    for(int i = 0; i < num_bands; i++) {
            for(int h = 0; h < chebyshev_distortions[i].size(); h++) {
                double centre_freq = filter_bank->filters[0][i]->GetCenterFrequency();
                
                // Trick to prevent aliasing:
                // When shift * filter_upper_bound >= nyquist, we're aliasing
                // We approximate the upper bound of the filter by taking center_freq * 3.5
                // I'd rather leave out some harmonics than introduce aliasing!
                // Could be lower if oversampling would work...
                
                // TODO: Make this more like a curve
                if((centre_freq * 3.5) * chebyshev_distortions[i][h]->shift >= sample_rate / 2.0) continue;

                //temp_buffer.copyFrom(split_bands[i]);
                //temp_buffer *=

                chebyshev_distortions[i][h]->process(split_bands[i], temp_buffer, num_samples);
                
                for(int ch = 0; ch < getTotalNumInputChannels(); ch++) {
                    harmonics_buffer.addFrom(ch, 0, temp_buffer.getChannelPointer(ch), num_samples);
                }

            }
        
        }
    for(int ch = 0; ch < getTotalNumInputChannels(); ch++) {
        buffer.copyFrom(ch, 0, harmonics_buffer, ch, 0, num_samples);
        
    }
    
    auto block = dsp::AudioBlock<float>(buffer);
    tone_filter.process(dsp::ProcessContextReplacing<float>(block));
    }

void Distortion_ModellerAudioProcessor::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    if(childWhichHasBeenRemoved.getType() == Identifier("XYSlider")) {
        queue.enqueue([this, indexFromWhichChildWasRemoved]() mutable {
            delete_harmonic(indexFromWhichChildWasRemoved);
        });
    }
    
}

void Distortion_ModellerAudioProcessor::valueTreePropertyChanged (ValueTree &treeWhosePropertyHasChanged, const Identifier &property)
{
    float value = treeWhosePropertyHasChanged.getProperty(property);
    if(treeWhosePropertyHasChanged.getType() == Identifier("XYSlider")) {
        int slider_idx = treeWhosePropertyHasChanged.getParent().indexOf(treeWhosePropertyHasChanged);
        if(property == Identifier("X") || property == Identifier("Y") || property == Identifier("Kind")) {
            float x_value = treeWhosePropertyHasChanged.getProperty("X");
            float y_value = treeWhosePropertyHasChanged.getProperty("Y");
            bool kind = treeWhosePropertyHasChanged.getProperty("Kind");
            HarmonicResponse harmonic = {x_value * 13.5, (1.0 - y_value) * 1.5, kind};
            
            queue.enqueue([this, harmonic, slider_idx]() mutable {
                set_harmonic(slider_idx, harmonic);
            });
        }
        else if(property == Identifier("FilterQ")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                for(int b = 0; b < num_bands; b++) {
                    float cutoff = chebyshev_distortions[b][slider_idx]->last_cutoff;
                    chebyshev_distortions[b][slider_idx]->set_filter_cutoff(cutoff, value);
                }
            });
        }
        else if(property == Identifier("FilterType")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                for(int b = 0; b < num_bands; b++) {
                    chebyshev_distortions[b][slider_idx]->set_filter_type((int)value);
                }
            });
        }
        else if(property == Identifier("Enabled")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                for(int b = 0; b < num_bands; b++) {
                    chebyshev_distortions[b][slider_idx]->enabled = value != 0.0;
                }
            });
        }
        else if(property == Identifier("FilterHz")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                for(int b = 0; b < num_bands; b++) {
                    float q =  chebyshev_distortions[b][slider_idx]->last_q;
                    chebyshev_distortions[b][slider_idx]->set_filter_cutoff(value, q);
                }
            });
        }
        else if(property == Identifier("Clarity")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                for(int b = 0; b < num_bands; b++) {
                    chebyshev_distortions[b][slider_idx]->set_scaling(value);
                }
            });
        }
    }
    else if(property == Identifier("Intermodulation")) {
        queue.enqueue([this, value]() mutable {
            update_bands(value);
        });
    }
    else if(property == Identifier("Tone")) {
        queue.enqueue([this, value]() mutable {
            value += 1e-8;
            tone_filter.state->setCutOffFrequency(sample_rate, value * 20000);

        });
    }
    else if(property == Identifier("Gain")) {
        queue.enqueue([this, value]() mutable {
            for(int b = 0; b < num_bands; b++) {
                for(int s = 0; s < chebyshev_distortions[b].size(); s++) {
                    chebyshev_distortions[b][s]->set_gain(value);
                }
            }
            
        });
    }
}

void Distortion_ModellerAudioProcessor::delete_harmonic(int idx) {
    for(int b = 0; b < num_bands; b++) {
        chebyshev_distortions[b].erase(chebyshev_distortions[b].begin() + idx);
    }
}

void Distortion_ModellerAudioProcessor::set_harmonic(int idx, HarmonicResponse response) {
    
    if(idx >= chebyshev_distortions[0].size() || idx < 0) {
        auto& [shift, gain, kind] = response;
        
        for(int b = 0; b < num_bands; b++) {
            chebyshev_distortions[b].push_back(std::unique_ptr<ChebyshevTable>(new ChebyshevTable({sample_rate, (juce::uint32)block_size, (juce::uint32)getTotalNumInputChannels()}, shift, gain, kind)));
        }
    }
    else
    {
        auto& [shift, gain, kind] = response;
        for(int b = 0; b < num_bands; b++) {
            chebyshev_distortions[b][idx]->set_table(shift, gain, kind);
        }
        
        
        
    }
}

//==============================================================================
bool Distortion_ModellerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Distortion_ModellerAudioProcessor::createEditor()
{
    return new Distortion_ModellerAudioProcessorEditor (*this);
}

//==============================================================================
void Distortion_ModellerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Distortion_ModellerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Distortion_ModellerAudioProcessor();
}
