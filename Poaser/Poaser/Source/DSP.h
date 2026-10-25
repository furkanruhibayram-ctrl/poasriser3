#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace poaser
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

//==============================================================================
// Snapshot of every user parameter (plain values, block-rate).
struct Params
{
    float predelayMs = 0.f;
    float lowcut     = 20.f;
    float highcut    = 20000.f;
    float size       = 0.5f;
    float diffusion  = 0.8f;
    float decay      = 2.f;
    float damping    = 0.4f;
    float modSpeed   = 0.5f;
    float modDepth   = 0.2f;
    float width      = 1.f;
    float dry        = 1.f;
    float wet        = 0.35f;
    float pitch      = 0.f;   // semitones
    float fine       = 0.f;   // cents
    float pitchMix   = 0.f;
    int   pitchMode  = 0;     // 0 = pre reverb, 1 = post reverb, 2 = shimmer
    float shimmer    = 0.f;
    float grainMs    = 80.f;
    float revMix     = 0.f;
    float riseMs     = 1000.f;
    float outDb      = 0.f;
};

//==============================================================================
inline int nextPow2 (int v)
{
    int p = 1;
    while (p < v) p <<= 1;
    return p;
}

// Circular delay line, read (0) = newest sample.
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        size = nextPow2 (maxSamples + 8);
        mask = size - 1;
        buf.assign ((size_t) size, 0.f);
        w = 0;
    }

    void clear() { std::fill (buf.begin(), buf.end(), 0.f); }

    inline void push (float x)
    {
        w = (w + 1) & mask;
        buf[(size_t) w] = x;
    }

    inline float read (float d) const
    {
        d = std::min (std::max (d, 0.f), (float) (size - 4));
        const int di  = (int) d;
        const float f = d - (float) di;
        const float a = buf[(size_t) ((w - di) & mask)];
        const float b = buf[(size_t) ((w - di - 1) & mask)];
        return a + (b - a) * f;
    }

private:
    std::vector<float> buf;
    int size = 0, mask = 0, w = 0;
};

//==============================================================================
struct OnePole
{
    float z = 0.f, a = 0.f;

    void setCutoff (float fc, float sr)
    {
        fc = std::min (std::max (fc, 5.f), sr * 0.45f);
        a = 1.f - std::exp (-kTwoPi * fc / sr);
    }
    inline float lp (float x) { z += a * (x - z); return z; }
    inline float hp (float x) { return x - lp (x); }
    void reset() { z = 0.f; }
};

//==============================================================================
// Classic two-tap crossfading delay-line pitch shifter.
class PitchShifter
{
public:
    void prepare (float sr)
    {
        dl.prepare ((int) (0.5f * sr));
        phase = 0.f;
    }

    void reset() { dl.clear(); phase = 0.f; }

    inline float process (float x, float ratio, float grainSamples)
    {
        dl.push (x);
        float out = 0.f;
        for (int i = 0; i < 2; ++i)
        {
            float ph = phase + 0.5f * (float) i;
            if (ph >= 1.f) ph -= 1.f;
            const float d = ph * grainSamples + 2.f;
            const float w = 0.5f - 0.5f * std::cos (kTwoPi * ph);
            out += w * dl.read (d);
        }
        phase += (1.f - ratio) / grainSamples;
        phase -= std::floor (phase);
        return out;
    }

private:
    DelayLine dl;
    float phase = 0.f;
};

//==============================================================================
// Real-time "reverse" chunk player: plays the recent past backwards with two
// overlapping Hann-windowed voices.
class Reverser
{
public:
    void prepare (float sr, float maxMs)
    {
        dl.prepare ((int) (2.f * maxMs * 0.001f * sr) + 16);
        phase = 0.f;
    }

    void reset() { dl.clear(); phase = 0.f; }

    inline float process (float x, float lenSamples)
    {
        dl.push (x);
        float out = 0.f;
        for (int i = 0; i < 2; ++i)
        {
            float ph = phase + 0.5f * (float) i;
            if (ph >= 1.f) ph -= 1.f;
            const float d = 2.f * ph * lenSamples + 1.f;
            const float w = 0.5f - 0.5f * std::cos (kTwoPi * ph);
            out += w * dl.read (d);
        }
        phase += 1.f / lenSamples;
        phase -= std::floor (phase);
        return out;
    }

private:
    DelayLine dl;
    float phase = 0.f;
};

//==============================================================================
struct Allpass
{
    DelayLine dl;
    int len = 1;
    float g = 0.5f;

    void prepare (int length) { len = std::max (1, length); dl.prepare (len + 4); }
    void clear() { dl.clear(); }

    inline float process (float x)
    {
        const float delayed = dl.read ((float) (len - 1));
        const float v = x - g * delayed;
        dl.push (v);
        return g * v + delayed;
    }
};

