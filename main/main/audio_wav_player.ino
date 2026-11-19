#include "audio_wav_player.h"
#include "audio_system.h"

void wavPlayerInit(WavPlayer &wp) {
  wp.playing = false;
  wp.valid = false;

  wp.dataStart = 44;
  wp.dataSize = 0;
  wp.bytesRead = 0;

  wp.channels = 1;
  wp.bits = 16;
  wp.sampleRate = 22050;

  wp.lastMicros = 0;
  wp.interval = 45;

  wp.bufferLen = 0;
  wp.bufferPos = 0;

  wp.volume = 80;
}

uint16_t wavPlayerRead16(File &f) {
  uint16_t v = f.read();
  v |= f.read() << 8;
  return v;
}

uint32_t wavPlayerRead32(File &f) {
  uint32_t v = f.read();
  v |= f.read() << 8;
  v |= f.read() << 16;
  v |= f.read() << 24;
  return v;
}

bool wavPlayerReadHeader(WavPlayer &wp) {
  char id[5];
  id[4] = 0;

  wp.file.read((uint8_t*)id, 4);
  if (String(id) != "RIFF") return false;

  wavPlayerRead32(wp.file);

  wp.file.read((uint8_t*)id, 4);
  if (String(id) != "WAVE") return false;

  bool foundFmt = false;
  bool foundData = false;

  while (wp.file.available()) {
    wp.file.read((uint8_t*)id, 4);
    uint32_t chunkSize = wavPlayerRead32(wp.file);

    String chunk = String(id);

    if (chunk == "fmt ") {
      uint16_t audioFormat = wavPlayerRead16(wp.file);

      wp.channels = wavPlayerRead16(wp.file);
      wp.sampleRate = wavPlayerRead32(wp.file);

      wavPlayerRead32(wp.file);
      wavPlayerRead16(wp.file);

      wp.bits = wavPlayerRead16(wp.file);

      if (chunkSize > 16) {
        wp.file.seek(wp.file.position() + (chunkSize - 16));
      }

      if (audioFormat != 1) return false;

      foundFmt = true;
    }

    else if (chunk == "data") {
      wp.dataStart = wp.file.position();
      wp.dataSize = chunkSize;
      foundData = true;
      break;
    }

    else {
      wp.file.seek(wp.file.position() + chunkSize);
    }
  }

  if (!foundFmt || !foundData) return false;

  if (wp.channels != 1) {
    Serial.println("WAV refuse : il faut MONO");
    return false;
  }

  if (wp.bits != 16) {
    Serial.println("WAV refuse : il faut 16 bits PCM");
    return false;
  }

  wp.interval = 1000000UL / wp.sampleRate;

  return true;
}

bool wavPlayerFillBuffer(WavPlayer &wp) {
  if (!wp.file) return false;
  if (wp.bytesRead >= wp.dataSize) return false;

  uint32_t remaining = wp.dataSize - wp.bytesRead;

  int toRead = WAV_BUFFER_SIZE;
  if (remaining < WAV_BUFFER_SIZE) {
    toRead = remaining;
  }

  wp.bufferLen = wp.file.read(wp.buffer, toRead);
  wp.bufferPos = 0;

  if (wp.bufferLen <= 0) return false;

  wp.bytesRead += wp.bufferLen;
  return true;
}

bool wavPlayerOpen(WavPlayer &wp, String path, int volume) {
  wavPlayerStop(wp);
  wavPlayerInit(wp);

  wp.volume = constrain(volume, 0, 100);

  if (!SD.exists(path)) {
    Serial.print("WAV introuvable : ");
    Serial.println(path);
    return false;
  }

  wp.file = SD.open(path);

  if (!wp.file) {
    Serial.print("Impossible ouvrir WAV : ");
    Serial.println(path);
    return false;
  }

  if (!wavPlayerReadHeader(wp)) {
    Serial.println("Header WAV invalide");
    wp.file.close();
    return false;
  }

  wp.file.seek(wp.dataStart);

  wp.bytesRead = 0;
  wp.bufferLen = 0;
  wp.bufferPos = 0;

  if (!wavPlayerFillBuffer(wp)) {
    Serial.println("Buffer WAV vide");
    wp.file.close();
    return false;
  }

  audioSystemEnable();

  wp.lastMicros = micros();
  wp.playing = true;
  wp.valid = true;

  Serial.print("Lecture WAV OK : ");
  Serial.println(path);

  return true;
}

void wavPlayerStop(WavPlayer &wp) {
  wp.playing = false;
  wp.valid = false;

  if (wp.file) {
    wp.file.close();
  }

  wp.bufferLen = 0;
  wp.bufferPos = 0;
  wp.bytesRead = 0;

  audioSystemStop();
}

void wavPlayerUpdate(WavPlayer &wp) {
  if (!wp.playing || !wp.valid) return;

  int guard = 0;

  while (
    wp.playing &&
    micros() - wp.lastMicros >= wp.interval &&
    guard < 220
  ) {
    wp.lastMicros += wp.interval;

    if (wp.bufferPos + 1 >= wp.bufferLen) {
      if (!wavPlayerFillBuffer(wp)) {
        wavPlayerStop(wp);
        return;
      }
    }

    uint8_t lo = wp.buffer[wp.bufferPos++];
    uint8_t hi = wp.buffer[wp.bufferPos++];

    int16_t s = (int16_t)((hi << 8) | lo);

    int sample = (s >> 8) + 128;
    sample = constrain(sample, 0, 255);

    int centered = sample - 128;
    int scaled = 128 + ((centered * wp.volume) / 100);
    scaled = constrain(scaled, 0, 255);

    audioSystemWrite((uint8_t)scaled);

    guard++;
  }
}

bool wavPlayerIsPlaying(WavPlayer &wp) {
  return wp.playing;
}

void wavPlayerSetVolume(WavPlayer &wp, int volume) {
  wp.volume = constrain(volume, 0, 100);
}