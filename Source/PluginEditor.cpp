#include "PluginEditor.h"

namespace pr
{
    juce::Font makeFont (float height, bool bold, float kerning)
    {
        juce::Font f (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
        return f.withExtraKerningFactor (kerning);
    }

    float textWidth (const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (font, text, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    }
}

namespace
{
    const char* const barLabels[] = { "1/2 BAR", "1 BAR", "2 BARS", "4 BARS", "8 BARS" };
    constexpr float knobStart = juce::MathConstants<float>::pi * 1.25f;
    constexpr float knobEnd = juce::MathConstants<float>::pi * 2.75f;
}

//==============================================================================
void RiserLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = size * 0.5f;

    // plate behind the dots, so the knob appears to cut into the screen
    g.setColour (juce::Colour (0xff1a1a1c));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // dots
    const int numDots = 33;
    for (int i = 0; i < numDots; ++i)
    {
        const float t = (float) i / (float) (numDots - 1);
        const float ang = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const float dx = centre.x + radius * 0.88f * std::sin (ang);
        const float dy = centre.y - radius * 0.88f * std::cos (ang);
        const bool major = (i % 8) == 0;
        const bool lit = ! major && t <= sliderPos + 0.0001f;

        float r = major ? 2.4f : 2.0f;
        if (lit)
        {
            g.setColour (pr::accentHi.withAlpha (0.25f));
            g.fillEllipse (dx - 5.0f, dy - 5.0f, 10.0f, 10.0f);
            g.setColour (pr::accentHi);
        }
        else
        {
            g.setColour (major ? juce::Colour (0xff8e8e96) : juce::Colour (0xff333338));
        }
        g.fillEllipse (dx - r, dy - r, r * 2.0f, r * 2.0f);
    }

    // knob body
    const float kr = radius * 0.68f;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (centre.x - kr - 6.0f, centre.y - kr + 4.0f, (kr + 6.0f) * 2.0f, (kr + 6.0f) * 2.0f);
    g.setColour (juce::Colour (0xff2f2f33));
    g.fillEllipse (centre.x - kr - 3.0f, centre.y - kr - 3.0f, (kr + 3.0f) * 2.0f, (kr + 3.0f) * 2.0f);
    g.setColour (juce::Colour (0xff101012));
    g.fillEllipse (centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f);

    juce::ColourGradient body (juce::Colour (0xff2c2c2f), centre.x, centre.y - kr * 0.3f,
                               juce::Colour (0xff1a1a1c), centre.x, centre.y + kr, true);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - kr + 3.0f, centre.y - kr + 3.0f, (kr - 3.0f) * 2.0f, (kr - 3.0f) * 2.0f);

