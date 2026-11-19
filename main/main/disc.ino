#include "disc.h"
#include "menu_core.h"
#include "audio_system.h"
#include "audio_wav_player.h"

#define DISC_WAV_DIR "/wav"

#define DISC_SD_CS   5
#define DISC_SD_SCK  18
#define DISC_SD_MISO 19
#define DISC_SD_MOSI 23

SPIClass discSdSPI = SPIClass(HSPI);

#define DISC_MAX_FILES 30

String discNames[DISC_MAX_FILES];
String discPaths[DISC_MAX_FILES];

int discCount = 0;
int discIndex = 0;
int discVolume = 80;

bool discPlaying = false;

unsigned long discLastTouch = 0;

WavPlayer discWav;

// =====================================================
// SD
// =====================================================

bool discStartSD() {
  SD.end();
  delay(30);

  discSdSPI.end();
  delay(30);

  discSdSPI.begin(DISC_SD_SCK, DISC_SD_MISO, DISC_SD_MOSI, DISC_SD_CS);

  if (!SD.begin(DISC_SD_CS, discSdSPI, 10000000)) {
    Serial.println("DISC SD ECHEC");
    return false;
  }

  Serial.println("DISC SD OK");
  return true;
}

bool discIsWav(String name) {
  String n = name;
  n.toLowerCase();
  return n.endsWith(".wav");
}

void discLoadFiles() {
  discCount = 0;
  discIndex = 0;

  if (!discStartSD()) return;

  SD.mkdir(DISC_WAV_DIR);

  File dir = SD.open(DISC_WAV_DIR);

  if (!dir) {
    Serial.println("Dossier /wav introuvable");
    return;
  }

  while (discCount < DISC_MAX_FILES) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (!entry.isDirectory()) {
      String raw = String(entry.name());

      if (discIsWav(raw)) {
        String clean = raw;
        clean.replace("/wav/", "");
        clean.replace("wav/", "");

        discNames[discCount] = clean;
        discPaths[discCount] = String(DISC_WAV_DIR) + "/" + clean;

        Serial.print("WAV trouve : ");
        Serial.println(discPaths[discCount]);

        discCount++;
      }
    }

    entry.close();
  }

  dir.close();
}

// =====================================================
// LECTURE
// =====================================================

void discStop() {
  discPlaying = false;
  wavPlayerStop(discWav);
}

bool discPlayCurrent() {
  if (discCount <= 0) return false;

  discStop();

  String path = discPaths[discIndex];

  Serial.println("----------");
  Serial.print("Lecture WAV : ");
  Serial.println(path);

  if (!wavPlayerOpen(discWav, path, discVolume)) {
    Serial.println("Lecture WAV ECHEC");
    discPlaying = false;
    return false;
  }

  discPlaying = true;
  return true;
}

void discAudioUpdate() {
  if (!discPlaying) return;

  wavPlayerUpdate(discWav);

  if (!wavPlayerIsPlaying(discWav)) {
    discPlaying = false;

    discIndex++;
    if (discIndex >= discCount) discIndex = 0;

    discDrawUI();
  }
}

// =====================================================
// UI
// =====================================================

String discShortName(String name) {
  if (name.length() > 24) return name.substring(0, 24);
  return name;
}

void discHeader() {
  tft.fillRect(0, 0, 320, 34, TFT_WHITE);
  tft.fillTriangle(11, 17, 31, 6, 31, 28, TFT_BLACK);
  drawBoldText("Disc", 160, 17, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);
}

