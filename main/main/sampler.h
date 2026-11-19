#ifndef SAMPLER_H
#define SAMPLER_H

#include <Arduino.h>

#define SMP_CURVE_N 64
#define SMP_SR 22050

// =====================================================
// PRESETS DE COURBES
// =====================================================

#define SMP_PRESET_COUNT 8

struct SmpPreset {
  const char* name;
  int type;
};

extern SmpPreset smpPresets[SMP_PRESET_COUNT];

void smpApplyPreset(int presetIndex, uint8_t curve[SMP_CURVE_N]);

// =====================================================
// APP SAMPLER
// =====================================================

void launchSampler();
void updateSampler();

#endif