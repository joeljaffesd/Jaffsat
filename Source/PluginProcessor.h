/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class JaffsatAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    JaffsatAudioProcessor();
    ~JaffsatAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
        juce::AudioProcessorValueTreeState treeState {*this, nullptr, "Parameters", createParameterLayout()};

private:
    
    float dbtoa(float valueIndB) {
            return std::pow(10.0, valueIndB / 20.0);
        }
    
    float scale(float x, float inMin, float inMax, float outMin, float outMax) {
            // Perform linear mapping based on specified input and output ranges
            float scaledValue = ((x - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
            
            return scaledValue;
        }
    
    juce::dsp::Oversampling<float> overSamp;
    juce::dsp::Gain<float> sinOscGain;
    juce::dsp::Oscillator<float> sinOsc { [](float x) {return std::sin (x); }};
    
    float distData(float samples, float sastCoef);
    static constexpr float piDivisor = 2.0f / juce::MathConstants<float>::pi;
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JaffsatAudioProcessor)
};
