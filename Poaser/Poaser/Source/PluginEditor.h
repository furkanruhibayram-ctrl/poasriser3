#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include "PluginProcessor.h"

//==============================================================================
class PoaserLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PoaserLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    static juce::Colour accent() { return juce::Colour (0xff8b6cff); }
    static juce::Colour accent2() { return juce::Colour (0xff3fe0c5); }
    static juce::Colour background() { return juce::Colour (0xff0f1118); }
    static juce::Colour panel() { return juce::Colour (0xff181b25); }
};

//==============================================================================
struct Knob
{
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class DragHandle : public juce::Component
{
public:
    void setFile (const juce::File& f);
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    juce::File file;
};

//==============================================================================
class PoaserEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PoaserEditor (PoaserProcessor&);
    ~PoaserEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void addKnob (const juce::String& id, const juce::String& text);
    Knob* knob (const juce::String& id);
    void layoutKnobs (std::initializer_list<const char*> ids, juce::Rectangle<int> area, int cols);
    void stepPreset (int delta);

    PoaserProcessor& processor;
    PoaserLookAndFeel laf;

    std::vector<std::pair<juce::String, std::unique_ptr<Knob>>> knobs;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };

    juce::Label modeLabel;
    juce::ComboBox modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    juce::TextButton recButton { "REC" }, clearButton { "CLEAR" },
                     renderButton { "RENDER" }, saveButton { "SAVE WAV" };
    juce::Label infoLabel;
    DragHandle dragHandle;
    juce::File lastRendered;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> reverbArea, pitchArea, reverseArea, outputArea, riserArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PoaserEditor)
};
