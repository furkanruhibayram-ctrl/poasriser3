#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

struct RiserPreset
{
    juce::String name;
    float amount;
    int bars;      // index into the bars choice list
    int mode;      // 0 = reverse, 1 = fall
    bool phone, chorus, tremolo, edit;
};

class PoasRiserProcessor : public juce::AudioProcessor
{
public:
    PoasRiserProcessor();
    ~PoasRiserProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "POASRISER"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.3; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // used by the editor
    void requestPreview() { previewRequest.store (1); }
    void requestStop() { previewRequest.store (2); }
    bool isRiserActive() const { return voiceActiveFlag.load(); }
    float getProgress() const { return progress.load(); }
    void saveUserPreset();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void applyPreset (const RiserPreset& p);
    void triggerVoice (int note);
    void releaseVoice();

    std::vector<RiserPreset> userPresets;
    int currentProgram = 0;

    // parameters
    std::atomic<float>* amountP = nullptr;
    std::atomic<float>* barsP = nullptr;
    std::atomic<float>* modeP = nullptr;
    std::atomic<float>* loopP = nullptr;
    std::atomic<float>* editP = nullptr;
    std::atomic<float>* phoneP = nullptr;
    std::atomic<float>* chorusP = nullptr;
    std::atomic<float>* tremoloP = nullptr;
    std::atomic<float>* levelP = nullptr;

    // transport / voice
    std::atomic<int> previewRequest { 0 };
    std::atomic<bool> voiceActiveFlag { false };
    std::atomic<float> progress { -1.0f };

    double sr = 44100.0;
    double currentBpm = 120.0;

    struct Voice
    {
        bool active = false;
        bool releasing = false;
        double pos = 0.0;
        double len = 1.0;
        float rel = 1.0f;
        float noteRatio = 1.0f;
        int note = -1;
    } voice;

    float phase1 = 0.0f, phase2 = 0.0f;
    float chorusPhase1 = 0.0f, chorusPhase2 = 0.0f;
    float tremPhase = 0.0f;
    float lastFp = 0.0f;

    juce::Random rng;
    juce::dsp::StateVariableTPTFilter<float> noiseBP, oscLP, phoneHP, phoneLP;
    juce::SmoothedValue<float> phoneMix, chorusMix, tremMix;

    std::vector<float> delayL, delayR;
    int delayMask = 0;
    int delayWrite = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PoasRiserProcessor)
};
