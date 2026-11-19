#ifndef AUDIO_WAV_PLAYER_H
#define AUDIO_WAV_PLAYER_H

#include <Arduino.h>
#include <SD.h>

// 1024 consomme trop de RAM avec Disc + Video + Loader + Compo.
// 256 suffit pour ton lecteur WAV simple.
#define WAV_BUFFER_SIZE 256

struct WavPlayer {
  File file;

  bool playing;
  bool valid;

  uint32_t dataStart;
  uint32_t dataSize;
  uint32_t bytesRead;

  uint16_t channels;
  uint16_t bits;
  uint32_t sampleRate;

  unsigned long lastMicros;
  unsigned long interval;

  uint8_t buffer[WAV_BUFFER_SIZE];
  int bufferLen;
  int bufferPos;

  int volume;
};

void wavPlayerInit(WavPlayer &wp);
bool wavPlayerOpen(WavPlayer &wp, String path, int volume);
void wavPlayerStop(WavPlayer &wp);
void wavPlayerUpdate(WavPlayer &wp);
bool wavPlayerIsPlaying(WavPlayer &wp);
void wavPlayerSetVolume(WavPlayer &wp, int volume);

#endif