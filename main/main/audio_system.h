#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <Arduino.h>

#define SYS_AUDIO_PIN 26
#define SYS_AUDIO_RES 8

void audioSystemBegin();
void audioSystemEnable();
void audioSystemWrite(uint8_t sample);
void audioSystemTone(int freq, int volume, int duration);
void audioSystemStop();
void audioSystemMutePin();

#endif