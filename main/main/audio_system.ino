#include "audio_system.h"

bool audioSystemActive = false;

void audioSystemBegin() {
  audioSystemActive = false;
  pinMode(SYS_AUDIO_PIN, INPUT_PULLDOWN);
}

void audioSystemEnable() {
  if (audioSystemActive) return;

  pinMode(SYS_AUDIO_PIN, OUTPUT);
  ledcAttach(SYS_AUDIO_PIN, 40000, SYS_AUDIO_RES);

  ledcWrite(SYS_AUDIO_PIN, 128);
  delay(8);

  audioSystemActive = true;
}

void audioSystemWrite(uint8_t sample) {
  if (!audioSystemActive) {
    audioSystemEnable();
  }

  ledcWrite(SYS_AUDIO_PIN, sample);
}

// =====================================================
// TON SIMPLE POUR INSTRUMENT / SAMPLER / EFFETS
// =====================================================

void audioSystemTone(int freq, int volume, int duration) {
  volume = constrain(volume, 0, 180);

  pinMode(SYS_AUDIO_PIN, OUTPUT);

  ledcAttach(SYS_AUDIO_PIN, freq, SYS_AUDIO_RES);
  ledcWriteTone(SYS_AUDIO_PIN, freq);
  ledcWrite(SYS_AUDIO_PIN, volume);

  delay(duration);

  ledcWriteTone(SYS_AUDIO_PIN, 0);
  ledcWrite(SYS_AUDIO_PIN, 0);

  ledcDetach(SYS_AUDIO_PIN);
  pinMode(SYS_AUDIO_PIN, INPUT_PULLDOWN);

  audioSystemActive = false;
}

void audioSystemMutePin() {
  if (audioSystemActive) {
    for (int i = 0; i < 12; i++) {
      ledcWrite(SYS_AUDIO_PIN, 128);
      delay(1);
    }

    ledcWrite(SYS_AUDIO_PIN, 0);
    delay(5);

    ledcDetach(SYS_AUDIO_PIN);
  }

  pinMode(SYS_AUDIO_PIN, INPUT_PULLDOWN);
  audioSystemActive = false;
}

void audioSystemStop() {
  audioSystemMutePin();
}