//==============================================================================
// 8-line feedback delay network reverb (Hadamard mixing, damping, modulation)
// with allpass input diffusion.
class FDNReverb
{
public:
    static constexpr int N = 8;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        const int maxLen = (int) (0.5f * sr);
        for (int i = 0; i < N; ++i)
        {
            line[i].prepare (maxLen);
            lfoPhase[i] = kTwoPi * (float) i / (float) N;
            damp[i] = 0.f;
            gain[i] = 0.f;
            gTarget[i] = 0.f;
            target[i] = lenSm[i] = baseMs[i] * 0.001f * sr;
        }
        static const float apL[4] = { 4.7f, 6.9f, 10.3f, 14.1f };
        static const float apR[4] = { 5.3f, 7.7f, 11.1f, 15.3f };
        for (int k = 0; k < 4; ++k)
        {
            ap[0][k].prepare ((int) (apL[k] * 0.001f * sr));
            ap[1][k].prepare ((int) (apR[k] * 0.001f * sr));
        }
    }

    void reset()
    {
        for (int i = 0; i < N; ++i) { line[i].clear(); damp[i] = 0.f; }
        for (auto& c : ap) for (auto& a : c) a.clear();
    }

    void setParams (float size, float decaySec, float damping, float diffusion,
                    float modSpeed, float modDepth, bool snap)
    {
        const float sizeScale = 0.4f + 2.2f * size;
        decaySec = std::max (decaySec, 0.05f);

        float gSum = 0.f;
        for (int i = 0; i < N; ++i)
        {
            target[i] = baseMs[i] * 0.001f * sr * sizeScale;
            gTarget[i] = std::pow (10.f, -3.f * (target[i] / sr) / decaySec);
            gSum += gTarget[i];
            if (snap) { lenSm[i] = target[i]; gain[i] = gTarget[i]; }
        }
        const float gAvg = gSum / (float) N;
        inGain = 0.5f * std::min (1.f, std::max (0.1f, 1.8f * std::sqrt (std::max (0.f, 1.f - gAvg * gAvg))));

        const float fc = 20000.f * std::pow (0.075f, damping);
        dampA = std::min (1.f, 1.f - std::exp (-kTwoPi * fc / sr));

        const float apG = 0.15f + 0.6f * diffusion;
        for (auto& c : ap) for (auto& a : c) a.g = apG;

        lfoInc = kTwoPi * modSpeed / sr;
        modSamples = modDepth * 0.0015f * sr;
    }

    inline void process (float inL, float inR, float& outL, float& outR)
    {
        float l = inL, r = inR;
        for (int k = 0; k < 4; ++k)
        {
            l = ap[0][k].process (l);
            r = ap[1][k].process (r);
        }

        float v[N];
        float tap[N];
        for (int i = 0; i < N; ++i)
        {
            lenSm[i] += (target[i] - lenSm[i]) * 0.0008f;
            gain[i]  += (gTarget[i] - gain[i]) * 0.0008f;

            lfoPhase[i] += lfoInc;
            if (lfoPhase[i] > kTwoPi) lfoPhase[i] -= kTwoPi;

            const float d = lenSm[i] + std::sin (lfoPhase[i]) * modSamples;
            float s = line[i].read (std::max (d - 1.f, 0.f));
            damp[i] += dampA * (s - damp[i]);
            s = damp[i];
            tap[i] = s;
            v[i] = s * gain[i];
        }

        hadamard (v);

        float oL = 0.f, oR = 0.f;
        for (int i = 0; i < N; ++i)
        {
            const float in = (i & 1) ? r : l;
            line[i].push (v[i] + in * inGain);

            const float sgn = ((i >> 1) & 1) ? -1.f : 1.f;
            if (i & 1) oR += sgn * tap[i];
            else       oL += sgn * tap[i];
        }
        outL = oL * 0.5f;
        outR = oR * 0.5f;
    }

private:
    static inline void hadamard (float* a)
    {
        for (int h = 1; h < N; h <<= 1)
            for (int i = 0; i < N; i += h << 1)
                for (int j = i; j < i + h; ++j)
                {
                    const float x = a[j], y = a[j + h];
                    a[j] = x + y;
                    a[j + h] = x - y;
                }
        for (int i = 0; i < N; ++i) a[i] *= 0.35355339f;
    }

    static constexpr float baseMs[N] = { 31.3f, 37.1f, 41.9f, 47.9f, 53.9f, 61.3f, 67.1f, 73.3f };

    float sr = 44100.f;
    DelayLine line[N];
    Allpass ap[2][4];
    float lenSm[N] {}, target[N] {}, gain[N] {}, gTarget[N] {}, damp[N] {}, lfoPhase[N] {};
    float dampA = 1.f, lfoInc = 0.f, modSamples = 0.f, inGain = 0.5f;
};

