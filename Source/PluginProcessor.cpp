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
   
   main_tree.setProperty("Intermodulation", 0, nullptr);
   main_tree.setProperty("Tone", 1.0, nullptr);
   main_tree.setProperty("Gain", 1.0, nullptr);
   main_tree.setProperty("Volume", 1.0, nullptr);
   main_tree.setProperty("Saturation", 0.5, nullptr);
   main_tree.setProperty("Smooth", false, nullptr);
   main_tree.setProperty("Heavy", false, nullptr);
   main_tree.setProperty("Quality", 1, nullptr);
   // init valuetree and parameters here!!
   
   // Reserve space to prevent memory allocations later on
   peak_scalers.ensureStorageAllocated(max_bands);
   chebyshev_distortions.ensureStorageAllocated(max_voices);
   noise_filters.reserve(max_bands);
   
   skipped_bands.reserve(max_bands);
   
}

Distortion_ModellerAudioProcessor::~Distortion_ModellerAudioProcessor()
{
   //ChebyshevFactory::deallocate();
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

void Distortion_ModellerAudioProcessor::update_bands(float freq_range_overlap, bool reset)
{
   
   int old_num_bands = reset ? 0 : filter_bank->get_num_filters() - 1;
   num_bands = filter_bank->init_with_overlap(20, sample_rate / 2.0, freq_range_overlap - 0.9);
   
   int num_channels = getTotalNumInputChannels();
   
   dsp::ProcessSpec spec = {sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, (juce::uint32)getTotalNumInputChannels()};
   
   if(old_num_bands > num_bands) {
      peak_scalers.removeLast(old_num_bands - num_bands);
   }
   else if(old_num_bands < num_bands || reset) {
      for(int b = old_num_bands; b < num_bands; b++) {
         auto* scaler = peak_scalers.add(new PeakScaler(spec, oversample_factor));
         scaler->set_gain(peak_scalers[0]->gain);
         scaler->heavy = peak_scalers[0]->heavy;
      }
   }
   
   noise_filters.resize(num_bands);
   
   for(int b = 0; b < num_bands; b++) {
      float centre_freq = filter_bank->filters[0][b]->get_centre_freq();
      noise_filters[b].reset();
      noise_filters[b].prepare(spec);
      noise_filters[b].setType(smooth_mode ? dsp::StateVariableTPTFilterType::bandpass : dsp::StateVariableTPTFilterType::highpass);
      noise_filters[b].setCutoffFrequency(smooth_mode ? centre_freq : (centre_freq * (2.0 / 3.0)));
      noise_filters[b].setResonance(1.0 / sqrt(2));
   }
   
   
   band_data.clear();
   band_data.resize(num_bands);
   
   iamp_data.clear();
   iamp_data.resize(num_bands);
   
   write_data.clear();
   write_data.resize(num_bands);
   
   inv_data.clear();
   inv_data.resize(num_bands);
   
   split_bands.resize(num_bands);
   instant_amp.resize(num_bands);
   write_bands.resize(num_bands);
   inv_scaling.resize(num_bands);
   
   for(int i = 0; i < num_bands; i++) {
      split_bands[i] = dsp::AudioBlock<float>(band_data[i], num_channels, block_size * oversample_factor);
      
      instant_amp[i] = dsp::AudioBlock<float>(iamp_data[i], num_channels, block_size * oversample_factor);
      
      write_bands[i] = dsp::AudioBlock<float>(write_data[i], num_channels, block_size * oversample_factor);
      
      inv_scaling[i] = dsp::AudioBlock<float>(inv_data[i], num_channels, block_size * oversample_factor);
      
      split_bands[i].fill(0.0);
      instant_amp[i].fill(0.0);
      write_bands[i].fill(0.0);
      inv_scaling[i].fill(0.0);
      
   }
      
}

//==============================================================================
void Distortion_ModellerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
   block_size = samplesPerBlock;
   sample_rate = sampleRate;
   int num_channels = getTotalNumInputChannels();
   
   oversampler.reset(new dsp::Oversampling<float>(num_channels, std::log2(oversample_factor), dsp::Oversampling<float>::filterHalfBandFIREquiripple));
   
   float oversampled_rate = sample_rate * oversample_factor;
   
   filter_bank.reset(new GammatoneFilterBank(oversampled_rate, samplesPerBlock * oversample_factor, num_channels));
   
   oversampler->initProcessing(samplesPerBlock);
   
   update_bands(-0.8, true);
   
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
   
   
   dsp::AudioBlock<float> in_block(buffer);

   int num_samples = buffer.getNumSamples() * oversample_factor;
   dsp::AudioBlock<float> oversampled = oversampler->processSamplesUp(in_block);
   
   for(int ch = 0; ch < oversampled.getNumChannels(); ch++) {
      auto* channel_ptr = oversampled.getChannelPointer(ch);
      
      for(int n = 0; n < oversampled.getNumSamples(); n++) {
         channel_ptr[n] = dsp::FastMathApproximations::tanh(saturation * channel_ptr[n]) / saturation;
      }
   }
   
   filter_bank->process(oversampled, split_bands);
   oversampled.clear();
   
   // TODO: allocation? bad? maybe reserve beforehand!
   auto read_bands = split_bands;
   
   skipped_bands.clear();
   

   for(int b = read_bands.size()-1; b >= 0; b--) {
      float centre_freq = filter_bank->filters[0][b]->get_centre_freq();

      // Apply tone control
      float attenuation = ((tone_cutoff * 0.25 * sample_rate) / centre_freq) - 1.0;
      read_bands[b] *= std::clamp(attenuation, 0.0f, 1.0f);
      
      // Scale bands to be closer to range -1 to 1.
      // This will lead to more extreme distortion effects with clearer harmonics,
      // Since chebyshev distortions are more sensitive in that range
      peak_scalers[b]->get_scaling(read_bands[b], instant_amp[b]);
      peak_scalers[b]->get_inverse(inv_scaling[b]);
      
      read_bands[b] *= instant_amp[b];

   }
   
   for(int h = 0; h < chebyshev_distortions.size(); h++) {
      
      chebyshev_distortions[h]->process(read_bands, write_bands);
   }
   
   for(int b = 0; b < write_bands.size(); b++) {
      write_bands[b] *= inv_scaling[b];
      noise_filters[b].process(dsp::ProcessContextReplacing<float>(write_bands[b]));
      oversampled += write_bands[b];
      
      write_bands[b].clear();
   }

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
      
      //int size = chebyshev_distortions.size();
      //if(slider_idx >= size) return;
      
      if(property == Identifier("X") || property == Identifier("Y") || property == Identifier("Kind")) {
         float x_value = treeWhosePropertyHasChanged.getProperty("X");
         float y_value = treeWhosePropertyHasChanged.getProperty("Y");
         bool kind = treeWhosePropertyHasChanged.getProperty("Kind");
         HarmonicResponse harmonic = {x_value * 9, (1.0 - y_value) * 1.5, kind};
         
         queue.enqueue([this, harmonic, slider_idx]() mutable {
            set_harmonic(slider_idx, harmonic);
         });
      }
      else if(property == Identifier("Even")) {
        
         queue.enqueue([this, value, slider_idx]() mutable {
         chebyshev_distortions[slider_idx]->set_even((int)value & 1);
         chebyshev_distortions[slider_idx]->set_odd((int)value & 2);
         });
      }
      else if(property == Identifier("ModDepth")) {
         queue.enqueue([this, value, slider_idx]() mutable {
            for(int b = 0; b < num_bands; b++) {
               //chebyshev_distortions[b][slider_idx]->set_filter_resonance(value);
               chebyshev_distortions[slider_idx]->set_mod_depth(value);
            }
         });
      }
      else if(property == Identifier("ModShape")) {
         queue.enqueue([this, value, slider_idx]() mutable {
            int shape_flag = value;
               chebyshev_distortions[slider_idx]->set_mod_shape(shape_flag);
         });
      }
      else if(property == Identifier("ModRate")) {
         queue.enqueue([this, value, slider_idx]() mutable {
               chebyshev_distortions[slider_idx]->set_mod_rate(value);
         });
      }
      else if(property == Identifier("Enabled")) {
         queue.enqueue([this, value, slider_idx]() mutable {
               chebyshev_distortions[slider_idx]->enabled = value;
         });
      }
      else if(property == Identifier("Clarity")) {
         queue.enqueue([this, value, slider_idx]() mutable {
               chebyshev_distortions[slider_idx]->set_scaling(value);
         });
      }
   }
   else if(property == Identifier("Intermodulation")) {
      queue.enqueue([this, value]() mutable {
         float options[3] = {-0.8, 0.0, 0.3};
         update_bands(options[(int)value]);
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
         saturation = std::max(value * 17.0, 1e-4);
      });
   }
   else if(property == Identifier("Volume")) {
      queue.enqueue([this, value]() mutable {
         master_volume = value;
         
      });
   }
   else if(property == Identifier("Quality")) {
      queue.enqueue([this, value]() mutable {
         
         
      });
   }
   else if(property == Identifier("Heavy")) {
      queue.enqueue([this, value]() mutable {
         for(int b = 0; b < num_bands; b++) {
            peak_scalers[b]->set_heavy(value);
         }
      });
      
      
   }
   else if(property == Identifier("Smooth")) {
      queue.enqueue([this, value]() mutable {
         smooth_mode = value;
         
         for(int b = 0; b < num_bands; b++) {
            float centre_freq = filter_bank->filters[0][b]->get_centre_freq();
            noise_filters[b].setType(smooth_mode ? dsp::StateVariableTPTFilterType::bandpass : dsp::StateVariableTPTFilterType::highpass);
            noise_filters[b].setCutoffFrequency(smooth_mode ? centre_freq : (centre_freq * (2.0 / 3.0)));
         }
      });
   }
}

