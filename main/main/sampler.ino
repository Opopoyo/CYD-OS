#include "sampler.h"
#include "menu_core.h"
#include "audio_system.h"

#define SMP_GRAPH_X 10
#define SMP_GRAPH_Y 40
#define SMP_GRAPH_W 300
#define SMP_GRAPH_H 110

uint8_t smpCurve[SMP_CURVE_N];

int smpCurrentPreset = 0;

int smpDurationMs = 500;
int smpFrequency = 220;
int smpVolume = 70;

bool smpDrawing = false;

unsigned long smpLastTouch = 0;

// =====================================================
// COULEURS
// =====================================================

uint16_t smpBgColor() {
  return tft.color565(10, 10, 35);
}

uint16_t smpPanelColor() {
  return tft.color565(35, 35, 65);
}

uint16_t smpAccentColor() {
  return tft.color565(90, 220, 255);
}

// =====================================================
// OUTILS
// =====================================================
void smpDrawUI();
void smpDrawGraph();


void smpDrawButton(
  int x,
  int y,
  int w,
  int h,
  String txt,
  uint16_t bg
) {
  tft.fillRoundRect(x, y, w, h, 8, bg);

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    8,
    TFT_WHITE
  );

  drawBoldText(
    txt,
    x + w / 2,
    y + h / 2,
    TFT_WHITE,
    bg,
    MC_DATUM,
    1
  );
}

// =====================================================
// ECRAN
// =====================================================

void smpDrawHeader() {

  tft.fillRect(
    0,
    0,
    320,
    34,
    TFT_BLACK
  );

  tft.fillTriangle(
    11,
    17,
    31,
    6,
    31,
    28,
    TFT_WHITE
  );

  drawBoldText(
    "Createur Son",
    160,
    17,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );
}


//sampler.ino (PARTIE 2)


void smpDrawGraph() {

  tft.fillRoundRect(
    SMP_GRAPH_X,
    SMP_GRAPH_Y,
    SMP_GRAPH_W,
    SMP_GRAPH_H,
    8,
    TFT_BLACK
  );

  tft.drawRoundRect(
    SMP_GRAPH_X,
    SMP_GRAPH_Y,
    SMP_GRAPH_W,
    SMP_GRAPH_H,
    8,
    smpAccentColor()
  );

  int mid =
    SMP_GRAPH_Y +
    SMP_GRAPH_H / 2;

  tft.drawFastHLine(
    SMP_GRAPH_X + 4,
    mid,
    SMP_GRAPH_W - 8,
    TFT_DARKGREY
  );

  for (int i = 0; i < SMP_CURVE_N - 1; i++) {

    int x1 = SMP_GRAPH_X +
             map(
               i,
               0,
               SMP_CURVE_N - 1,
               5,
               SMP_GRAPH_W - 5
             );

    int x2 = SMP_GRAPH_X +
             map(
               i + 1,
               0,
               SMP_CURVE_N - 1,
               5,
               SMP_GRAPH_W - 5
             );

    int y1 =
      SMP_GRAPH_Y +
      map(
        smpCurve[i],
        255,
        0,
        5,
        SMP_GRAPH_H - 5
      );

    int y2 =
      SMP_GRAPH_Y +
      map(
        smpCurve[i + 1],
        255,
        0,
        5,
        SMP_GRAPH_H - 5
      );

    tft.drawLine(
      x1,
      y1,
      x2,
      y2,
      TFT_YELLOW
    );
  }
}

//sampler.ino (PARTIE 3)

void smpDrawControls() {

  smpDrawButton(
    10,
    160,
    55,
    24,
    "PLAY",
    TFT_GREEN
  );

  smpDrawButton(
    70,
    160,
    55,
    24,
    "SAVE",
    TFT_ORANGE
  );

  smpDrawButton(
    130,
    160,
    55,
    24,
    "RAND",
    TFT_BLUE
  );

  smpDrawButton(
    190,
    160,
    55,
    24,
    "SMTH",
    TFT_PURPLE
  );

  smpDrawButton(
    250,
    160,
    60,
    24,
    smpPresets[smpCurrentPreset].name,
    TFT_RED
  );

  drawBoldText(
    "Duree " + String(smpDurationMs) + "ms",
    70,
    210,
    TFT_WHITE,
    smpBgColor(),
    MC_DATUM,
    1
  );

  drawBoldText(
    "Freq " + String(smpFrequency),
    160,
    210,
    TFT_CYAN,
    smpBgColor(),
    MC_DATUM,
    1
  );

  drawBoldText(
    "Vol " + String(smpVolume),
    260,
    210,
    TFT_YELLOW,
    smpBgColor(),
    MC_DATUM,
    1
  );
}

