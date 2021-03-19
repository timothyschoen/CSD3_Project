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
   
   
   main_tree.addListener(this);
   // init valuetree and parameters here!!
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

void Distortion_ModellerAudioProcessor::update_bands(float freq_range_overlap)
{
   num_bands = filter_bank->InitWithFreqRangeOverlap(20, sample_rate / 2.0, freq_range_overlap - 0.9);
   
   int old_num_bands = (int)chebyshev_distortions.size();
   int num_channels = getTotalNumInputChannels();
   
   dsp::ProcessSpec spec = {sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, (juce::uint32)getTotalNumInputChannels()};
   
   if(old_num_bands > num_bands) {
      chebyshev_distortions.resize(num_bands);
      peak_scalers.resize(num_bands);
   }
   else if(old_num_bands < num_bands) {
      chebyshev_distortions.resize(num_bands);
      peak_scalers.resize(num_bands);
      for(int b = old_num_bands; b < num_bands; b++) {
         chebyshev_distortions[b].resize(chebyshev_distortions[0].size());
         peak_scalers[b].reset(new PeakScaler(spec, oversample_factor));
         peak_scalers[b]->set_gain(peak_scalers[0]->gain);
         for(int i = 0; i < chebyshev_distortions[0].size(); i++)
         {
            chebyshev_distortions[b][i].reset(new ChebyshevTable(spec, chebyshev_distortions[0][i]->shift, chebyshev_distortions[0][i]->g));
            
            chebyshev_distortions[b][i]->set_scaling(chebyshev_distortions[0][i]->get_scaling());
            
            
         }
      }
   }
   
   noise_filters.resize(num_bands);
   
   for(int b = 0; b < num_bands; b++) {
      float centre_freq = filter_bank->filters[0][b]->GetCenterFrequency();
      noise_filters[b].reset();
      noise_filters[b].prepare(spec);
      noise_filters[b].setType(dsp::StateVariableTPTFilterType::highpass);
      noise_filters[b].setCutoffFrequency(centre_freq * (2.0 / 3.0));
      noise_filters[b].setResonance(1.0 / sqrt(2));
   }
   
   
   band_data.clear();
   band_data.resize(num_bands);
   
   split_bands.resize(num_bands);
   
   for(int i = 0; i < num_bands; i++) {
      split_bands[i] = dsp::AudioBlock<float>(band_data[i], num_channels, block_size * oversample_factor);
   }
   
   harmonics_buffer.setSize(num_channels, block_size * oversample_factor);
   
}

