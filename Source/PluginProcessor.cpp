/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"


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
    noise_filters.reserve(max_bands);
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
    main_tree.setProperty("Smooth", false, nullptr);
    main_tree.setProperty("Heavy", false, nullptr);
    main_tree.setProperty("Quality", 1, nullptr);
    
    // Then initialise audio processor value tree
    layout.add (std::make_unique<AudioParameterFloat> ("Tone", "Tone", 0.0f, 1.0f, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> ("Gain", "Gain", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> ("Volume", "Volume", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> ("Wet", "Wet", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<AudioParameterBool> ("Smooth", "Smooth", false));
    layout.add (std::make_unique<AudioParameterBool> ("Heavy", "Heavy", false));
    
    // Don't add Intermodulation and Quality as automatable parameters: these are clicky parameters that shouldn't be changed during playback
    
    int max_polynomials = 5;
    
    for(int p = 0; p < max_polynomials; p++) {
        String ID = "XYSlider" + String(p + 1) + ": ";
        
        layout.add (std::make_unique<AudioParameterFloat> (ID + "X", ID + "X", 0, 1, 0.5f));
        layout.add (std::make_unique<AudioParameterFloat> (ID + "Y", ID + "Y", 0, 1, 0.5f));
        
        layout.add (std::make_unique<AudioParameterFloat> (ID + "Drive", ID + "Distortion Shape", 0, 1, 0.5f));
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

void ZirconAudioProcessor::update_bands(int freq_range_overlap, bool reset)
{
    if(reset)  {
        noise_filters.clear();
    }
    

    freq_range_overlap += 1;
    
    if(freq_range_overlap == 1) {
        // Use reson bands
        
        filter_bank.reset(new ResonBands(oversampled_spec));
        
        num_bands = 12;
        static_cast<ResonBands*>(filter_bank.get())->create_bands(num_bands, {50.0f, 10000.0f});
        
    }
    else {
        // use gammatone bands
        filter_bank.reset(new GammatoneFilterBank(oversampled_spec));
        const float num_options[2] = {-1.1, -0.4};
        num_bands = static_cast<GammatoneFilterBank*>(filter_bank.get())->init_with_overlap(20, sample_rate / 2.0f, num_options[freq_range_overlap - 2]);
    }
    
    
    juce::uint32 num_channels = getTotalNumOutputChannels();
    
    dsp::ProcessSpec spec = {sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, num_channels};
    
    hilbert.reset(new HilbertAmplitude(spec, num_bands, oversample_factor));
    
    noise_filters.resize(num_bands);
    
    for(int b = 0; b < num_bands; b++) {
        float centre_freq = filter_bank->get_centre_freq(b);
        centre_freq = std::min<float>(centre_freq, sample_rate / 2 - 1);
        noise_filters[b].reset();
        noise_filters[b].prepare(spec);
        noise_filters[b].setType(smooth_mode ? dsp::StateVariableTPTFilterType::bandpass : dsp::StateVariableTPTFilterType::highpass);
        noise_filters[b].setCutoffFrequency(smooth_mode ? centre_freq : (centre_freq * (2.0f / 3.0f)));
        noise_filters[b].setResonance(1.0f / sqrt(2));
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
    
    tone_block = dsp::AudioBlock<float>(tone_data, 1, block_size * oversample_factor);
    gain_block = dsp::AudioBlock<float>(gain_data, num_channels, block_size * oversample_factor);
    
    wet_block = dsp::AudioBlock<float>(wet_data, num_channels, block_size);
    dry_block = dsp::AudioBlock<float>(dry_data, num_channels, block_size);
    
    for(int i = 0; i < num_bands; i++) {
        split_bands[i] = dsp::AudioBlock<float>(band_data[i], num_channels, block_size * oversample_factor);
        
        instant_amp[i] = dsp::AudioBlock<float>(iamp_data[i], num_channels, block_size * oversample_factor);
        
        band_tone[i] = dsp::AudioBlock<float>(band_tone_data[i], 1, block_size * oversample_factor);
        
        write_bands[i] = dsp::AudioBlock<float>(write_data[i], num_channels, block_size * oversample_factor);
        
        inv_scaling[i] = dsp::AudioBlock<float>(inv_data[i], num_channels, block_size * oversample_factor);
        
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
}

void ZirconAudioProcessor::set_oversample_rate(int new_oversample_factor)
{
    int num_channels = getTotalNumOutputChannels();
    oversample_factor = new_oversample_factor;
    
    oversampler.reset(new dsp::Oversampling<float>(num_channels, std::log2(oversample_factor), dsp::Oversampling<float>::filterHalfBandFIREquiripple));
    
    oversampled_spec = {sample_rate * oversample_factor, (juce::uint32)block_size * oversample_factor, (juce::uint32)num_channels};
    
    oversampler->initProcessing(block_size);
    
    for(int i = 0; i < chebyshev_distortions.size(); i++) {
        auto state = chebyshev_distortions[i]->get_state();
        
        chebyshev_distortions.set(i, new ChebyshevTable(oversampled_spec, 1, 1, false));
        chebyshev_distortions[i]->set_state(state);
    }
    
    gain.reset(sample_rate * oversample_factor, 0.02f);
    master_volume.reset(sample_rate * oversample_factor, 0.02f);
    tone_cutoff.reset(sample_rate * oversample_factor, 0.02f);
    wet.reset(sample_rate * oversample_factor, 0.02f);
    
    gain.setCurrentAndTargetValue(main_tree.getProperty("Gain"));
    master_volume.setCurrentAndTargetValue(main_tree.getProperty("Volume"));
    tone_cutoff.setCurrentAndTargetValue(main_tree.getProperty("Tone"));
    wet.setCurrentAndTargetValue(main_tree.getProperty("Wet"));
    
    update_bands((int)main_tree.getProperty("Intermodulation"), true);
    
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
    
    wet_block.fill(1.0f);
    wet_block *= wet;
    
    dsp::AudioBlock<float> in_block(buffer);

    if(getTotalNumInputChannels() < getTotalNumOutputChannels()) {
        in_block.getSingleChannelBlock(1).copyFrom(in_block.getSingleChannelBlock(0));
    }
    
    dry_block.copyFrom(in_block);
    
    for(int ch = 0; ch < dry_block.getNumChannels(); ch++) {
        auto* wet_ptr = wet_block.getChannelPointer(ch);
        auto* dry_ptr = dry_block.getChannelPointer(ch);
        for(int n = 0; n < dry_block.getNumSamples(); n++) {
            dry_ptr[n] *= 1.0f - wet_ptr[n];
        }
    }
    
    
    dsp::AudioBlock<float> oversampled = oversampler->processSamplesUp(in_block);
    
    // Get smoothed tone value multiplied by 1/5th sample rate
    tone_block.fill(0.2f * sample_rate);
    tone_block *= tone_cutoff;
    
    gain_block.fill(heavy_mode ? 3.0f : 1.0f);
    gain_block *= gain;


    for(int ch = 0; ch < oversampled.getNumChannels(); ch++) {
        auto* channel_ptr = oversampled.getChannelPointer(ch);
        for(int n = 0; n < oversampled.getNumSamples(); n++) {
            channel_ptr[n] = dsp::FastMathApproximations::tanh(channel_ptr[n]);
        }
    }
    
    filter_bank->process(oversampled, split_bands);
    
    int reduced_block_size = (int)oversampled.getNumSamples();
    oversampled.clear();
    
    read_bands = split_bands;
    
    hilbert->process(read_bands, instant_amp, inv_scaling, reduced_block_size);
    
    for(int b = read_bands.size()-1; b >= 0; b--) {
           
        read_bands[b] = read_bands[b].getSubBlock(0, reduced_block_size);
        
        // Scale bands to be closer to range -1 to 1.
        // This will lead to more extreme distortion effects with clearer harmonics,
        // since Chebyshev polynomials are more sensitive in that range
        
        // Apply gain after measuring volume
        read_bands[b] *= gain_block;
    }
    
    for(int h = 0; h < chebyshev_distortions.size(); h++) {
        chebyshev_distortions[h]->process(read_bands, write_bands, instant_amp);
    }
    
    for(int b = 0; b < write_bands.size(); b++) {
        
        auto block = write_bands[b].getSubBlock(0, reduced_block_size);
        block *= inv_scaling[b];
        
        band_tone[b].copyFrom(tone_block);
        
        band_tone[b] *= 1.0f / filter_bank->get_centre_freq(b);
        
        auto* tone_ptr = band_tone[b].getChannelPointer(0);
        FloatVectorOperations::clip(tone_ptr, tone_ptr, 0.0f, 1.0f, reduced_block_size);
        
        // Apply tone control
        for(int ch = 0; ch < oversampled.getNumChannels(); ch++) {
            block.getSingleChannelBlock(ch) *= band_tone[b];
        }
        
        noise_filters[b].process(dsp::ProcessContextReplacing<float>(block));
        oversampled += block;
        
        write_bands[b].clear();
    }
    
    oversampler->processSamplesDown(in_block);
    
    in_block *= wet_block;
    in_block += dry_block;
    
    in_block *= master_volume;
    
}

void ZirconAudioProcessor::valueTreeChildRemoved(ValueTree &parent_tree, ValueTree &removed_child, int idx)
{
    if(removed_child.getType() == Identifier("XYSlider")) {
        queue.enqueue([this, idx]() mutable {
            delete_harmonic(idx);
        });
    }
}

void ZirconAudioProcessor::valueTreePropertyChanged (ValueTree &changed_tree, const Identifier &property)
{
    float value = changed_tree.getProperty(property);
    if(changed_tree.getType() == Identifier("XYSlider")) {
        int slider_idx = changed_tree.getParent().indexOf(changed_tree);
        
        if(property == Identifier("X") || property == Identifier("Y") || property == Identifier("Kind")) {
            float x_value = changed_tree.getProperty("X");
            float y_value = changed_tree.getProperty("Y");
            bool kind = changed_tree.getProperty("Kind");
            float shift = (x_value + 0.0085f) * 8.2f;
            float gain = (1.0f - y_value) * 1.5;
            
            queue.enqueue([this, kind, shift, gain, slider_idx]() mutable {
                set_harmonic(slider_idx, shift, gain, kind);
            });
        }
        else if(property == Identifier("ModDepth")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                chebyshev_distortions[slider_idx]->set_mod_depth(value * 4.0f);
            });
        }
        else if(property == Identifier("ModSettings")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                chebyshev_distortions[slider_idx]->set_sync((int)value & 1);
                chebyshev_distortions[slider_idx]->set_stereo((int)value & 2);
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
                chebyshev_distortions[slider_idx]->set_enabled((bool)value);
            });
        }
        else if(property == Identifier("Drive")) {
            queue.enqueue([this, value, slider_idx]() mutable {
                chebyshev_distortions[slider_idx]->set_scaling(value);
            });
        }
    }
    else if(property == Identifier("Intermodulation")) {
        queue.enqueue([this, value]() mutable {
            update_bands((int)value);
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
            wet.setTargetValue(std::max(value, 1e-4f));
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
    else if(property == Identifier("Heavy")) {
        queue.enqueue([this, value]() mutable {
            heavy_mode = value;
        });
        
        
    }
    else if(property == Identifier("Smooth")) {
        queue.enqueue([this, value]() mutable {
            smooth_mode = value;
            
            for(int b = 0; b < num_bands; b++) {
                float centre_freq = filter_bank->get_centre_freq(b);
                centre_freq = std::min<float>(centre_freq, sample_rate / 2 - 1);
                noise_filters[b].setType(smooth_mode ? dsp::StateVariableTPTFilterType::bandpass : dsp::StateVariableTPTFilterType::highpass);
                noise_filters[b].setCutoffFrequency(smooth_mode ? centre_freq : (centre_freq * (2.0f / 3.0f)));
            }
        });
    }
}

void ZirconAudioProcessor::delete_harmonic(int idx) {
    chebyshev_distortions.remove(idx);
}

void ZirconAudioProcessor::set_harmonic(int idx, float shift, float gain, bool kind) {
    
    if(idx >= chebyshev_distortions.size() || idx < 0) {
        auto* distortion = chebyshev_distortions.add(new ChebyshevTable(oversampled_spec, shift, gain, kind));
        
        distortion->set_scaling(1.0f);
    }
    else
    {
        chebyshev_distortions[idx]->set_table(shift, gain, kind);
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
    // todo: check if correct data
    
    proc_valuetree.state.copyPropertiesAndChildrenFrom(ValueTree::readFromData(data, sizeInBytes), nullptr);
    
    main_tree = proc_valuetree.state;
    
    auto* editor = static_cast<ZirconAudioProcessorEditor*>(getActiveEditor());
    
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

void ZirconAudioProcessor::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZirconAudioProcessor();
}
