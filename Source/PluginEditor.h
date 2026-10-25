#pragma once

#include "PluginProcessor.h"

namespace pr
{
    const juce::Colour shell   { 0xff0d0d0f };
    const juce::Colour panelHi { 0xff1e1e20 };
    const juce::Colour panelLo { 0xff131315 };
    const juce::Colour screen  { 0xff0e0e10 };
    const juce::Colour line    { 0xff34343a };
    const juce::Colour fg      { 0xffe7e7eb };
    const juce::Colour dim     { 0xff6d6d75 };
    const juce::Colour accent  { 0xff7b5cf2 };
    const juce::Colour accentHi{ 0xffa78fff };
    const juce::Colour warm    { 0xffff9d1c };

    juce::Font makeFont (float height, bool bold, float kerning = 0.0f);
    float textWidth (const juce::Font& font, const juce::String& text);
}

//==============================================================================
class RiserLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

//==============================================================================
class FlatButton : public juce::Button
{
public:
    enum class Icon { None, Prev, Next, Save, Reset, Play, Stop, Loop, Power };
    enum class Align { Left, Centre, Right };

    FlatButton (const juce::String& name, Icon ic, const juce::String& label = {});

    void setLabel (const juce::String& t);
    void setIcon (Icon ic);

    juce::Colour onColour { pr::accentHi };
    juce::Colour offColour { pr::dim };
    float fontSize = 12.0f;
    float kerning = 0.16f;
    bool bold = true;
    Align align = Align::Centre;

    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    Icon icon;
    juce::String text;
};

//==============================================================================
class WaveView : public juce::Component
{
public:
    explicit WaveView (PoasRiserProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    PoasRiserProcessor& proc;
};

//==============================================================================
class PoasRiserEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit PoasRiserEditor (PoasRiserProcessor&);
    ~PoasRiserEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setChoice (const char* id, int index);
    void setBool (const char* id, bool value);
    int getChoice (const char* id) const;

    PoasRiserProcessor& proc;
    RiserLookAndFeel lnf;

    WaveView wave;
    juce::Slider knob, level;
    FlatButton prevBtn { "prev", FlatButton::Icon::Prev };
    FlatButton nextBtn { "next", FlatButton::Icon::Next };
    FlatButton saveBtn { "save", FlatButton::Icon::Save };
    FlatButton resetBtn { "reset", FlatButton::Icon::Reset, "RESET" };
    FlatButton listenBtn { "listen", FlatButton::Icon::Play, "LISTEN" };
    FlatButton loopBtn { "loop", FlatButton::Icon::Loop, "LOOP" };
    FlatButton rawBtn { "raw", FlatButton::Icon::None, "RAW" };
    FlatButton editBtn { "edit", FlatButton::Icon::None, "EDIT" };
    FlatButton modeBtn { "mode", FlatButton::Icon::None, "REVERSE" };
    FlatButton lenBtn { "len", FlatButton::Icon::None, "1 BAR" };
    FlatButton phoneBtn { "phone", FlatButton::Icon::Power, "PHONE" };
    FlatButton chorusBtn { "chorus", FlatButton::Icon::Power, "CHORUS" };
    FlatButton tremBtn { "tremolo", FlatButton::Icon::Power, "TREMOLO" };

    juce::String presetName { "Default" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt, levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopAtt, phoneAtt, chorusAtt, tremAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PoasRiserEditor)
};
