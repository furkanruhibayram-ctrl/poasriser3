#pragma once

#include <vector>

namespace poaser
{
constexpr int kNumParams = 21;

// Parameter order used everywhere (presets, ids):
//  0 predelay  1 lowcut  2 highcut  3 size  4 diffusion  5 decay  6 damping
//  7 modspeed  8 moddepth  9 width  10 dry  11 wet  12 pitch  13 fine
//  14 pitchmix  15 pitchmode  16 shimmer  17 grain  18 revmix  19 rise  20 out
extern const char* const kParamIds[kNumParams];

struct Preset
{
    const char* category;
    const char* name;
    float v[kNumParams];
};

const std::vector<Preset>& getPresets();

} // namespace poaser
