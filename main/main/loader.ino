#include "loader.h"
#include "menu_core.h"
#include "audio_system.h"
#include "audio_wav_player.h"

#define LOADER_IMG "/systeme/cydimg.jpg"
#define LOADER_WAV "/systeme/startup.wav"

WavPlayer loaderWav;

void loaderProgress(int percent) {
  percent = constrain(percent, 0, 100);

  int barW = map(percent, 0, 100, 0, 300);

  tft.fillRect(10, 228, 300, 4, TFT_DARKGREY);
  tft.fillRect(10, 228, barW, 4, TFT_YELLOW);
}

void playStartupSound() {
  wavPlayerInit(loaderWav);

  if (!SD.exists(LOADER_WAV)) {
    Serial.println("startup.wav introuvable");
    return;
  }

  if (!wavPlayerOpen(loaderWav, LOADER_WAV, 90)) {
    Serial.println("startup.wav invalide");
    return;
  }

  loaderProgress(20);

  while (wavPlayerIsPlaying(loaderWav)) {
    wavPlayerUpdate(loaderWav);
    delayMicroseconds(20);
  }

  wavPlayerStop(loaderWav);
  loaderProgress(35);
}

void showLoaderStart() {
  tft.fillScreen(TFT_BLACK);

  if (startSD()) {
    if (SD.exists(LOADER_IMG)) {
      TJpgDec.drawSdJpg(0, 0, LOADER_IMG);
    } else {
      drawBoldText("CYD2USB", 160, 100, TFT_WHITE, TFT_BLACK, MC_DATUM, 4);
    }

    loaderProgress(5);
    playStartupSound();
    loaderProgress(35);

    stopSDStartTouch();
  } else {
    drawBoldText("CYD2USB", 160, 100, TFT_WHITE, TFT_BLACK, MC_DATUM, 4);
    loaderProgress(30);
  }
}