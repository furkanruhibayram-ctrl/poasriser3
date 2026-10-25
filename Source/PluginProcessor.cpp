#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::StringArray barNames { "1/2 Bar", "1 Bar", "2 Bars", "4 Bars", "8 Bars" };
    const float barValues[] = { 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };

    const RiserPreset factoryPresets[] = {
        { "Default",    50.0f, 1, 0, false, true,  true,  true },
        { "Airlift",    82.0f, 2, 0, false, true,  false, true },
        { "Tape Climb", 36.0f, 3, 0, false, false, true,  true },
        { "Phone Call", 62.0f, 1, 0, true,  false, true,  true },
        { "Slow Burn",  24.0f, 3, 0, false, true,  true,  true },
        { "Drop Tail",  72.0f, 2, 1, false, true,  false, true }
    };
    constexpr int numFactory = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

    inline float polyBlep (float t, float dt)
    {
        if (t < dt)         { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt)  { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    inline float sawBlep (float phase, float dt)
    {
        return 2.0f * phase - 1.0f - polyBlep (phase, dt);
    }
}

//==============================================================================
PoasRiserProcessor::PoasRiserProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    amountP  = apvts.getRawParameterValue ("amount");
    barsP    = apvts.getRawParameterValue ("bars");
    modeP    = apvts.getRawParameterValue ("mode");
    loopP    = apvts.getRawParameterValue ("loop");
    editP    = apvts.getRawParameterValue ("edit");
    phoneP   = apvts.getRawParameterValue ("phone");
    chorusP  = apvts.getRawParameterValue ("chorus");
    tremoloP = apvts.getRawParameterValue ("tremolo");
    levelP   = apvts.getRawParameterValue ("level");
}

juce::AudioProcessorValueTreeState::ParameterLayout PoasRiserProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "amount", 1 }, "Intensity",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Length", barNames, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Direction", StringArray { "Reverse", "Fall" }, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "loop", 1 }, "Loop", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "edit", 1 }, "Edit", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "phone", 1 }, "Phone", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "chorus", 1 }, "Chorus", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "tremolo", 1 }, "Tremolo", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "level", 1 }, "Level",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f));
    return layout;
}

//==============================================================================
void PoasRiserProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };

    noiseBP.prepare (spec);  noiseBP.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    oscLP.prepare (spec);    oscLP.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    oscLP.setResonance (0.8f);
    phoneHP.prepare (spec);  phoneHP.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    phoneHP.setCutoffFrequency (520.0f); phoneHP.setResonance (0.9f);
    phoneLP.prepare (spec);  phoneLP.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    phoneLP.setCutoffFrequency (3100.0f); phoneLP.setResonance (1.1f);
    noiseBP.reset(); oscLP.reset(); phoneHP.reset(); phoneLP.reset();

    const int size = juce::nextPowerOfTwo ((int) (sampleRate * 0.07));
    delayL.assign ((size_t) size, 0.0f);
    delayR.assign ((size_t) size, 0.0f);
    delayMask = size - 1;
    delayWrite = 0;

    phoneMix.reset (sampleRate, 0.03);  phoneMix.setCurrentAndTargetValue (0.0f);
    chorusMix.reset (sampleRate, 0.03); chorusMix.setCurrentAndTargetValue (0.0f);
    tremMix.reset (sampleRate, 0.03);   tremMix.setCurrentAndTargetValue (0.0f);

    voice = Voice();
    voiceActiveFlag.store (false);
    progress.store (-1.0f);
}

bool PoasRiserProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PoasRiserProcessor::triggerVoice (int note)
{
    const int barsIdx = juce::jlimit (0, 4, (int) barsP->load());
    voice.len = juce::jmax (1.0, (double) barValues[barsIdx] * 4.0 * 60.0 / currentBpm * sr);
    voice.pos = 0.0;
    voice.active = true;
    voice.releasing = false;
    voice.rel = 1.0f;
    voice.note = note;
    voice.noteRatio = std::pow (2.0f, (float) (note - 60) / 12.0f);
    phase1 = phase2 = 0.0f;
}

void PoasRiserProcessor::releaseVoice()
{
    if (voice.active)
        voice.releasing = true;
}

void PoasRiserProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // host tempo
    currentBpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 20.0 && *bpm < 400.0)
                    currentBpm = *bpm;

    const float a = amountP->load() / 100.0f;
    const bool rev = modeP->load() < 0.5f;
    const bool loop = loopP->load() > 0.5f;
    const bool ed = editP->load() > 0.5f;
    const float outGain = levelP->load() / 100.0f * 0.85f;

    phoneMix.setTargetValue (ed && phoneP->load() > 0.5f ? 1.0f : 0.0f);
    chorusMix.setTargetValue (ed && chorusP->load() > 0.5f ? 1.0f : 0.0f);
    tremMix.setTargetValue (ed && tremoloP->load() > 0.5f ? 1.0f : 0.0f);

    const int req = previewRequest.exchange (0);
    if (req == 1) triggerVoice (60);
    else if (req == 2) releaseVoice();

    const float fs = (float) sr;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float noiseGain = 0.5f + a * 0.35f;
    const float noiseLo = 260.0f, noiseHi = 1600.0f + a * 9400.0f;
    const float lpLo = 500.0f, lpHi = 1800.0f + a * 6000.0f;
    const float octaves = 1.5f + a * 2.5f;
    const int size = delayMask + 1;

    auto readDelay = [&] (const std::vector<float>& buf, float d)
    {
        float p = (float) delayWrite - d;
        while (p < 0.0f) p += (float) size;
        const int i0 = (int) p;
        const float fr = p - (float) i0;
        return buf[(size_t) (i0 & delayMask)] * (1.0f - fr) + buf[(size_t) ((i0 + 1) & delayMask)] * fr;
    };

    auto midiIt = midi.begin();

    for (int i = 0; i < numSamples; ++i)
    {
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i)
        {
            const auto m = (*midiIt).getMessage();
            if (m.isNoteOn())
                triggerVoice (m.getNoteNumber());
            else if (m.isNoteOff() && loop && m.getNoteNumber() == voice.note)
                releaseVoice();
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                releaseVoice();
            ++midiIt;
        }

        float vl = 0.0f, vr = 0.0f;

        if (voice.active)
        {
            const float f = (float) juce::jlimit (0.0, 1.0, voice.pos / voice.len);
            const float fp = rev ? f : 1.0f - f;
            lastFp = fp;
            const float env = std::pow (fp, 1.7f);
            const float declick = juce::jmin (1.0f, (float) (voice.pos / (0.006 * sr)))
                                * juce::jmin (1.0f, (float) ((voice.len - voice.pos) / (0.02 * sr)));

            if (voice.releasing)
            {
                voice.rel -= 1.0f / (0.04f * fs);
                if (voice.rel <= 0.0f) { voice.rel = 0.0f; voice.active = false; }
            }
            const float gain = env * declick * (voice.releasing ? voice.rel : 1.0f);

            if ((i & 15) == 0)
            {
                noiseBP.setCutoffFrequency (noiseLo * std::pow (noiseHi / noiseLo, fp));
                noiseBP.setResonance (0.9f + a * 1.4f);
                oscLP.setCutoffFrequency (lpLo * std::pow (lpHi / lpLo, fp));
            }

            const float nl = rng.nextFloat() * 2.0f - 1.0f;
            const float nr = rng.nextFloat() * 2.0f - 1.0f;
            const float bl = noiseBP.processSample (0, nl);
            const float br = noiseBP.processSample (1, nr);

            const float f1 = 110.0f * voice.noteRatio * std::pow (2.0f, octaves * fp);
            const float dt1 = juce::jmin (0.45f, f1 / fs);
            const float dt2 = juce::jmin (0.45f, dt1 * 1.0052f);
            phase1 += dt1; if (phase1 >= 1.0f) phase1 -= 1.0f;
            phase2 += dt2; if (phase2 >= 1.0f) phase2 -= 1.0f;
            const float saws = sawBlep (phase1, dt1) + sawBlep (phase2, dt2);
            const float o = oscLP.processSample (0, saws);

            vl = (bl * noiseGain + o * 0.16f) * gain;
            vr = (br * noiseGain + o * 0.16f) * gain;

            voice.pos += 1.0;
            if (voice.pos >= voice.len)
            {
                if (loop && ! voice.releasing) voice.pos -= voice.len;
                else voice.active = false;
            }
        }

        // phone
        const float pm = phoneMix.getNextValue();
        if (pm > 0.0001f || phoneMix.isSmoothing())
        {
            const float pl = phoneLP.processSample (0, phoneHP.processSample (0, vl)) * 1.5f;
            const float pr = phoneLP.processSample (1, phoneHP.processSample (1, vr)) * 1.5f;
            vl = vl * (1.0f - pm) + pl * pm;
            vr = vr * (1.0f - pm) + pr * pm;
        }

        // chorus
        const float cm = chorusMix.getNextValue();
        delayL[(size_t) delayWrite] = vl;
        delayR[(size_t) delayWrite] = vr;
        chorusPhase1 += 0.6f / fs;  if (chorusPhase1 >= 1.0f) chorusPhase1 -= 1.0f;
        chorusPhase2 += 0.83f / fs; if (chorusPhase2 >= 1.0f) chorusPhase2 -= 1.0f;
        if (cm > 0.0001f || chorusMix.isSmoothing())
        {
            const float d1 = 0.018f * fs, d2 = 0.027f * fs;
            const float m1 = 0.0035f * fs, m2 = 0.005f * fs;
            const float l1 = readDelay (delayL, d1 + m1 * std::sin (twoPi * chorusPhase1))
                           + readDelay (delayL, d2 + m2 * std::sin (twoPi * chorusPhase2));
            const float r1 = readDelay (delayR, d1 + m1 * std::sin (twoPi * (chorusPhase1 + 0.5f)))
                           + readDelay (delayR, d2 + m2 * std::sin (twoPi * (chorusPhase2 + 0.5f)));
            vl = vl * (1.0f - 0.2f * cm) + 0.4f * cm * l1;
            vr = vr * (1.0f - 0.2f * cm) + 0.4f * cm * r1;
        }
        delayWrite = (delayWrite + 1) & delayMask;

        // tremolo, speeds up through the riser
        const float tm = tremMix.getNextValue();
        const float rate = 3.0f * std::pow (16.0f / 3.0f, lastFp);
        tremPhase += rate / fs; if (tremPhase >= 1.0f) tremPhase -= 1.0f;
        const float depth = 0.7f * tm;
        const float tg = 1.0f - depth * 0.5f + depth * 0.5f * std::sin (twoPi * tremPhase);
        vl *= tg; vr *= tg;

        const float oL = std::tanh (vl * outGain * 1.4f);
        const float oR = std::tanh (vr * outGain * 1.4f);
        outL[i] = oL;
        if (outR != nullptr) outR[i] = oR;
    }

    voiceActiveFlag.store (voice.active);
    progress.store (voice.active ? (float) (voice.pos / voice.len) : -1.0f);
}