void Distortion_ModellerAudioProcessor::delete_harmonic(int idx) {
      chebyshev_distortions.remove(idx);
}

void Distortion_ModellerAudioProcessor::set_harmonic(int idx, HarmonicResponse response) {
   
   if(idx >= chebyshev_distortions.size() || idx < 0) {
      auto& [shift, gain, kind] = response;
      
         auto* distortion = chebyshev_distortions.add(new ChebyshevTable({sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, (juce::uint32)getTotalNumInputChannels()}, shift, gain, kind));
         
      distortion->set_scaling(1.0);
   }
   else
   {
      auto& [shift, gain, kind] = response;
      chebyshev_distortions[idx]->set_table(shift, gain, kind);
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
   
   auto* editor = static_cast<Distortion_ModellerAudioProcessorEditor*>(getActiveEditor());

   if(editor) {
      
      editor->xy_pad.update_tree(main_tree.getChildWithName("XYPad"));
   
      editor->num_filters.referTo(main_tree.getPropertyAsValue("Intermodulation", nullptr));
      editor->smooth_mode.referTo(main_tree.getPropertyAsValue("Smooth", nullptr));
      editor->heavy_mode.referTo(main_tree.getPropertyAsValue("Heavy", nullptr));
   }
   
   main_tree.sendPropertyChangeMessage("Smooth");
   main_tree.sendPropertyChangeMessage("Heavy");
   main_tree.sendPropertyChangeMessage("Intermodulation");
   
   // You should use this method to restore your parameters from this memory block,
   // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
   return new Distortion_ModellerAudioProcessor();
}