    // needle
    const float ang = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float s = std::sin (ang), c = std::cos (ang);
    juce::Path needle;
    needle.startNewSubPath (centre.x + s * kr * 0.62f, centre.y - c * kr * 0.62f);
    needle.lineTo (centre.x + s * kr * 0.9f, centre.y - c * kr * 0.9f);
    g.setColour (juce::Colour (0xff8d8d94));
    g.strokePath (needle, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void RiserLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = (float) y + (float) height * 0.5f;
    const float x0 = (float) x + 4.0f, x1 = (float) (x + width) - 4.0f;
    g.setColour (juce::Colour (0xff2a2a2e));
    g.fillRoundedRectangle (x0, cy - 1.5f, x1 - x0, 3.0f, 1.5f);
    g.setColour (pr::accent);
    g.fillRoundedRectangle (x0, cy - 1.5f, sliderPos - x0, 3.0f, 1.5f);
    g.setColour (juce::Colour (0xffd8d8de));
    g.fillEllipse (sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f);
}

//==============================================================================
FlatButton::FlatButton (const juce::String& name, Icon ic, const juce::String& label)
    : juce::Button (name), icon (ic), text (label)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void FlatButton::setLabel (const juce::String& t)
{
    if (t != text) { text = t; repaint(); }
}

void FlatButton::setIcon (Icon ic)
{
    if (ic != icon) { icon = ic; repaint(); }
}

void FlatButton::paintButton (juce::Graphics& g, bool over, bool)
{
    const auto b = getLocalBounds().toFloat();
    const bool on = getToggleState();
    juce::Colour col = on ? onColour : (over ? offColour.brighter (0.6f) : offColour);

    const auto font = pr::makeFont (fontSize, bold, kerning);
    const float tw = text.isEmpty() ? 0.0f : pr::textWidth (font, text);
    const float isz = icon == Icon::None ? 0.0f : 14.0f;
    const float gap = (icon != Icon::None && text.isNotEmpty()) ? 7.0f : 0.0f;
    const float total = isz + gap + tw;

    float sx = b.getX();
    if (align == Align::Centre) sx = b.getCentreX() - total * 0.5f;
    else if (align == Align::Right) sx = b.getRight() - total;

    const float cy = b.getCentreY();

    if (icon != Icon::None)
    {
        const float cx = sx + isz * 0.5f;
        if (on && icon == Icon::Power)
        {
            g.setColour (col.withAlpha (0.18f));
            g.fillEllipse (cx - 11.0f, cy - 11.0f, 22.0f, 22.0f);
        }
        g.setColour (col);
        const juce::PathStrokeType stroke (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path p;
        const float r = 5.5f;
        switch (icon)
        {
            case Icon::Power:
                p.addCentredArc (cx, cy + 1.0f, r, r, 0.0f, 0.65f, juce::MathConstants<float>::twoPi - 0.65f, true);
                p.startNewSubPath (cx, cy - r - 1.0f);
                p.lineTo (cx, cy);
                g.strokePath (p, stroke);
                break;
            case Icon::Play:
                p.addTriangle (cx - 4.0f, cy - 6.0f, cx - 4.0f, cy + 6.0f, cx + 6.0f, cy);
                g.fillPath (p);
                break;
            case Icon::Stop:
                g.fillRoundedRectangle (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f, 1.5f);
                break;
            case Icon::Loop:
                p.addRoundedRectangle (cx - 6.5f, cy - 4.0f, 13.0f, 8.0f, 3.0f);
                g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.fillRect (cx + 1.0f, cy - 6.0f, 6.0f, 4.0f);
                g.fillRect (cx - 7.0f, cy + 2.0f, 6.0f, 4.0f);
                {
                    juce::Path a1, a2;
                    a1.addTriangle (cx + 2.0f, cy - 8.0f, cx + 2.0f, cy - 0.5f, cx + 6.5f, cy - 4.2f);
                    a2.addTriangle (cx - 2.0f, cy + 8.0f, cx - 2.0f, cy + 0.5f, cx - 6.5f, cy + 4.2f);
                    g.fillPath (a1); g.fillPath (a2);
                }
                break;
            case Icon::Reset:
                p.addCentredArc (cx, cy, r, r, 0.0f, 0.9f, juce::MathConstants<float>::twoPi, true);
                g.strokePath (p, stroke);
                {
                    juce::Path ar;
                    ar.addTriangle (cx - 3.5f, cy - r - 3.0f, cx - 3.5f, cy - r + 3.0f, cx + 1.5f, cy - r);
                    g.fillPath (ar);
                }
                break;
            case Icon::Prev:
                p.startNewSubPath (cx + 3.0f, cy - 6.0f); p.lineTo (cx - 3.0f, cy); p.lineTo (cx + 3.0f, cy + 6.0f);
                g.strokePath (p, stroke);
                break;
            case Icon::Next:
                p.startNewSubPath (cx - 3.0f, cy - 6.0f); p.lineTo (cx + 3.0f, cy); p.lineTo (cx - 3.0f, cy + 6.0f);
                g.strokePath (p, stroke);
                break;
            case Icon::Save:
                p.startNewSubPath (cx - 7.0f, cy + 3.0f); p.lineTo (cx - 7.0f, cy - 7.0f); p.lineTo (cx + 3.0f, cy - 7.0f);
                p.lineTo (cx + 7.0f, cy - 3.0f); p.lineTo (cx + 7.0f, cy - 1.0f);
                p.startNewSubPath (cx - 7.0f, cy + 3.0f); p.lineTo (cx - 7.0f, cy + 7.0f); p.lineTo (cx - 2.0f, cy + 7.0f);
                p.startNewSubPath (cx + 4.0f, cy + 2.0f); p.lineTo (cx + 4.0f, cy + 8.0f);
                p.startNewSubPath (cx + 1.0f, cy + 5.0f); p.lineTo (cx + 7.0f, cy + 5.0f);
                g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            default: break;
        }
    }

    if (text.isNotEmpty())
    {
        g.setColour (col);
        g.setFont (font);
        g.drawText (text, juce::Rectangle<float> (sx + isz + gap, b.getY(), tw + 8.0f, b.getHeight()),
                    juce::Justification::centredLeft, false);
    }
}

//==============================================================================
void WaveView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (pr::screen);
    g.fillRoundedRectangle (b, 18.0f);

    // brand
    const auto light = pr::makeFont (15.0f, false, 0.26f);
    const auto heavy = pr::makeFont (15.0f, true, 0.26f);
    const float bx = 56.0f;
    g.setColour (juce::Colour (0xffc9c9cf));
    g.drawEllipse (16.0f + 2.0f, 16.0f + 2.0f, 20.0f, 20.0f, 1.5f);
    g.drawEllipse (16.0f + 7.0f, 16.0f + 7.0f, 10.0f, 10.0f, 1.5f);
    g.fillEllipse (16.0f + 10.5f, 16.0f + 10.5f, 3.0f, 3.0f);
    g.setFont (light);
    g.setColour (juce::Colour (0xff8d8d95));
    g.drawText ("POAS", juce::Rectangle<float> (bx, 14.0f, 80.0f, 24.0f), juce::Justification::centredLeft, false);
    const float w1 = pr::textWidth (light, "POAS");
    g.setFont (heavy);
    g.setColour (juce::Colour (0xffd7d7dc));
    g.drawText ("RISER", juce::Rectangle<float> (bx + w1, 14.0f, 100.0f, 24.0f), juce::Justification::centredLeft, false);

    // bars
    const float a = proc.apvts.getRawParameterValue ("amount")->load() / 100.0f;
    const bool rev = proc.apvts.getRawParameterValue ("mode")->load() < 0.5f;
    const float p = proc.getProgress();
    const int n = 118;
    const float x0 = b.getWidth() * 0.27f, x1 = b.getWidth() * 0.76f;
    const float mid = b.getHeight() * 0.46f;
    const float maxH = b.getHeight() * (0.30f + 0.24f * a);
    const float bw = juce::jmax (1.5f, (x1 - x0) / (float) n * 0.5f);

    juce::ColourGradient glow (pr::accent.withAlpha (0.22f), (x0 + x1) * 0.5f, mid,
                               pr::accent.withAlpha (0.0f), x1 + 20.0f, mid, true);
    g.setGradientFill (glow);
    g.fillEllipse (x0 - 30.0f, mid - maxH * 0.7f, (x1 - x0) + 60.0f, maxH * 1.4f);

    const float hue = 255.0f / 360.0f;
    for (int i = 0; i < n; ++i)
    {
        const float f = (float) i / (float) (n - 1);
        const float e = std::pow (rev ? f : 1.0f - f, 1.7f);
        const float bh = 2.5f + e * maxH;
        const float x = x0 + (x1 - x0) * f;

        juce::Colour c;
        if (p < 0.0f)                     c = juce::Colour::fromHSL (hue, 0.62f + e * 0.2f, 0.44f + e * 0.22f, 1.0f);
        else if (std::abs (f - p) < 0.012f) c = juce::Colours::white;
        else if (f < p)                   c = juce::Colour::fromHSL (hue, 1.0f, 0.74f, 1.0f);
        else                              c = juce::Colour::fromHSL (hue, 0.38f, 0.28f, 1.0f);

        g.setColour (c.withAlpha (0.10f));
        g.fillRect (x - bw * 1.6f, mid - bh * 0.5f - 2.0f, bw * 3.2f, bh + 4.0f);
        g.setColour (c);
        g.fillRect (x - bw * 0.5f, mid - bh * 0.5f, bw, bh);
    }
}

//==============================================================================
PoasRiserEditor::PoasRiserEditor (PoasRiserProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), wave (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (wave);

    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (knobStart, knobEnd, true);
    knob.setMouseDragSensitivity (260);
    knob.setDoubleClickReturnValue (true, 50.0);
    knob.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (knob);

    level.setSliderStyle (juce::Slider::LinearHorizontal);
    level.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    level.setDoubleClickReturnValue (true, 80.0);
    addAndMakeVisible (level);

    knobAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "amount", knob);
    levelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "level", level);

    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &resetBtn, &listenBtn, &loopBtn, &rawBtn, &editBtn,
                     &modeBtn, &lenBtn, &phoneBtn, &chorusBtn, &tremBtn })
        addAndMakeVisible (*b);

    resetBtn.fontSize = 11.5f; resetBtn.align = FlatButton::Align::Right;
    for (auto* b : { &listenBtn, &loopBtn, &rawBtn, &editBtn })
    {
        b->fontSize = 11.5f;
    }
    listenBtn.align = FlatButton::Align::Left;
    loopBtn.align = FlatButton::Align::Left;
    listenBtn.onColour = pr::accentHi;
    loopBtn.setClickingTogglesState (true);
    loopBtn.offColour = juce::Colour (0xff7d7d85);
    listenBtn.offColour = juce::Colour (0xff7d7d85);
    resetBtn.offColour = juce::Colour (0xff7b7b83);
    rawBtn.offColour = juce::Colour (0xff6d6d75);
    editBtn.offColour = juce::Colour (0xff6d6d75);
    rawBtn.onColour = pr::accent; editBtn.onColour = pr::accent;
    rawBtn.align = FlatButton::Align::Right; editBtn.align = FlatButton::Align::Right;

    modeBtn.fontSize = 18.0f; modeBtn.align = FlatButton::Align::Right;
    modeBtn.offColour = juce::Colour (0xffd3d3d8);
    lenBtn.fontSize = 18.0f; lenBtn.align = FlatButton::Align::Left;
    lenBtn.offColour = juce::Colour (0xff5f5f67); lenBtn.bold = false;

    for (auto* b : { &phoneBtn, &chorusBtn, &tremBtn })
    {
        b->setClickingTogglesState (true);
        b->fontSize = 14.0f; b->kerning = 0.14f;
        b->onColour = pr::warm; b->offColour = juce::Colour (0xff6f6f77);
    }
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn })
        b->offColour = juce::Colour (0xffc9c9cf);

    loopAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "loop", loopBtn);
    phoneAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "phone", phoneBtn);
    chorusAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "chorus", chorusBtn);
    tremAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "tremolo", tremBtn);

    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() - 1 + n) % n); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    saveBtn.onClick = [this] { proc.saveUserPreset(); };
    resetBtn.onClick = [this]
    {
        proc.setCurrentProgram (0);
        setBool ("loop", false);
        if (auto* prm = proc.apvts.getParameter ("level")) prm->setValueNotifyingHost (prm->convertTo0to1 (80.0f));
    };
    listenBtn.onClick = [this] { if (proc.isRiserActive()) proc.requestStop(); else proc.requestPreview(); };
    rawBtn.onClick = [this] { setBool ("edit", false); };
    editBtn.onClick = [this] { setBool ("edit", true); };
    modeBtn.onClick = [this] { setChoice ("mode", 1 - getChoice ("mode")); };
    lenBtn.onClick = [this] { setChoice ("bars", (getChoice ("bars") + 1) % 5); };

    setSize (540, 690);
    setResizable (false, false);
    startTimerHz (30);
    timerCallback();
}