//==============================================================================
juce::AudioProcessorEditor* PoasRiserProcessor::createEditor()
{
    return new PoasRiserEditor (*this);
}

int PoasRiserProcessor::getNumPrograms() { return numFactory + (int) userPresets.size(); }

const juce::String PoasRiserProcessor::getProgramName (int index)
{
    if (index >= 0 && index < numFactory) return factoryPresets[index].name;
    const int u = index - numFactory;
    if (u >= 0 && u < (int) userPresets.size()) return userPresets[(size_t) u].name;
    return {};
}

void PoasRiserProcessor::applyPreset (const RiserPreset& p)
{
    auto set = [this] (const char* id, float plain)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
    };
    set ("amount", p.amount);
    set ("bars", (float) p.bars);
    set ("mode", (float) p.mode);
    set ("phone", p.phone ? 1.0f : 0.0f);
    set ("chorus", p.chorus ? 1.0f : 0.0f);
    set ("tremolo", p.tremolo ? 1.0f : 0.0f);
    set ("edit", p.edit ? 1.0f : 0.0f);
}

void PoasRiserProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;
    if (index < numFactory) applyPreset (factoryPresets[index]);
    else applyPreset (userPresets[(size_t) (index - numFactory)]);
}

void PoasRiserProcessor::saveUserPreset()
{
    RiserPreset p;
    p.name = "User " + juce::String ((int) userPresets.size() + 1);
    p.amount = amountP->load();
    p.bars = (int) barsP->load();
    p.mode = (int) modeP->load();
    p.phone = phoneP->load() > 0.5f;
    p.chorus = chorusP->load() > 0.5f;
    p.tremolo = tremoloP->load() > 0.5f;
    p.edit = editP->load() > 0.5f;
    userPresets.push_back (p);
    currentProgram = numFactory + (int) userPresets.size() - 1;
}

void PoasRiserProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr) return;

    auto* presets = xml->createNewChildElement ("UserPresets");
    for (auto& p : userPresets)
    {
        auto* e = presets->createNewChildElement ("P");
        e->setAttribute ("name", p.name);
        e->setAttribute ("amount", (double) p.amount);
        e->setAttribute ("bars", p.bars);
        e->setAttribute ("mode", p.mode);
        e->setAttribute ("phone", p.phone);
        e->setAttribute ("chorus", p.chorus);
        e->setAttribute ("tremolo", p.tremolo);
        e->setAttribute ("edit", p.edit);
    }
    xml->setAttribute ("program", currentProgram);
    copyXmlToBinary (*xml, destData);
}

void PoasRiserProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr) return;

    userPresets.clear();
    if (auto* presets = xml->getChildByName ("UserPresets"))
    {
        for (auto* e : presets->getChildIterator())
        {
            RiserPreset p;
            p.name = e->getStringAttribute ("name", "User");
            p.amount = (float) e->getDoubleAttribute ("amount", 50.0);
            p.bars = e->getIntAttribute ("bars", 1);
            p.mode = e->getIntAttribute ("mode", 0);
            p.phone = e->getBoolAttribute ("phone", false);
            p.chorus = e->getBoolAttribute ("chorus", true);
            p.tremolo = e->getBoolAttribute ("tremolo", true);
            p.edit = e->getBoolAttribute ("edit", true);
            userPresets.push_back (p);
        }
        xml->removeChildElement (presets, true);
    }
    currentProgram = juce::jlimit (0, juce::jmax (0, getNumPrograms() - 1), xml->getIntAttribute ("program", 0));

    if (xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PoasRiserProcessor();
}