//==============================================================================
// The whole effect chain (used by the real-time processor and by the offline
// riser renderer).
class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        smCoef = 1.f - std::exp (-1.f / (0.02f * sr));
        for (int c = 0; c < 2; ++c)
        {
            pd[c].prepare ((int) (0.6f * sr));
            shiftPre[c].prepare (sr);
            shiftPost[c].prepare (sr);
            rev[c].prepare (sr, 4000.f);
        }
        reverb.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            pd[c].clear(); shiftPre[c].reset(); shiftPost[c].reset(); rev[c].reset();
            fb[c] = 0.f; fbLp[c].reset();
            for (auto& f : hpF[c]) f.reset();
            for (auto& f : lpF[c]) f.reset();
        }
        reverb.reset();
        needSnap = true;
    }

    void setParams (const Params& p, bool snap = false)
    {
        par = p;
        tDry = p.dry;
        tWet = p.wet;
        tPm  = p.pitchMix;
        tRev = p.revMix;
        tOut = std::pow (10.f, p.outDb / 20.f);
        tRatio = std::pow (2.f, (p.pitch + p.fine * 0.01f) / 12.f);
        tPre = p.predelayMs * 0.001f * sr;
        grain = std::max (16.f, p.grainMs * 0.001f * sr);
        rise  = std::max (64.f, p.riseMs * 0.001f * sr);
        shim  = p.shimmer * 0.85f;

        const bool doSnap = snap || needSnap;
        reverb.setParams (p.size, p.decay, p.damping, p.diffusion, p.modSpeed, p.modDepth, doSnap);

        for (int c = 0; c < 2; ++c)
        {
            for (auto& f : hpF[c]) f.setCutoff (p.lowcut, sr);
            for (auto& f : lpF[c]) f.setCutoff (p.highcut, sr);
            fbLp[c].setCutoff (6500.f, sr);
        }

        if (doSnap)
        {
            sDry = tDry; sWet = tWet; sPm = tPm; sRev = tRev;
            sOut = tOut; sRatio = tRatio; sPre = tPre;
            needSnap = false;
        }
    }

    inline void process (float inL, float inR, float& outL, float& outR)
    {
        sDry   += (tDry - sDry) * smCoef;
        sWet   += (tWet - sWet) * smCoef;
        sPm    += (tPm - sPm) * smCoef;
        sRev   += (tRev - sRev) * smCoef;
        sOut   += (tOut - sOut) * smCoef;
        sRatio += (tRatio - sRatio) * smCoef;
        sPre   += (tPre - sPre) * smCoef;

        const float x[2] = { inL, inR };
        float w[2];
        float rin[2];

        for (int c = 0; c < 2; ++c)
        {
            float s = x[c];
            if (par.pitchMode == 0)
            {
                const float ps = shiftPre[c].process (s, sRatio, grain);
                s += (ps - s) * sPm;
            }
            pd[c].push (s);
            rin[c] = pd[c].read (sPre) + fb[c];
        }

        reverb.process (rin[0], rin[1], w[0], w[1]);

        for (int c = 0; c < 2; ++c)
        {
            float s = w[c];
            if (par.pitchMode >= 1)
            {
                const float ps = shiftPost[c].process (s, sRatio, grain);
                if (par.pitchMode == 2)
                    fb[c] = std::tanh (fbLp[c].lp (ps) * shim);
                else
                    fb[c] = 0.f;
                s += (ps - s) * sPm;
            }
            else
                fb[c] = 0.f;

            const float rv = rev[c].process (s, rise);
            s += (rv - s) * sRev;

            s = hpF[c][1].hp (hpF[c][0].hp (s));
            s = lpF[c][1].lp (lpF[c][0].lp (s));
            w[c] = s;
        }

        const float m = 0.5f * (w[0] + w[1]);
        const float sd = 0.5f * (w[0] - w[1]) * par.width;
        const float wl = m + sd, wr = m - sd;

        outL = (inL * sDry + wl * sWet) * sOut;
        outR = (inR * sDry + wr * sWet) * sOut;
    }

private:
    float sr = 44100.f, smCoef = 0.001f;
    Params par;

    DelayLine pd[2];
    PitchShifter shiftPre[2], shiftPost[2];
    Reverser rev[2];
    FDNReverb reverb;
    OnePole hpF[2][2], lpF[2][2], fbLp[2];
    float fb[2] { 0.f, 0.f };

    float tDry = 1.f, tWet = 0.3f, tPm = 0.f, tRev = 0.f, tOut = 1.f, tRatio = 1.f, tPre = 0.f;
    float sDry = 1.f, sWet = 0.3f, sPm = 0.f, sRev = 0.f, sOut = 1.f, sRatio = 1.f, sPre = 0.f;
    float grain = 3000.f, rise = 48000.f, shim = 0.f;
    bool needSnap = true;
};

} // namespace poaser