//Partie 4 — moteur audio preview

uint8_t smpCurveValue(float phase) {
  while (phase >= 1.0f) phase -= 1.0f;
  while (phase < 0.0f) phase += 1.0f;

  float pos = phase * SMP_CURVE_N;
  int i0 = (int)pos;
  int i1 = (i0 + 1) % SMP_CURVE_N;
  float frac = pos - i0;

  return smpCurve[i0] * (1.0f - frac) + smpCurve[i1] * frac;
}

void smpPlayPreview() {
  audioSystemEnable();

  uint32_t totalSamples = ((uint32_t)SMP_SR * smpDurationMs) / 1000;
  unsigned long interval = 1000000UL / SMP_SR;
  unsigned long last = micros();

  for (uint32_t i = 0; i < totalSamples; i++) {
    float t = (float)i / SMP_SR;
    float phase = t * smpFrequency;

    int v = smpCurveValue(phase);
    int centered = v - 128;
    int scaled = 128 + ((centered * smpVolume) / 100);

    scaled = constrain(scaled, 0, 255);

    audioSystemWrite((uint8_t)scaled);

    while (micros() - last < interval) {
      delayMicroseconds(5);
    }

    last += interval;
  }

  audioSystemStop();
}

//Partie 5 — sauvegarde WAV

void smpWrite16(File &f, uint16_t v) {
  f.write(v & 0xFF);
  f.write((v >> 8) & 0xFF);
}

void smpWrite32(File &f, uint32_t v) {
  f.write(v & 0xFF);
  f.write((v >> 8) & 0xFF);
  f.write((v >> 16) & 0xFF);
  f.write((v >> 24) & 0xFF);
}

void smpWriteWavHeader(File &f, uint32_t dataSize) {
  f.print("RIFF");
  smpWrite32(f, 36 + dataSize);
  f.print("WAVE");

  f.print("fmt ");
  smpWrite32(f, 16);
  smpWrite16(f, 1);
  smpWrite16(f, 1);
  smpWrite32(f, SMP_SR);
  smpWrite32(f, SMP_SR * 2);
  smpWrite16(f, 2);
  smpWrite16(f, 16);

  f.print("data");
  smpWrite32(f, dataSize);
}

String smpNextFileName() {
  if (!startSD()) return "/Sampler/Sounds/sample.wav";

  SD.mkdir("/Sampler");
  SD.mkdir("/Sampler/Sounds");

  for (int i = 1; i < 999; i++) {
    char buf[45];
    sprintf(buf, "/Sampler/Sounds/sample%03d.wav", i);

    if (!SD.exists(buf)) {
      stopSDStartTouch();
      return String(buf);
    }
  }

  stopSDStartTouch();
  return "/Sampler/Sounds/sample999.wav";
}

