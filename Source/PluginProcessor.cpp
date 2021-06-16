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

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

#include "HilbertEnvelope.hpp"
#include "RMSEnvelope.hpp"

#include "Filterbanks/GammatoneFilterBank.hpp"
#include "Filterbanks/ResonBands.hpp"
//==============================================================================
ZirconAudioProcessor::ZirconAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                  ), proc_valuetree(*this, nullptr, Identifier("Main"), createParameterLayout())
#endif
{
    
    // Add listeners
    for(auto child : proc_valuetree.state) {
        proc_valuetree.addParameterListener(child.getProperty("id").toString(), this);
    }
    main_tree.addListener(this);

    // Reserve space to prevent memory allocations later on
    chebyshev_distortions.ensureStorageAllocated(max_voices);
    read_bands.reserve(max_bands);
    
}

ZirconAudioProcessor::~ZirconAudioProcessor()
{
    //ChebyshevFactory::deallocate();
}


AudioProcessorValueTreeState::ParameterLayout ZirconAudioProcessor::createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    
    // First initialise our own value tree
    main_tree.setProperty("Intermodulation", 0, nullptr);
    main_tree.setProperty("Tone", 1.0f, nullptr);
    main_tree.setProperty("Gain", 1.0f, nullptr);
    main_tree.setProperty("Volume", 1.0f, nullptr);
    main_tree.setProperty("Wet", 0.5f, nullptr);
    main_tree.setProperty("High", false, nullptr);
    main_tree.setProperty("Smooth", false, nullptr);
    main_tree.setProperty("Quality", 1, nullptr);
    
    // Then initialise audio processor value tree
    layout.add (std::make_unique<AudioParameterFloat> ("Tone", "Tone", 0.0f, 1.0f, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> ("Gain", "Gain", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> ("Volume", "Volume", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> ("Wet", "Wet", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterBool> ("High", "High", false));
    layout.add (std::make_unique<AudioParameterBool> ("Smooth", "Smooth", false));
    
    // Don't add Intermodulation and Quality as automatable parameters: these are clicky parameters that shouldn't be changed during playback
    
    int max_polynomials = 5;
    
    for(int p = 0; p < max_polynomials; p++) {
        String ID = "XYSlider" + String(p + 1) + ": ";
        
        layout.add (std::make_unique<AudioParameterFloat> (ID + "X", ID + "X", 0, 1, 0.5f));
        layout.add (std::make_unique<AudioParameterFloat> (ID + "Y", ID + "Y", 0, 1, 0.5f));
        
        layout.add (std::make_unique<AudioParameterFloat> (ID + "Volume", ID + "Volume", 0, 1, 0.5f));
        layout.add (std::make_unique<AudioParameterBool> (ID + "Phase", ID + "Phase", 0));
        layout.add (std::make_unique<AudioParameterBool> (ID + "Kind", ID + "Kind", 0));
        layout.add (std::make_unique<AudioParameterInt> (ID + "ModShape", ID + "Modulation Shape", 0, 15, 0));
        layout.add (std::make_unique<AudioParameterFloat> (ID + "ModDepth", ID + "Modulation Depth", 0, 1, 1.0f));
        layout.add (std::make_unique<AudioParameterFloat> (ID + "ModRate", ID + "Modulation Rate", 1, 8, 1.0f));
        layout.add (std::make_unique<AudioParameterInt> (ID + "ModSettings", ID + "Stereo/Sync", 0, 3, 0));
    }
    
    return layout;
}

//==============================================================================
const juce::String ZirconAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ZirconAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ZirconAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ZirconAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ZirconAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ZirconAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int ZirconAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ZirconAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ZirconAudioProcessor::getProgramName (int index)
{
    return {};
}

void ZirconAudioProcessor::changeProgramName (int index, const juce::String& new_name)
{
}

void ZirconAudioProcessor::set_num_bands(int freq_range_overlap, bool reset)
{
    freq_range_overlap += 1;
    
    if(freq_range_overlap == 1) {
        // Use reson bands
        filter_bank.reset(new ResonBands(oversampled_spec));
        
        num_bands = 12;
        static_cast<ResonBands*>(filter_bank.get())->create_bands(num_bands, {60.0f, 10000.0f});
    }
    else {
        // use gammatone bands
        filter_bank.reset(new GammatoneFilterBank(oversampled_spec));
        num_bands = static_cast<GammatoneFilterBank*>(filter_bank.get())->init_with_overlap(60.0f, 10000.0f, -0.9);
    }
    
    juce::uint32 num_channels = getTotalNumOutputChannels();
    
    ProcessSpec spec = {sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, num_channels};

    if(smooth_mode) {
        envelope_follower.reset(new RMSEnvelope(spec, num_bands, oversample_factor));
    }
    else {
        envelope_follower.reset(new HilbertEnvelope(spec, num_bands, oversample_factor));
    }
    
    auto centre_freqs = get_centre_freqs();
    for(auto& distortion : chebyshev_distortions) {
        distortion->set_centre_freqs(centre_freqs);
    }
    
    iamp_data.clear();
    iamp_data.resize(num_bands);
    
    band_data.clear();
    band_data.resize(num_bands);
    
    band_tone_data.clear();
    band_tone_data.resize(num_bands);
    
    write_data.clear();
    write_data.resize(num_bands);
    
    inv_data.clear();
    inv_data.resize(num_bands);
    
    split_bands.resize(num_bands);
    band_tone.resize(num_bands);
    instant_amp.resize(num_bands);
    write_bands.resize(num_bands);
    inv_scaling.resize(num_bands);
    
    tone_block = AudioBlock<float>(tone_data, 1, block_size * oversample_factor);
    gain_block = AudioBlock<float>(gain_data, num_channels, block_size * oversample_factor);
    
    for(int i = 0; i < num_bands; i++) {
        split_bands[i] = AudioBlock<float>(band_data[i], num_channels, block_size * oversample_factor);
        
        instant_amp[i] = AudioBlock<float>(iamp_data[i], num_channels, block_size * oversample_factor);
        
        band_tone[i] = AudioBlock<float>(band_tone_data[i], 1, block_size * oversample_factor);
        
        write_bands[i] = AudioBlock<float>(write_data[i], num_channels, block_size * oversample_factor);
        
        inv_scaling[i] = AudioBlock<float>(inv_data[i], num_channels, block_size * oversample_factor);
        
        split_bands[i].fill(0.0f);
        band_tone[i].fill(0.0f);
        write_bands[i].fill(0.0f);
        inv_scaling[i].fill(0.0f);
    }
}

void ZirconAudioProcessor::parameterChanged (const String &parameter_id, float new_value) {
    if(parameter_id.startsWith("XYSlider")) {
        String param_name = parameter_id.fromFirstOccurrenceOf("XYSlider", false, false);
        int idx = param_name.substring(0, 1).getIntValue() - 1;
        param_name = param_name.substring(3);
        
        auto pad_tree = main_tree.getChildWithName("XYPad");
        
        // Don't change parameters on non-existent sliders
        if (idx >= pad_tree.getNumChildren()) return;
        
        auto slider_tree = main_tree.getChildWithName("XYPad").getChild(idx);
        slider_tree.setProperty(param_name, new_value, nullptr);
    }
    else {
        main_tree.setProperty(parameter_id, new_value, nullptr);
    }
}


//==============================================================================
void ZirconAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    block_size = samplesPerBlock;
    sample_rate = sampleRate;
    
    last_spec = {sample_rate, (juce::uint32)block_size, (juce::uint32)getTotalNumOutputChannels()};
    
    set_oversample_rate(1 << (int)main_tree.getProperty("Quality"));
    
    mixer.prepare(last_spec);
    mixer.setMixingRule(DryWetMixingRule::balanced);
}

void ZirconAudioProcessor::set_oversample_rate(int new_oversample_factor)
{
    int num_channels = getTotalNumOutputChannels();
    oversample_factor = new_oversample_factor;
    
    oversampler.reset(new Oversampling<float>(num_channels, std::log2(oversample_factor), Oversampling<float>::filterHalfBandFIREquiripple));
    
    oversampled_spec = {sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, (juce::uint32)num_channels};
    
    oversampler->initProcessing(block_size);
    
    for(int i = 0; i < chebyshev_distortions.size(); i++) {
        chebyshev_distortions.set(i, new ChebyshevTable(oversampled_spec, *chebyshev_distortions[i]));
    }
    
    gain.reset(sample_rate * oversample_factor, 0.02f);
    master_volume.reset(sample_rate * oversample_factor, 0.02f);
    tone_cutoff.reset(sample_rate * oversample_factor, 0.02f);
    
    gain.setCurrentAndTargetValue(main_tree.getProperty("Gain"));
    master_volume.setCurrentAndTargetValue(main_tree.getProperty("Volume"));
    tone_cutoff.setCurrentAndTargetValue(main_tree.getProperty("Tone"));
    mixer.setWetMixProportion(main_tree.getProperty("Wet"));
    
    set_num_bands((int)main_tree.getProperty("Intermodulation"), true);
    
}
void ZirconAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool ZirconAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{

    if (layouts.getMainOutputChannelSet() == AudioChannelSet::mono())
    {
        // Mono-to-mono
        if (layouts.getMainInputChannelSet() == AudioChannelSet::mono())
            return true;
    }
    else if (layouts.getMainOutputChannelSet() == AudioChannelSet::stereo())
    {
        // Mono-to-stereo OR stereo-to-stereo
        if ((layouts.getMainInputChannelSet() == AudioChannelSet::mono()) ||
                (layouts.getMainInputChannelSet() == AudioChannelSet::stereo()))
            return true;
    }
    
    return false;
}

void ZirconAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Check for parameter changes
    std::function<void()> action;
    while(queue.try_dequeue(action)) {
        action();
    }
    
    auto* playhead = getPlayHead();
    if(playhead) {
        for(auto& distortion : chebyshev_distortions) {
            distortion->sync_with_playhead(playhead);
        }
    }

    AudioBlock<float> in_block(buffer);

    if(getTotalNumInputChannels() < getTotalNumOutputChannels()) {
        in_block.getSingleChannelBlock(1).copyFrom(in_block.getSingleChannelBlock(0));
    }
    
    mixer.pushDrySamples(in_block);
    
    auto oversampled = oversampler->processSamplesUp(in_block);
    
    // Get smoothed tone value multiplied by 1/5th sample rate
    tone_block.fill(0.2f * sample_rate);
    tone_block *= tone_cutoff;
    
    gain_block.fill(1.0f);
    gain_block *= gain;

    filter_bank->process(oversampled, split_bands);
    
    int reduced_block_size = (int)oversampled.getNumSamples();
    oversampled.clear();
    
    read_bands = split_bands;
    
    envelope_follower->process(read_bands, instant_amp, inv_scaling, reduced_block_size);
    
    for(int b = read_bands.size()-1; b >= 0; b--) {
        
        read_bands[b] = read_bands[b].getSubBlock(0, reduced_block_size);
        
        // Divide bands by the calculated envelope of the band
        // This will scale the signal amplitude up to 1 at all times
        // Basically it removes all the dynamic range from the signal before applying distortion
        read_bands[b] *= instant_amp[b];
        
        // Apply master gain
        read_bands[b] *= gain_block;
        write_bands[b].clear();
    }
    
    // Apply distortion!
    for(int h = 0; h < chebyshev_distortions.size(); h++) {
        chebyshev_distortions[h]->process(read_bands, write_bands);
    }
    
    for(int b = 0; b < write_bands.size(); b++) {
        auto block = write_bands[b].getSubBlock(0, reduced_block_size);
        
        // Re-apply original dynamics
        block *= inv_scaling[b];
        
        // Apply tone control
        band_tone[b].copyFrom(tone_block);
        band_tone[b] *= 1.0f / filter_bank->get_centre_freq(b);
        
        auto* tone_ptr = band_tone[b].getChannelPointer(0);
        FloatVectorOperations::clip(tone_ptr, tone_ptr, 0.0f, 1.0f, reduced_block_size);
        
        // Apply tone control
        for(int ch = 0; ch < oversampled.getNumChannels(); ch++) {
            block.getSingleChannelBlock(ch) *= band_tone[b];
        }
        
        oversampled += block;
        
        write_bands[b].clear();
    }
    
    oversampler->processSamplesDown(in_block);
    
    mixer.mixWetSamples(in_block);
    
    // Apply master volume
    in_block *= master_volume;
}

std::vector<float> ZirconAudioProcessor::get_centre_freqs() {
    std::vector<float> result(filter_bank->get_num_filters());
    for(int i = 0; i < filter_bank->get_num_filters(); i++) {
        result[i] = filter_bank->get_centre_freq(i);
    }
    return result;
}

void ZirconAudioProcessor::valueTreeChildRemoved(ValueTree &parent_tree, ValueTree &removed_child, int idx)
{
    if(removed_child.getType() == Identifier("XYSlider")) {
        queue.enqueue([this, idx]() mutable {
            chebyshev_distortions.remove(idx);
        });
    }
}

void ZirconAudioProcessor::valueTreePropertyChanged (ValueTree &changed_tree, const Identifier &property)
{
    float value = changed_tree.getProperty(property);
    
    // When a property changes on a subtree named XYSlider,
    // forward the messge to the chebychev distortion
    if(changed_tree.getType() == Identifier("XYSlider")) {
        int idx = changed_tree.getParent().indexOf(changed_tree);
        float value = changed_tree.getProperty(property);
        Identifier id = property;
        
        queue.enqueue([this, idx, id, value]() mutable {
            if(idx < chebyshev_distortions.size())
                chebyshev_distortions[idx]->receive_message(id, value);
        });
    }
    else if(property == Identifier("Intermodulation")) {
        queue.enqueue([this, value]() mutable {
            set_num_bands((int)value);
        });
    }
    else if(property == Identifier("Tone")) {
        queue.enqueue([this, value]() mutable {
            tone_cutoff.setTargetValue(value);
        });
    }
    else if(property == Identifier("Gain")) {
        queue.enqueue([this, value]() mutable {
            gain.setTargetValue(value);
        });
    }
    else if(property == Identifier("Wet")) {
        queue.enqueue([this, value]() mutable {
            mixer.setWetMixProportion(std::max(value, 1e-4f));
        });
    }
    else if(property == Identifier("Volume")) {
        queue.enqueue([this, value]() mutable {
            master_volume.setTargetValue(value);
        });
    }
    else if(property == Identifier("Quality")) {
        queue.enqueue([this, value]() mutable {
            set_oversample_rate(1<<(int)value);
        });
    }
    else if(property == Identifier("Smooth")) {
        queue.enqueue([this, value]() mutable {
            smooth_mode = value;
            if(smooth_mode) {
                envelope_follower.reset(new RMSEnvelope(oversampled_spec, num_bands, oversample_factor));
            }
            else {
                envelope_follower.reset(new HilbertEnvelope(oversampled_spec, num_bands, oversample_factor));
            }
        });
    }
    else if(property == Identifier("High")) {
        queue.enqueue([this, property, value]() mutable {
            high_mode = value;
            for(auto& distortion : chebyshev_distortions)
            {
                distortion->receive_message("High", value);
            }
        });
    }
}


//==============================================================================
bool ZirconAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ZirconAudioProcessor::createEditor()
{
    return new ZirconAudioProcessorEditor (*this);
}

//==============================================================================
void ZirconAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    
    MemoryOutputStream ostream;
    main_tree.writeToStream(ostream);
    destData = ostream.getMemoryBlock();
    
    
}

void ZirconAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    proc_valuetree.state.copyPropertiesAndChildrenFrom(ValueTree::readFromData(data, sizeInBytes), nullptr);
    
    main_tree = proc_valuetree.state;
    
    auto* editor = static_cast<ZirconAudioProcessorEditor*>(getActiveEditor());
    
    chebyshev_distortions.clear();
    
    if(editor) {
        editor->xy_pad.update_tree(main_tree.getChildWithName("XYPad"));
        editor->num_filters.referTo(main_tree.getPropertyAsValue("Intermodulation", nullptr));
        editor->high_mode.referTo(main_tree.getPropertyAsValue("High", nullptr));
        editor->smooth_mode.referTo(main_tree.getPropertyAsValue("Smooth", nullptr));
    }
    
    for(auto slider : main_tree.getChildWithName("XYPad")) {
        queue.enqueue([this]() mutable {
            chebyshev_distortions.add(new ChebyshevTable(oversampled_spec, get_centre_freqs()));
        });
    }
    
    main_tree.sendPropertyChangeMessage("High");
    main_tree.sendPropertyChangeMessage("Smooth");
    main_tree.sendPropertyChangeMessage("Intermodulation");
}

void ZirconAudioProcessor::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    if(childWhichHasBeenAdded.getType() == Identifier("XYSlider")) {
        queue.enqueue([this]() mutable {
            chebyshev_distortions.add(new ChebyshevTable(oversampled_spec, get_centre_freqs()));
        });
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZirconAudioProcessor();
}