void discDrawButton(int x, int y, int w, int h, String txt) {
  tft.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
  tft.drawRoundRect(x, y, w, h, 8, TFT_DARKGREY);
  drawBoldText(txt, x + w / 2, y + h / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

void discDrawPlayInLabel() {
  tft.fillCircle(160, 110, 34, TFT_WHITE);
  tft.drawCircle(160, 110, 34, TFT_DARKGREY);

  if (!discPlaying) {
    tft.fillTriangle(150, 92, 150, 128, 178, 110, TFT_BLACK);
  } else {
    tft.fillRect(149, 92, 9, 36, TFT_BLACK);
    tft.fillRect(165, 92, 9, 36, TFT_BLACK);
  }
}

void discDrawUI() {
  tft.fillScreen(TFT_WHITE);
  discHeader();

  if (discCount <= 0) {
    drawBoldText("Aucun WAV", 160, 100, TFT_RED, TFT_WHITE, MC_DATUM, 2);
    drawBoldText("/wav mono 16bits", 160, 130, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
    return;
  }

  tft.fillCircle(160, 110, 72, TFT_BLACK);
  tft.drawCircle(160, 110, 72, TFT_DARKGREY);
  tft.drawCircle(160, 110, 55, TFT_DARKGREY);
  tft.drawCircle(160, 110, 38, TFT_DARKGREY);

  discDrawPlayInLabel();

  drawBoldText(discShortName(discNames[discIndex]), 160, 190, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);

  drawBoldText(
    "Piste " + String(discIndex + 1) + "/" + String(discCount),
    160,
    210,
    TFT_BLACK,
    TFT_WHITE,
    MC_DATUM,
    1
  );

  drawBoldText(
    "Vol " + String(discVolume),
    160,
    230,
    TFT_BLACK,
    TFT_WHITE,
    MC_DATUM,
    1
  );

  discDrawButton(18, 90, 48, 38, "<<");
  discDrawButton(254, 90, 48, 38, ">>");

  discDrawButton(72, 216, 45, 22, "V-");
  discDrawButton(203, 216, 45, 22, "V+");
}

// =====================================================
// ACTIONS
// =====================================================

void discTogglePlay() {
  if (discPlaying) {
    discStop();
  } else {
    bool ok = discPlayCurrent();

    if (!ok) {
      audioSystemStop();
      tft.fillRect(0, 160, 320, 24, TFT_WHITE);
      drawBoldText("WAV invalide", 160, 170, TFT_RED, TFT_WHITE, MC_DATUM, 2);
      return;
    }
  }

  discDrawUI();
}

void discNext() {
  discStop();

  discIndex++;
  if (discIndex >= discCount) discIndex = 0;

  discDrawUI();
}

void discPrev() {
  discStop();

  discIndex--;
  if (discIndex < 0) discIndex = discCount - 1;

  discDrawUI();
}

void discVolPlus() {
  discVolume += 10;
  if (discVolume > 100) discVolume = 100;

  wavPlayerSetVolume(discWav, discVolume);
  discDrawUI();
}

void discVolMinus() {
  discVolume -= 10;
  if (discVolume < 10) discVolume = 10;

  wavPlayerSetVolume(discWav, discVolume);
  discDrawUI();
}

void discExit() {
  discStop();

  SD.end();
  delay(30);

  stopSDStartTouch();
  drawSubMenu(currentMenu);
}

// =====================================================
// LANCEMENT
// =====================================================

void launchDisc() {
  appMode = MODE_DISC;

  stopSDStartTouch();
  audioSystemStop();

  wavPlayerInit(discWav);

  discVolume = 80;
  discPlaying = false;

  discLoadFiles();
  discDrawUI();

  Serial.println("DISC WAV PLAYER COMMUN PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateDisc() {
  discAudioUpdate();

  int x, y;

  if (!getTouch(x, y)) return;

  if (millis() - discLastTouch < 180) return;
  discLastTouch = millis();

  if (y < 36 && x < 45) {
    discExit();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  int dx = x - 160;
  int dy = y - 110;

  if (dx * dx + dy * dy <= 34 * 34) {
    discTogglePlay();
  }

  else if (x >= 18 && x <= 66 && y >= 90 && y <= 128) {
    discPrev();
  }

  else if (x >= 254 && x <= 302 && y >= 90 && y <= 128) {
    discNext();
  }

  else if (x >= 72 && x <= 117 && y >= 216 && y <= 238) {
    discVolMinus();
  }

  else if (x >= 203 && x <= 248 && y >= 216 && y <= 238) {
    discVolPlus();
  }

  while (ts.touched()) delay(5);
  delay(80);
}