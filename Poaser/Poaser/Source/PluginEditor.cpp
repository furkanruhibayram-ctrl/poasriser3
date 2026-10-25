#include "PluginEditor.h"

namespace
{
juce::Font makeFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}
}

//==============================================================================
PoaserLookAndFeel::PoaserLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffdfe3f0));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xffa9b0c6));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff222635));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffeaeefa));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff343a50));
    setColour (juce::ComboBox::arrowColourId, accent2());
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c2030));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffeaeefa));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent());
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff222635));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0475b));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeaeefa));
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void PoaserLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float startAngle, float endAngle,
                                          juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float lineW = 4.f;
    const float arcR = radius - lineW * 0.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2f43));
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, angle, true);
    g.setGradientFill (juce::ColourGradient (accent(), bounds.getX(), bounds.getBottom(),
                                             accent2(), bounds.getRight(), bounds.getY(), false));
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyR = radius - 9.f;
    g.setColour (juce::Colour (0xff232838));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.f, bodyR * 2.f);
    g.setColour (juce::Colour (0xff394060));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.f, bodyR * 2.f, 1.f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -bodyR + 3.f, 3.f, bodyR * 0.45f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

//==============================================================================
void DragHandle::setFile (const juce::File& f)
{
    file = f;
    repaint();
}

void DragHandle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool ready = file.existsAsFile();
    g.setColour (ready ? PoaserLookAndFeel::accent().withAlpha (0.25f) : juce::Colour (0xff1e2230));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (ready ? PoaserLookAndFeel::accent2() : juce::Colour (0xff343a50));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.5f);
    g.setColour (ready ? juce::Colours::white : juce::Colour (0xff7d849c));
    g.setFont (makeFont (13.f, true));
    g.drawText (ready ? "DRAG RISER INTO YOUR DAW  >>" : "Render first, then drag the audio here",
                getLocalBounds(), juce::Justification::centred);
}

void DragHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (file.existsAsFile() && e.getDistanceFromDragStart() > 5)
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
PoaserEditor::PoaserEditor (PoaserProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    setSize (900, 548);

    addKnob ("predelay", "PREDELAY");
    addKnob ("lowcut", "LOW CUT");
    addKnob ("highcut", "HIGH CUT");
    addKnob ("size", "SIZE");
    addKnob ("diffusion", "DIFFUSION");
    addKnob ("decay", "DECAY");
    addKnob ("damping", "DAMPING");
    addKnob ("modspeed", "MOD SPEED");
    addKnob ("moddepth", "MOD DEPTH");
    addKnob ("width", "STEREO");

    addKnob ("pitch", "PITCH");
    addKnob ("fine", "FINE");
    addKnob ("pitchmix", "PITCH MIX");
    addKnob ("shimmer", "SHIMMER");
    addKnob ("grain", "GRAIN");

    addKnob ("revmix", "REVERSE");
    addKnob ("rise", "RISE TIME");

    addKnob ("dry", "DRY");
    addKnob ("wet", "WET");
    addKnob ("out", "OUTPUT");

    // Pitch mode
    modeLabel.setText ("PITCH MODE", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setFont (makeFont (11.f, true));
    addAndMakeVisible (modeLabel);
    modeBox.addItemList ({ "Pre Reverb", "Post Reverb", "Shimmer" }, 1);
    addAndMakeVisible (modeBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "pitchmode", modeBox);

    // Presets
    {
        const auto& presets = poaser::getPresets();
        juce::String lastCat;
        for (size_t i = 0; i < presets.size(); ++i)
        {
            if (lastCat != presets[i].category)
            {
                lastCat = presets[i].category;
                presetBox.addSectionHeading (lastCat);
            }
            presetBox.addItem (presets[i].name, (int) i + 1);
        }
        presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
        presetBox.onChange = [this]
        {
            const int id = presetBox.getSelectedId();
            if (id > 0) processor.setCurrentProgram (id - 1);
        };
        addAndMakeVisible (presetBox);
        addAndMakeVisible (prevButton);
        addAndMakeVisible (nextButton);
        prevButton.onClick = [this] { stepPreset (-1); };
        nextButton.onClick = [this] { stepPreset (1); };
    }

    // Riser builder
    recButton.setClickingTogglesState (true);
    recButton.onClick = [this]
    {
        if (recButton.getToggleState()) processor.startCapture();
        else processor.stopCapture();
    };
    clearButton.onClick = [this]
    {
        processor.clearCapture();
        lastRendered = juce::File();
        dragHandle.setFile (lastRendered);
    };
    renderButton.onClick = [this]
    {
        lastRendered = processor.renderRiser();
        dragHandle.setFile (lastRendered);
        if (! lastRendered.existsAsFile())
            infoLabel.setText ("Nothing captured yet. Press REC, play audio, then press REC again.", juce::dontSendNotification);
    };
    saveButton.onClick = [this]
    {
        if (! lastRendered.existsAsFile()) return;
        chooser = std::make_unique<juce::FileChooser> ("Save riser",
            juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("Poaser Riser.wav"), "*.wav");
        juce::Component::SafePointer<PoaserEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (safe != nullptr && f != juce::File())
                                      safe->lastRendered.copyFileTo (f);
                              });
    };
    for (auto* b : { &recButton, &clearButton, &renderButton, &saveButton })
        addAndMakeVisible (b);

    infoLabel.setFont (makeFont (11.f));
    infoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (infoLabel);
    addAndMakeVisible (dragHandle);

    startTimerHz (10);
}

