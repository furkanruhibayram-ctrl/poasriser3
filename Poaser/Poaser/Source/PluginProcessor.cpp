#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace poaser;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PoaserProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using Range = juce::NormalisableRange<float>;
    using Attr  = juce::AudioParameterFloatAttributes;

    auto addF = [&layout] (const juce::String& id, const juce::String& name, Range range, float def,
                           std::function<juce::String (float)> toText,
                           std::function<float (const juce::String&)> fromText = nullptr)
    {
        Attr a;
        a = a.withStringFromValueFunction ([toText] (float v, int) { return toText (v); });
        if (fromText)
            a = a.withValueFromStringFunction ([fromText] (const juce::String& s) { return fromText (s); });
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def, a));
    };

    auto unit = [] (const juce::String& u, int dec)
    {
        return [u, dec] (float v) { return juce::String (v, dec) + u; };
    };
    auto percent = [] (float v) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; };
    auto fromPercent = [] (const juce::String& s) { return s.getFloatValue() / 100.f; };

    addF ("predelay", "Predelay",  Range (0.f, 500.f, 0.f, 0.5f), 0.f,   unit (" ms", 0));
    addF ("lowcut",   "Low Cut",   Range (20.f, 1000.f, 0.f, 0.35f), 20.f, unit (" Hz", 0));
    addF ("highcut",  "High Cut",  Range (1000.f, 20000.f, 0.f, 0.4f), 20000.f, unit (" Hz", 0));
    addF ("size",     "Size",      Range (0.f, 1.f), 0.5f, percent, fromPercent);
    addF ("diffusion","Diffusion", Range (0.f, 1.f), 0.8f, percent, fromPercent);
    addF ("decay",    "Decay",     Range (0.1f, 30.f, 0.f, 0.4f), 2.f, unit (" s", 2));
    addF ("damping",  "Damping",   Range (0.f, 1.f), 0.4f, percent, fromPercent);
    addF ("modspeed", "Mod Speed", Range (0.05f, 8.f, 0.f, 0.4f), 0.5f, unit (" Hz", 2));
    addF ("moddepth", "Mod Depth", Range (0.f, 1.f), 0.2f, percent, fromPercent);
    addF ("width",    "Stereo",    Range (0.f, 2.f), 1.f, [] (float v) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; }, fromPercent);
    addF ("dry",      "Dry",       Range (0.f, 1.f), 1.f, percent, fromPercent);
    addF ("wet",      "Wet",       Range (0.f, 1.f), 0.35f, percent, fromPercent);
    addF ("pitch",    "Pitch",     Range (-24.f, 24.f, 1.f), 0.f, unit (" st", 0));
    addF ("fine",     "Fine",      Range (-100.f, 100.f, 1.f), 0.f, unit (" ct", 0));
    addF ("pitchmix", "Pitch Mix", Range (0.f, 1.f), 0.f, percent, fromPercent);

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pitchmode", 1 }, "Pitch Mode",
        juce::StringArray { "Pre Reverb", "Post Reverb", "Shimmer" }, 0));

    addF ("shimmer",  "Shimmer",   Range (0.f, 1.f), 0.f, percent, fromPercent);
    addF ("grain",    "Grain",     Range (20.f, 200.f, 1.f), 80.f, unit (" ms", 0));
    addF ("revmix",   "Reverse",   Range (0.f, 1.f), 0.f, percent, fromPercent);
    addF ("rise",     "Rise Time", Range (100.f, 4000.f, 0.f, 0.5f), 1000.f, unit (" ms", 0));
    addF ("out",      "Output",    Range (-24.f, 12.f, 0.1f), 0.f, unit (" dB", 1));

    return layout;
}

//==============================================================================
PoaserProcessor::PoaserProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "POASER", createLayout())
{
    for (int i = 0; i < kNumParams; ++i)
        raw[i] = apvts.getRawParameterValue (kParamIds[i]);
}

Params PoaserProcessor::getParams() const
{
    auto v = [this] (int i) { return raw[i]->load(); };
    Params p;
    p.predelayMs = v (0);
    p.lowcut     = v (1);
    p.highcut    = v (2);
    p.size       = v (3);
    p.diffusion  = v (4);
    p.decay      = v (5);
    p.damping    = v (6);
    p.modSpeed   = v (7);
    p.modDepth   = v (8);
    p.width      = v (9);
    p.dry        = v (10);
    p.wet        = v (11);
    p.pitch      = v (12);
    p.fine       = v (13);
    p.pitchMix   = v (14);
    p.pitchMode  = juce::jlimit (0, 2, (int) std::lround (v (15)));
    p.shimmer    = v (16);
    p.grainMs    = v (17);
    p.revMix     = v (18);
    p.riseMs     = v (19);
    p.outDb      = v (20);
    return p;
}

