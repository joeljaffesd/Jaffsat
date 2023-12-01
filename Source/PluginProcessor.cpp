/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JaffsatAudioProcessor::JaffsatAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),

overSamp (getTotalNumInputChannels(), 4, juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, true)

#endif
{
}

JaffsatAudioProcessor::~JaffsatAudioProcessor()
{
}

//==============================================================================
const juce::String JaffsatAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JaffsatAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JaffsatAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JaffsatAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JaffsatAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JaffsatAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JaffsatAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JaffsatAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JaffsatAudioProcessor::getProgramName (int index)
{
    return {};
}

void JaffsatAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void JaffsatAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();
    
    sinOsc.prepare (spec);
    sinOsc.setFrequency(4.0f);
    sinOscGain.setGainLinear ( 1.0f );
    
    convolver.reset();
    convolver.prepare (spec);
    
    auto dir = juce::File::getCurrentWorkingDirectory();
    
    //int numTries = 0;
    
    //while (! dir.getChildFile ("Resources").exists() && numTries++ < 15) dir = dir.getParentDirectory();
    
    /*
    if (getSampleRate() == 44100) {
        convolver.loadImpulseResponse(dir.getChildFile("Resources").getChildFile("44.1k_IR"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
    }
    else if (getSampleRate() == 48000) {
        convolver.loadImpulseResponse(dir.getChildFile("Resources").getChildFile("48k_IR"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
    }
    else {
        jassert(false);
    }
    */
    /*
    if (getSampleRate() == 44100) {
        convolver.loadImpulseResponse(dir.getChildFile("Resources").getChildFile("44.1k_IR"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
    }
    else if (getSampleRate() == 48000) {
        convolver.loadImpulseResponse(dir.getChildFile("Resources").getChildFile("48k_IR"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0);
    }
    else {
        jassert(false);
    }
    */
}

void JaffsatAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JaffsatAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

void JaffsatAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> bufferBlock (buffer);
    sinOsc.process(juce::dsp::ProcessContextReplacing<float> (bufferBlock));
    
    //Operation Mode
    float operationMode = treeState.getRawParameterValue ("Operation Mode")->load();
    
    //Gain Control
    float gainFactor = dbtoa(treeState.getRawParameterValue ("Gain")->load());
    
    //Saturation Control
    float satCoef = treeState.getRawParameterValue ("Saturation")->load();
    //Threshold Control
    float thresh = treeState.getRawParameterValue ("Threshold")->load();
    
    //Blend Control
    float blendFactor = treeState.getRawParameterValue ("Blend")->load();
    float dryGain = scale (100 - blendFactor, 0.0f, 100.0f, 0.0f, 1.0f);
    float wetGain = scale (blendFactor, 0.0f, 100.0f, 0.0f, 1.0f);
    
    //Volume Control
    float volFactor = dbtoa(treeState.getRawParameterValue ("Volume")->load());

    
    if ( operationMode == 0) {
        //Bypass
        for (int sample = 0; sample < bufferBlock.getNumSamples(); ++sample)
        {
            for (int channel = 0; channel < bufferBlock.getNumChannels(); ++channel) {
                float* data = bufferBlock.getChannelPointer(channel);
                
                data[sample] = data[sample] * volFactor;
            }
        }
    }
    else {
        //On
        overSamp.processSamplesUp(bufferBlock);
        
        for (int sample = 0; sample < bufferBlock.getNumSamples(); ++sample)
        {
            float* dataLeft = bufferBlock.getChannelPointer(0);
            float* dataRight = bufferBlock.getChannelPointer(1);
                
            float inputSample = dataLeft[sample] * gainFactor;
            float distSample = distData(inputSample, satCoef, thresh);
            float outputSample = (distSample * wetGain) + (inputSample * dryGain);
            dataLeft[sample] = outputSample * volFactor;
            dataRight[sample] = outputSample * volFactor;
            
        }
        
        overSamp.processSamplesDown(bufferBlock);
        /*
        if (convolver.getCurrentIRSize() > 0)
        {
            convolver.process(juce::dsp::ProcessContextReplacing<float>(bufferBlock));
        }
        
      */
    }
   
}

float JaffsatAudioProcessor::distData(float sample, float satCoef, float thresh)
{
    if (sample <= 0 ) {
        return piDivisor * (atanf(sample * satCoef) / atanf(satCoef));
    }
    else if (sample > thresh) {
        return thresh;
        }
    else {
        return sample;
    }
}

//==============================================================================
bool JaffsatAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JaffsatAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void JaffsatAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void JaffsatAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout
    JaffsatAudioProcessor::createParameterLayout()
{
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        
        //adds parameter for controlling input gain
        layout.add(std::make_unique<juce::AudioParameterFloat>("Gain",
        "Gain",
        juce::NormalisableRange<float>(-60.f, 60.f, 1.f, 1.f), 0.f));
                                       // (low, hi, step, skew), default value)
        
        //adds parameter for adjusting saturation level
        layout.add(std::make_unique<juce::AudioParameterFloat>("Saturation",
        "Saturation",
        juce::NormalisableRange<float>(1.f, 5.f, 0.01f, 1.f), 1.0f));
        
        //adds parameter for adjusting threshold
        layout.add(std::make_unique<juce::AudioParameterFloat>("Threshold",
        "Threshold",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f, 1.f), 1.0f));
        
        //adds parameter for blending clipped signal with input signal
        layout.add(std::make_unique<juce::AudioParameterFloat>("Blend",
        "Blend",
        juce::NormalisableRange<float>(0.f, 100.f, 1.f, 1.f), 0.0f));
        
        //adds parameter for output volume
        layout.add(std::make_unique<juce::AudioParameterFloat>("Volume",
        "Volume",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 1.f, 1.f), -60.0f));
        
        /*
        //adds irLoader
        layout.add(std::make_unique<juce::FileChooser>("Choose File", JaffsatAudioProcessor.root, "*"));
        
        const auto fileChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories;
        
        fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser)
                                 {
            juce::File result
        });
        */
        
        //adds binary option for Mono->Stereo or Bypass
        juce::StringArray stringArray;
        for( int i = 0; i < 2; ++i )
        {
            juce::String str;
            if (i == 0) {
                str << "Mono-to-Stereo Bypass";
            }
            else {
                str << "Mono-to-Stereo Operation";
            }
            stringArray.add(str);
        }
        
        layout.add(std::make_unique<juce::AudioParameterChoice>("Operation Mode", "Operation Mode", stringArray, 0));
        
        return layout;
    }

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JaffsatAudioProcessor();
}