PoaserEditor::~PoaserEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PoaserEditor::addKnob (const juce::String& id, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setPopupDisplayEnabled (false, false, nullptr);
    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (makeFont (11.f, true));
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, id, k->slider);
    knobs.emplace_back (id, std::move (k));
}

Knob* PoaserEditor::knob (const juce::String& id)
{
    for (auto& pr : knobs)
        if (pr.first == id) return pr.second.get();
    return nullptr;
}

void PoaserEditor::layoutKnobs (std::initializer_list<const char*> ids, juce::Rectangle<int> area, int cols)
{
    const int count = (int) ids.size();
    const int rows = (count + cols - 1) / cols;
    const int cw = area.getWidth() / cols;
    const int ch = area.getHeight() / rows;
    int i = 0;
    for (auto* id : ids)
    {
        if (auto* k = knob (id))
        {
            auto cell = juce::Rectangle<int> (area.getX() + (i % cols) * cw, area.getY() + (i / cols) * ch, cw, ch);
            k->label.setBounds (cell.removeFromTop (16));
            k->slider.setBounds (cell);
        }
        ++i;
    }
}

void PoaserEditor::stepPreset (int delta)
{
    const int n = processor.getNumPrograms();
    const int next = (processor.getCurrentProgram() + delta + n) % n;
    processor.setCurrentProgram (next);
    presetBox.setSelectedId (next + 1, juce::dontSendNotification);
}

void PoaserEditor::timerCallback()
{
    const int cur = processor.getCurrentProgram();
    if (presetBox.getSelectedId() != cur + 1)
        presetBox.setSelectedId (cur + 1, juce::dontSendNotification);

    recButton.setToggleState (processor.isRecording(), juce::dontSendNotification);

    const double secs = processor.getCapturedSeconds();
    if (processor.isRecording())
        infoLabel.setText ("Recording... " + juce::String (secs, 1) + " s", juce::dontSendNotification);
    else if (secs > 0.0)
        infoLabel.setText ("Captured " + juce::String (secs, 1) + " s", juce::dontSendNotification);
    else if (! lastRendered.existsAsFile())
        infoLabel.setText ("REC, play audio, REC again, RENDER", juce::dontSendNotification);
}