//==============================================================================
void Distortion_ModellerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
   block_size = samplesPerBlock;
   sample_rate = sampleRate;
   int num_channels = getTotalNumInputChannels();
   
   oversampler.reset(new dsp::Oversampling<float>(num_channels, std::log2(oversample_factor), dsp::Oversampling<float>::filterHalfBandFIREquiripple));
   //FloatVectorOperations::enableFlushToZeroMode(true);
   
   float oversampled_rate = sample_rate * oversample_factor;
   
   temp_buffer = dsp::AudioBlock<float>(temp_storage, num_channels, block_size * oversample_factor);
   
   temp_buffer2 = dsp::AudioBlock<float>(temp_storage2, num_channels, block_size * oversample_factor);
   
   instant_amp = dsp::AudioBlock<float>(temp_storage3, num_channels, block_size * oversample_factor);
   
   inv_scaling = dsp::AudioBlock<float>(temp_storage4, num_channels, block_size * oversample_factor);
   
   filter_buffer = dsp::AudioBlock<float>(temp_storage5, num_channels, block_size * oversample_factor);
   
   inv_filter = dsp::AudioBlock<float>(temp_storage6, num_channels, block_size * oversample_factor);
   
   filter_bank.reset(new GammatoneFilterBank(oversampled_rate, samplesPerBlock * oversample_factor, num_channels));
   

   oversampler->initProcessing(samplesPerBlock);
   
   update_bands(-0.8);
   
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

   int num_samples = buffer.getNumSamples() * oversample_factor;
   int num_channels = getTotalNumInputChannels();
   dsp::AudioBlock<float> oversampled = oversampler->processSamplesUp(in_block);
   
   for(int ch = 0; ch < oversampled.getNumChannels(); ch++) {
      auto* channel_ptr = oversampled.getChannelPointer(ch);
      
      for(int n = 0; n < oversampled.getNumSamples(); n++) {
         
         channel_ptr[n] = dsp::FastMathApproximations::tanh(saturation * channel_ptr[n]) / saturation;
      }
   }
   
   filter_bank->process(oversampled, split_bands);
   oversampled.clear();
   
   for(int i = 0; i < num_bands; i++) {
      
      
      float centre_freq = filter_bank->filters[0][i]->GetCenterFrequency();
      
     
      if(centre_freq > tone_cutoff * 0.25 * sample_rate || centre_freq < 60) {
         oversampled += split_bands[i];
         continue;
      }
      else if(centre_freq > tone_cutoff * 0.125 * sample_rate) {
         split_bands[i] *= ((tone_cutoff * 0.25 * sample_rate) / centre_freq) - 1.0;
      }
      
      
      peak_scalers[i]->get_scaling(split_bands[i], instant_amp);
      
      
      for(int h = 0; h < chebyshev_distortions[i].size(); h++) {
            
         temp_buffer2.copyFrom(instant_amp);
         temp_buffer2 *= chebyshev_distortions[i][h]->get_scaling();
         temp_buffer2 *= split_bands[i];
         
         chebyshev_distortions[i][h]->process(temp_buffer2, temp_buffer, num_samples);
         
         peak_scalers[i]->get_inverse(inv_scaling);
         temp_buffer *= inv_scaling;
         
         for(int ch = 0; ch < getTotalNumInputChannels(); ch++) {
            harmonics_buffer.copyFrom(ch, 0, temp_buffer.getChannelPointer(ch), num_samples);
         }

         
         filter_buffer += harmonics_buffer;
      }
      
      noise_filters[i].process(dsp::ProcessContextReplacing<float>(filter_buffer));
      
      oversampled += filter_buffer;
      filter_buffer.clear();
      
   }
   oversampled += inv_filter;
   
   oversampler->processSamplesDown(in_block);

   in_block *= master_volume;
   
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
         HarmonicResponse harmonic = {x_value * 9, (1.0 - y_value) * 1.5, kind};
         
         queue.enqueue([this, harmonic, slider_idx]() mutable {
            set_harmonic(slider_idx, harmonic);
         });
      }
      else if(property == Identifier("ModDepth")) {
         queue.enqueue([this, value, slider_idx]() mutable {
            for(int b = 0; b < num_bands; b++) {
               //chebyshev_distortions[b][slider_idx]->set_filter_resonance(value);
               chebyshev_distortions[b][slider_idx]->mod_depth = value;
            }
         });
      }
      else if(property == Identifier("FilterType")) {
         queue.enqueue([this, value, slider_idx]() mutable {
            for(int b = 0; b < num_bands; b++) {
               //chebyshev_distortions[b][slider_idx]->set_filter_type((int)value);
            }
         });
      }
      else if(property == Identifier("ModRate")) {
         queue.enqueue([this, value, slider_idx]() mutable {
            for(int b = 0; b < num_bands; b++) {
               chebyshev_distortions[b][slider_idx]->mod_freq = value;
            }
         });
      }
      else if(property == Identifier("Enabled")) {
         queue.enqueue([this, value, slider_idx]() mutable {
            for(int b = 0; b < num_bands; b++) {
               chebyshev_distortions[b][slider_idx]->enabled = value;
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
         tone_cutoff = value;
      });
   }
   else if(property == Identifier("Gain")) {
      queue.enqueue([this, value]() mutable {
         for(int b = 0; b < num_bands; b++) {
            peak_scalers[b]->set_gain(value);
         }
         
      });
   }
   else if(property == Identifier("Saturation")) {
      queue.enqueue([this, value]() mutable {
            saturation = value * 4.0;
      });
   }
   else if(property == Identifier("Volume")) {
      queue.enqueue([this, value]() mutable {
         master_volume = value;
         
      });
   }
   else if(property == Identifier("Smooth")) {
      queue.enqueue([this, value]() mutable {
         smooth_mode = value;
         
         for(int b = 0; b < num_bands; b++) {
            float centre_freq = filter_bank->filters[0][b]->GetCenterFrequency();
            noise_filters[b].setType(value ? dsp::StateVariableTPTFilterType::bandpass : dsp::StateVariableTPTFilterType::highpass);
            noise_filters[b].setCutoffFrequency(value ? centre_freq : (centre_freq * (2.0 / 3.0)));
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
         chebyshev_distortions[b].push_back(std::unique_ptr<ChebyshevTable>(new ChebyshevTable({sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, (juce::uint32)getTotalNumInputChannels()}, shift, gain, kind)));
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
   

   MemoryOutputStream ostream;
   main_tree.writeToStream(ostream);
   destData = ostream.getMemoryBlock();
   
   
}

void Distortion_ModellerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
   // todo: check if correct data
   
   main_tree.copyPropertiesAndChildrenFrom(ValueTree::readFromData(data, sizeInBytes), nullptr);

   // You should use this method to restore your parameters from this memory block,
   // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
   return new Distortion_ModellerAudioProcessor();
}