//==============================================================================
bool PoaserProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PoaserProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);
    engine.setParams (getParams(), true);

    recording.store (false);
    capture.setSize (2, (int) (sampleRate * kMaxCaptureSeconds));
    capture.clear();
    captureLen.store (0);
}

void PoaserProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numCh < 1) return;

    float* l = buffer.getWritePointer (0);
    float* r = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    if (recording.load())
    {
        const int pos = captureLen.load();
        const int n = std::min (numSamples, capture.getNumSamples() - pos);
        if (n > 0)
        {
            capture.copyFrom (0, pos, l, n);
            capture.copyFrom (1, pos, r != nullptr ? r : l, n);
            captureLen.store (pos + n);
        }
        else
            recording.store (false);
    }

    engine.setParams (getParams());

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = l[i];
        const float inR = r != nullptr ? r[i] : inL;
        float oL = 0.f, oR = 0.f;
        engine.process (inL, inR, oL, oR);
        l[i] = oL;
        if (r != nullptr) r[i] = oR;
    }
}

//==============================================================================
int PoaserProcessor::getNumPrograms() { return (int) getPresets().size(); }

void PoaserProcessor::setCurrentProgram (int index)
{
    const auto& presets = getPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;

    const auto& p = presets[(size_t) index];
    for (int i = 0; i < kNumParams; ++i)
        if (auto* param = apvts.getParameter (kParamIds[i]))
            param->setValueNotifyingHost (param->convertTo0to1 (p.v[i]));
}

const juce::String PoaserProcessor::getProgramName (int index)
{
    const auto& presets = getPresets();
    if (index < 0 || index >= (int) presets.size()) return {};
    const auto& p = presets[(size_t) index];
    return juce::String (p.category) + " - " + p.name;
}

void PoaserProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PoaserProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
}

//==============================================================================
void PoaserProcessor::startCapture()
{
    captureLen.store (0);
    recording.store (true);
}

void PoaserProcessor::stopCapture() { recording.store (false); }

void PoaserProcessor::clearCapture()
{
    recording.store (false);
    captureLen.store (0);
}

double PoaserProcessor::getCapturedSeconds() const
{
    return (double) captureLen.load() / currentSampleRate;
}

// Reverse reverb, done offline: reverse the captured audio, run it through the
// whole chain (reverb + pitch), then reverse the result. The swell therefore
// builds up *into* the start of the original clip.
juce::File PoaserProcessor::renderRiser()
{
    recording.store (false);
    juce::Thread::sleep (30); // let a running audio block finish

    const int n = captureLen.load();
    if (n < 256) return {};

    juce::ScopedNoDenormals noDenormals;

    Params p = getParams();
    p.revMix = 0.f; // we reverse offline instead

    const double sr = currentSampleRate;
    const double tailSeconds = juce::jlimit (1.0, 20.0, (double) p.decay * 1.3 + p.predelayMs * 0.001 + 0.5);
    const int tail  = (int) (tailSeconds * sr);
    const int total = n + tail;

    Engine eng;
    eng.prepare (sr);
    eng.setParams (p, true);

    juce::AudioBuffer<float> out (2, total);
    out.clear();

    const float* cl = capture.getReadPointer (0);
    const float* cr = capture.getReadPointer (1);
    float* ol = out.getWritePointer (0);
    float* orr = out.getWritePointer (1);

    for (int i = 0; i < total; ++i)
    {
        float inL = 0.f, inR = 0.f;
        if (i < n) { inL = cl[n - 1 - i]; inR = cr[n - 1 - i]; }
        eng.process (inL, inR, ol[i], orr[i]);
    }

    out.reverse (0, total);

    const float peak = out.getMagnitude (0, total);
    if (peak > 0.97f)
        out.applyGain (0.97f / peak);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Poaser");
    dir.createDirectory();
    auto file = dir.getChildFile ("Poaser_Riser_" + juce::Time::getCurrentTime().formatted ("%H%M%S") + ".wav");

    juce::WavAudioFormat wav;
    auto stream = std::make_unique<juce::FileOutputStream> (file);
    if (stream->failedToOpen()) return {};
    stream->setPosition (0);
    stream->truncate();

    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
    if (writer == nullptr) return {};
    stream.release(); // the writer owns it now
    writer->writeFromAudioSampleBuffer (out, 0, total);
    writer.reset();

    return file;
}

//==============================================================================
juce::AudioProcessorEditor* PoaserProcessor::createEditor()
{
    return new PoaserEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PoaserProcessor();
}