//==============================================================================
void PoaserEditor::paint (juce::Graphics& g)
{
    g.fillAll (PoaserLookAndFeel::background());

    // Header
    g.setGradientFill (juce::ColourGradient (PoaserLookAndFeel::accent(), 20.f, 0.f,
                                             PoaserLookAndFeel::accent2(), 220.f, 40.f, false));
    g.setFont (makeFont (30.f, true));
    g.drawText ("POASER", 20, 8, 240, 40, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff7d849c));
    g.setFont (makeFont (11.f));
    g.drawText ("REVERB  /  PITCH  /  REVERSE RISER", 22, 40, 300, 14, juce::Justification::centredLeft);

    auto drawPanel = [&g] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (PoaserLookAndFeel::panel());
        g.fillRoundedRectangle (r.toFloat(), 10.f);
        g.setColour (juce::Colour (0xff262b3d));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.f, 1.f);
        g.setColour (PoaserLookAndFeel::accent2());
        g.setFont (makeFont (12.f, true));
        g.drawText (title, r.getX() + 14, r.getY() + 6, r.getWidth() - 28, 16, juce::Justification::centredLeft);
    };

    drawPanel (reverbArea, "REVERB");
    drawPanel (pitchArea, "PITCH SHIFTER");
    drawPanel (reverseArea, "REVERSE");
    drawPanel (outputArea, "OUTPUT");
    drawPanel (riserArea, "RISER BUILDER");
}

void PoaserEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (46);

    // preset bar
    auto bar = area.removeFromTop (30);
    auto barCentre = bar.withSizeKeepingCentre (400, 28);
    prevButton.setBounds (barCentre.removeFromLeft (34));
    nextButton.setBounds (barCentre.removeFromRight (34));
    presetBox.setBounds (barCentre.reduced (6, 0));

    area.removeFromTop (10);

    auto row1 = area.removeFromTop (262);
    reverbArea = row1.removeFromLeft (560);
    row1.removeFromLeft (12);
    pitchArea = row1;

    area.removeFromTop (12);
    auto row2 = area.removeFromTop (150);
    reverseArea = row2.removeFromLeft (232);
    row2.removeFromLeft (12);
    outputArea = row2.removeFromLeft (324);
    row2.removeFromLeft (12);
    riserArea = row2;

    auto inner = [] (juce::Rectangle<int> r) { return r.reduced (8).withTrimmedTop (18); };

    layoutKnobs ({ "predelay", "lowcut", "highcut", "size", "diffusion",
                   "decay", "damping", "modspeed", "moddepth", "width" }, inner (reverbArea), 5);

    // pitch panel: 3 x 2 grid, last cell holds the mode selector
    {
        auto pa = inner (pitchArea);
        layoutKnobs ({ "pitch", "fine", "pitchmix", "shimmer", "grain" }, pa, 3);
        const int cw = pa.getWidth() / 3;
        const int ch = pa.getHeight() / 2;
        auto cell = juce::Rectangle<int> (pa.getX() + 2 * cw, pa.getY() + ch, cw, ch);
        modeLabel.setBounds (cell.removeFromTop (16));
        modeBox.setBounds (cell.withTrimmedTop (22).withHeight (26).reduced (4, 0));
    }

    layoutKnobs ({ "revmix", "rise" }, inner (reverseArea), 2);
    layoutKnobs ({ "dry", "wet", "out" }, inner (outputArea), 3);

    // riser builder
    {
        auto ra = riserArea.reduced (12).withTrimmedTop (18);
        auto r1 = ra.removeFromTop (28);
        recButton.setBounds (r1.removeFromLeft (70));
        r1.removeFromLeft (6);
        clearButton.setBounds (r1.removeFromLeft (70));
        r1.removeFromLeft (8);
        infoLabel.setBounds (r1);

        ra.removeFromTop (8);
        auto r2 = ra.removeFromTop (28);
        renderButton.setBounds (r2.removeFromLeft (r2.getWidth() / 2 - 3));
        r2.removeFromLeft (6);
        saveButton.setBounds (r2);

        ra.removeFromTop (8);
        dragHandle.setBounds (ra.removeFromTop (40));
    }
}