PoasRiserEditor::~PoasRiserEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PoasRiserEditor::setChoice (const char* id, int index)
{
    if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        *prm = index;
}

void PoasRiserEditor::setBool (const char* id, bool value)
{
    if (auto* prm = dynamic_cast<juce::AudioParameterBool*> (proc.apvts.getParameter (id)))
        *prm = value;
}

int PoasRiserEditor::getChoice (const char* id) const
{
    return (int) proc.apvts.getRawParameterValue (id)->load();
}

void PoasRiserEditor::timerCallback()
{
    const bool active = proc.isRiserActive();
    listenBtn.setLabel (active ? "STOP" : "LISTEN");
    listenBtn.setIcon (active ? FlatButton::Icon::Stop : FlatButton::Icon::Play);
    listenBtn.setToggleState (active, juce::dontSendNotification);

    const bool ed = proc.apvts.getRawParameterValue ("edit")->load() > 0.5f;
    rawBtn.setToggleState (! ed, juce::dontSendNotification);
    editBtn.setToggleState (ed, juce::dontSendNotification);
    for (auto* b : { &phoneBtn, &chorusBtn, &tremBtn })
        b->setAlpha (ed ? 1.0f : 0.4f);

    modeBtn.setLabel (getChoice ("mode") == 0 ? "REVERSE" : "FALL");
    lenBtn.setLabel (barLabels[juce::jlimit (0, 4, getChoice ("bars"))]);

    const auto name = proc.getProgramName (proc.getCurrentProgram());
    if (name != presetName) { presetName = name; repaint (78, 16, 190, 36); }

    wave.repaint();
    repaint (150, 578, 240, 22);
}

