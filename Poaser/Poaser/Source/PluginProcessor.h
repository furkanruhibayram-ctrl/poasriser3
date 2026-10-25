#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>

#include "DSP.h"
#include "Presets.h"

class PoaserProcessor : public juce::AudioProcessor
{
public:
    PoaserProcessor();
    ~PoaserProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Poaser"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    //==========================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Riser builder (capture -> offline reverse reverb -> wav file)
    void startCapture();
    void stopCapture();
    void clearCapture();
    bool isRecording() const { return recording.load(); }
    double getCapturedSeconds() const;
    juce::File renderRiser();

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr double kMaxCaptureSeconds = 30.0;

private:
    poaser::Params getParams() const;

    std::atomic<float>* raw[poaser::kNumParams] {};
    poaser::Engine engine;
    int currentProgram = 0;

    juce::AudioBuffer<float> capture;
    std::atomic<int> captureLen { 0 };
    std::atomic<bool> recording { false };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PoaserProcessor)
};