void smpSaveWav() {
  String path = smpNextFileName();

  if (!startSD()) {
    drawBoldText("Erreur SD", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    delay(800);
    smpDrawUI();
    return;
  }

  SD.mkdir("/Sampler");
  SD.mkdir("/Sampler/Sounds");

  File f = SD.open(path, FILE_WRITE);

  if (!f) {
    stopSDStartTouch();
    drawBoldText("Erreur fichier", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    delay(800);
    smpDrawUI();
    return;
  }

  uint32_t totalSamples = ((uint32_t)SMP_SR * smpDurationMs) / 1000;
  uint32_t dataSize = totalSamples * 2;

  smpWriteWavHeader(f, dataSize);

  for (uint32_t i = 0; i < totalSamples; i++) {
    float t = (float)i / SMP_SR;
    float phase = t * smpFrequency;

    int v = smpCurveValue(phase);
    int centered = v - 128;

    int16_t sample = (centered * 250 * smpVolume) / 100;
    sample = constrain(sample, -30000, 30000);

    smpWrite16(f, (uint16_t)sample);
  }

  f.close();
  stopSDStartTouch();

  smpDrawUI();

  tft.fillRoundRect(35, 88, 250, 68, 10, TFT_BLACK);
  tft.drawRoundRect(35, 88, 250, 68, 10, TFT_GREEN);

  drawBoldText("Sample enregistre", 160, 108, TFT_GREEN, TFT_BLACK, MC_DATUM, 2);
  drawBoldText(path, 160, 136, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  delay(1200);
  smpDrawUI();
}


//Partie 6 — tactile + lancement


void smpSmoothCurve() {
  uint8_t tmp[SMP_CURVE_N];

  for (int i = 0; i < SMP_CURVE_N; i++) {
    int a = smpCurve[(i - 1 + SMP_CURVE_N) % SMP_CURVE_N];
    int b = smpCurve[i];
    int c = smpCurve[(i + 1) % SMP_CURVE_N];

    tmp[i] = (a + b + c) / 3;
  }

  for (int i = 0; i < SMP_CURVE_N; i++) {
    smpCurve[i] = tmp[i];
  }
}

void smpRandomCurve() {
  for (int i = 0; i < SMP_CURVE_N; i++) {
    smpCurve[i] = random(20, 236);
  }
}

void smpEditCurveAt(int x, int y) {
  if (x < SMP_GRAPH_X || x > SMP_GRAPH_X + SMP_GRAPH_W) return;
  if (y < SMP_GRAPH_Y || y > SMP_GRAPH_Y + SMP_GRAPH_H) return;

  int index = map(x, SMP_GRAPH_X + 5, SMP_GRAPH_X + SMP_GRAPH_W - 5, 0, SMP_CURVE_N - 1);
  index = constrain(index, 0, SMP_CURVE_N - 1);

  int value = map(y, SMP_GRAPH_Y + 5, SMP_GRAPH_Y + SMP_GRAPH_H - 5, 255, 0);
  value = constrain(value, 0, 255);

  smpCurve[index] = value;

  if (index > 0) smpCurve[index - 1] = (smpCurve[index - 1] + value) / 2;
  if (index < SMP_CURVE_N - 1) smpCurve[index + 1] = (smpCurve[index + 1] + value) / 2;

  smpDrawGraph();
}

void smpDrawUI() {
  tft.fillScreen(smpBgColor());
  smpDrawHeader();
  smpDrawGraph();
  smpDrawControls();
}

void launchSampler() {
  appMode = MODE_SAMPLER;

  stopSDStartTouch();
  audioSystemStop();

  smpCurrentPreset = 0;
  smpDurationMs = 500;
  smpFrequency = 220;
  smpVolume = 70;

  smpApplyPreset(smpCurrentPreset, smpCurve);
  smpDrawUI();

  Serial.println("SAMPLER CREATEUR SON PRET");
}

void updateSampler() {
  int x, y;

  if (!getTouch(x, y)) {
    smpDrawing = false;
    return;
  }

  if (y < 34 && x < 45) {
    audioSystemStop();
    drawSubMenu(currentMenu);

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (
    x >= SMP_GRAPH_X &&
    x <= SMP_GRAPH_X + SMP_GRAPH_W &&
    y >= SMP_GRAPH_Y &&
    y <= SMP_GRAPH_Y + SMP_GRAPH_H
  ) {
    smpEditCurveAt(x, y);
    smpDrawing = true;
    delay(25);
    return;
  }

  if (millis() - smpLastTouch < 180) return;
  smpLastTouch = millis();

  if (x >= 10 && x <= 65 && y >= 160 && y <= 184) {
    smpPlayPreview();
    smpDrawUI();
  }

  else if (x >= 70 && x <= 125 && y >= 160 && y <= 184) {
    smpSaveWav();
  }

  else if (x >= 130 && x <= 185 && y >= 160 && y <= 184) {
    smpRandomCurve();
    smpDrawUI();
  }

  else if (x >= 190 && x <= 245 && y >= 160 && y <= 184) {
    smpSmoothCurve();
    smpDrawUI();
  }

  else if (x >= 250 && x <= 310 && y >= 160 && y <= 184) {
    smpCurrentPreset++;
    if (smpCurrentPreset >= SMP_PRESET_COUNT) smpCurrentPreset = 0;

    smpApplyPreset(smpCurrentPreset, smpCurve);
    smpDrawUI();
  }

  // Zone réglages en bas :
  // gauche = durée, milieu = fréquence, droite = volume

  else if (x < 105 && y >= 195) {
    if (x < 52) smpDurationMs -= 100;
    else smpDurationMs += 100;

    smpDurationMs = constrain(smpDurationMs, 100, 3000);
    smpDrawUI();
  }

  else if (x >= 105 && x < 215 && y >= 195) {
    if (x < 160) smpFrequency -= 20;
    else smpFrequency += 20;

    smpFrequency = constrain(smpFrequency, 40, 2000);
    smpDrawUI();
  }

  else if (x >= 215 && y >= 195) {
    if (x < 267) smpVolume -= 10;
    else smpVolume += 10;

    smpVolume = constrain(smpVolume, 0, 100);
    smpDrawUI();
  }

  while (ts.touched()) delay(5);
  delay(40);
}