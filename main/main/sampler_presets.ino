#include "sampler.h"
#include <math.h>

SmpPreset smpPresets[SMP_PRESET_COUNT] = {
  {"SINE",   0},
  {"SQUARE", 1},
  {"SAW",    2},
  {"TRI",    3},
  {"NOISE",  4},
  {"BASS",   5},
  {"LASER",  6},
  {"BELL",   7}
};

void smpApplyPreset(int presetIndex, uint8_t curve[SMP_CURVE_N]) {
  presetIndex = constrain(presetIndex, 0, SMP_PRESET_COUNT - 1);

  int type = smpPresets[presetIndex].type;

  for (int i = 0; i < SMP_CURVE_N; i++) {
    float p = (float)i / (float)SMP_CURVE_N;
    float a = p * 6.28318f;

    int v = 128;

    if (type == 0) {
      // SINE
      v = 128 + sin(a) * 95;
    }

    else if (type == 1) {
      // SQUARE
      v = (i < SMP_CURVE_N / 2) ? 225 : 30;
    }

    else if (type == 2) {
      // SAW
      v = map(i, 0, SMP_CURVE_N - 1, 30, 225);
    }

    else if (type == 3) {
      // TRIANGLE
      if (i < SMP_CURVE_N / 2) {
        v = map(i, 0, SMP_CURVE_N / 2, 30, 225);
      } else {
        v = map(i, SMP_CURVE_N / 2, SMP_CURVE_N - 1, 225, 30);
      }
    }

    else if (type == 4) {
      // NOISE
      v = random(20, 236);
    }

    else if (type == 5) {
      // BASS : sinus écrasé
      float s = sin(a);
      if (s > 0) s = pow(s, 0.55f);
      else s = -pow(-s, 0.55f);
      v = 128 + s * 105;
    }

    else if (type == 6) {
      // LASER : pente brutale
      if (i < SMP_CURVE_N / 4) {
        v = map(i, 0, SMP_CURVE_N / 4, 230, 40);
      } else {
        v = 128 + sin(a * 3.0f) * 45;
      }
    }

    else if (type == 7) {
      // BELL : sinus + harmonique
      float s = sin(a) * 0.75f + sin(a * 2.0f) * 0.25f;
      v = 128 + s * 95;
    }

    curve[i] = constrain(v, 0, 255);
  }
}