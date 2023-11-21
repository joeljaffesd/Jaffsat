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
    
    //Operation Mode
    float operationMode = treeState.getRawParameterValue ("Operation Mode")->load();
    
    //Volume Control
    float volFactor = dbtoa(treeState.getRawParameterValue ("Volume")->load());
    
    //Gain Control
    float gainFactor = dbtoa(treeState.getRawParameterValue ("Gain")->load());
    
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
                
            dataLeft[sample] = distData((dataLeft[sample] * gainFactor)) * volFactor;
            dataRight[sample] = dataLeft[sample];
            
        }
        
        overSamp.processSamplesDown(bufferBlock);
    }
   
}

/*
float JaffsatAudioProcessor::distData(float samples)
{
    if (samples > 0) {
        samples = piDivisor * atanf(samples);
    }
    else {
        samples = samples;
    }
    
    return samples;
}
*/
float JaffsatAudioProcessor::distData(float samples)
{
    return piDivisor * atanf(samples);
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
        
        //adds parameter for controlling ratio of outpitch to inpitch
        layout.add(std::make_unique<juce::AudioParameterFloat>("Gain",
        "Gain",
        juce::NormalisableRange<float>(-60.f, 60.f, 1.f, 1.f), -60.f));
                                       // (low, hi, step, skew), default value)
        
        //adds parameter for blending pitshifted signal with input signal
        layout.add(std::make_unique<juce::AudioParameterFloat>("Blend",
        "Blend",
        juce::NormalisableRange<float>(0.f, 100.f, 1.f, 1.f), 0.0f));
        
        //adds parameter for blending pitshifted signal with input signal
        layout.add(std::make_unique<juce::AudioParameterFloat>("Volume",
        "Volume",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 1.f, 1.f), -60.0f));
        
        //adds binary option for Stereo and Mono modes (not implemented)
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