void PoasRiserEditor::resized()
{
    wave.setBounds (20, 76, 500, 220);
    resetBtn.setBounds (400, 84, 100, 30);
    listenBtn.setBounds (34, 262, 100, 24);
    loopBtn.setBounds (142, 262, 72, 24);
    rawBtn.setBounds (386, 262, 46, 24);
    editBtn.setBounds (440, 262, 60, 24);

    prevBtn.setBounds (268, 16, 34, 36);
    nextBtn.setBounds (302, 16, 34, 36);
    saveBtn.setBounds (346, 16, 38, 36);
    level.setBounds (404, 30, 116, 22);

    knob.setBounds (145, 280, 250, 250);
    modeBtn.setBounds (125, 538, 150, 32);
    lenBtn.setBounds (285, 538, 130, 32);

    phoneBtn.setBounds (91, 616, 110, 32);
    chorusBtn.setBounds (215, 616, 110, 32);
    tremBtn.setBounds (339, 616, 120, 32);
}

void PoasRiserEditor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (pr::shell);
    g.fillRoundedRectangle (b, 22.0f);

    // logo
    g.setColour (juce::Colour (0xffd9d9de));
    g.setFont (juce::Font (juce::FontOptions ("Times New Roman", 38.0f, juce::Font::italic)));
    g.drawText ("p", juce::Rectangle<int> (24, 8, 40, 50), juce::Justification::centredLeft, false);

    // preset bar
    const juce::Rectangle<float> pbox (78.0f, 16.0f, 258.0f, 36.0f);
    g.setColour (juce::Colour (0xff0b0b0d));
    g.fillRoundedRectangle (pbox, 8.0f);
    g.setColour (pr::line);
    g.drawRoundedRectangle (pbox, 8.0f, 1.0f);
    g.drawLine (268.0f, 16.0f, 268.0f, 52.0f);
    g.drawLine (302.0f, 16.0f, 302.0f, 52.0f);
    g.setColour (pr::fg);
    g.setFont (pr::makeFont (13.0f, true, 0.04f));
    g.drawText (presetName, juce::Rectangle<int> (94, 16, 168, 36), juce::Justification::centredLeft, true);

    const juce::Rectangle<float> sbox (346.0f, 16.0f, 38.0f, 36.0f);
    g.setColour (juce::Colour (0xff0b0b0d));
    g.fillRoundedRectangle (sbox, 8.0f);
    g.setColour (pr::line);
    g.drawRoundedRectangle (sbox, 8.0f, 1.0f);

    g.setColour (pr::dim);
    g.setFont (pr::makeFont (9.5f, true, 0.18f));
    g.drawText ("LEVEL", juce::Rectangle<int> (404, 14, 80, 14), juce::Justification::centredLeft, false);

    // panel
    const juce::Rectangle<float> panel (10.0f, 66.0f, 520.0f, 614.0f);
    juce::ColourGradient grad (pr::panelHi, 0.0f, panel.getY(), pr::panelLo, 0.0f, panel.getBottom() - 150.0f, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (panel, 26.0f);
    g.setColour (juce::Colour (0xff2a2a2e));
    g.drawRoundedRectangle (panel, 26.0f, 1.0f);

    // corner dot clusters
    g.setColour (juce::Colour (0xff2e2e32));
    auto cluster = [&g] (float cx, float cy, int sx, int sy)
    {
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3 - r; ++c)
                g.fillEllipse (cx + (float) (sx * c) * 14.0f - 2.6f, cy + (float) (sy * r) * 14.0f - 2.6f, 5.2f, 5.2f);
    };
    cluster (34.0f, 316.0f, 1, 1);
    cluster (506.0f, 316.0f, -1, 1);
    cluster (34.0f, 656.0f, 1, -1);
    cluster (506.0f, 656.0f, -1, -1);

    // readout
    const float amt = proc.apvts.getRawParameterValue ("amount")->load();
    const auto small = pr::makeFont (10.5f, true, 0.2f);
    const juce::String lbl = "INTENSITY ";
    const juce::String val = juce::String (juce::roundToInt (amt));
    const float tw = pr::textWidth (small, lbl) + pr::textWidth (small, val);
    const float rx = 270.0f - tw * 0.5f;
    g.setFont (small);
    g.setColour (juce::Colour (0xff55555c));
    g.drawText (lbl, juce::Rectangle<float> (rx, 580.0f, 120.0f, 16.0f), juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xff8d8d95));
    g.drawText (val, juce::Rectangle<float> (rx + pr::textWidth (small, lbl), 580.0f, 60.0f, 16.0f), juce::Justification::centredLeft, false);
}
