#include "instrument.h"
#include "menu_core.h"
#include "audio_system.h"

#define AUDIO_PIN 26
#define AUDIO_RES 8

enum InstMode {
  INST_PIANO,
  INST_FLUTE,
  INST_PERCU,
  INST_ELECTRO
};

InstMode instMode = INST_PIANO;

InstButton instButtons[36];
int instButtonCount = 0;

int instVolume = 70;
bool electroLoop = false;
unsigned long lastLoopStep = 0;
int loopStep = 0;

int notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
String noteNames[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO+"};

uint16_t INST_PANEL()  { return tft.color565(35, 35, 35); }
uint16_t INST_BLUE()   { return tft.color565(80, 150, 230); }
uint16_t INST_GREEN()  { return tft.color565(80, 190, 110); }
uint16_t INST_ORANGE() { return tft.color565(230, 150, 70); }
uint16_t INST_PURPLE() { return tft.color565(175, 110, 230); }

// =====================================================
// AUDIO
// =====================================================

void instAudioStop() {
  audioSystemStop();
}

void instAudioTone(int freq, int duration, int vol = -1) {
  int v = (vol < 0) ? instVolume : vol;
  v = constrain(v, 0, 180);

  audioSystemTone(freq, v, duration);
}

// =====================================================
// BOUTONS
// =====================================================

void instAddButton(int x, int y, int w, int h, String label, int index) {
  if (instButtonCount >= 36) return;

  instButtons[instButtonCount].x = x;
  instButtons[instButtonCount].y = y;
  instButtons[instButtonCount].w = w;
  instButtons[instButtonCount].h = h;
  instButtons[instButtonCount].label = label;
  instButtons[instButtonCount].index = index;
  instButtonCount++;
}

bool instInButton(InstButton b, int x, int y) {
  return x >= b.x && x <= b.x + b.w &&
         y >= b.y && y <= b.y + b.h;
}

void instDrawButton(InstButton b, uint16_t bg, uint16_t fg) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, bg);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_BLACK);

  drawBoldText(
    b.label,
    b.x + b.w / 2,
    b.y + b.h / 2,
    fg,
    bg,
    MC_DATUM,
    1
  );
}

// =====================================================
// UI
// =====================================================

void instHeader() {
  tft.fillRect(0, 0, 320, 36, TFT_BLACK);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);

  drawBoldText("Instrument", 155, 18, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  instAddButton(248, 6, 28, 22, "V-", -20);
  instAddButton(282, 6, 28, 22, "V+", -21);

  instDrawButton(instButtons[instButtonCount - 2], TFT_DARKGREY, TFT_WHITE);
  instDrawButton(instButtons[instButtonCount - 1], TFT_DARKGREY, TFT_WHITE);
}

void instDrawModes() {
  instAddButton(5, 40, 75, 28, "Piano", -10);
  instAddButton(84, 40, 75, 28, "Flute", -11);
  instAddButton(163, 40, 75, 28, "Percu", -12);
  instAddButton(242, 40, 73, 28, "Electro", -13);

  for (int i = 0; i < 4; i++) {
    int idx = instButtonCount - 4 + i;

    bool active =
      (i == 0 && instMode == INST_PIANO) ||
      (i == 1 && instMode == INST_FLUTE) ||
      (i == 2 && instMode == INST_PERCU) ||
      (i == 3 && instMode == INST_ELECTRO);

    uint16_t bg = active ? INST_GREEN() : INST_PANEL();

    if (i == 1 && active) bg = INST_BLUE();
    if (i == 2 && active) bg = INST_ORANGE();
    if (i == 3 && active) bg = INST_PURPLE();

    instDrawButton(instButtons[idx], bg, TFT_WHITE);
  }
}

void instDrawKeyboard() {
  int keyW = 38;
  int keyH = 115;
  int startX = 8;
  int startY = 92;

  for (int i = 0; i < 8; i++) {
    instAddButton(startX + i * keyW, startY, keyW - 3, keyH, noteNames[i], i);

    uint16_t bg = TFT_WHITE;
    if (instMode == INST_FLUTE) bg = INST_BLUE();
    if (instMode == INST_ELECTRO) bg = INST_PURPLE();

    instDrawButton(instButtons[instButtonCount - 1], bg, TFT_BLACK);
  }
}

void instDrawPercu() {
  String labels[] = {"BOUM", "CLAP", "TIC", "TOM", "SHAK", "BIP"};
  int ids[] = {100, 101, 102, 103, 104, 105};

  for (int i = 0; i < 6; i++) {
    int x = (i % 2 == 0) ? 20 : 170;
    int y = 82 + (i / 2) * 48;

    instAddButton(x, y, 130, 40, labels[i], ids[i]);
    instDrawButton(instButtons[instButtonCount - 1], INST_ORANGE(), TFT_BLACK);
  }
}

void instDrawElectro() {
  String labels[] = {"KICK", "SNARE", "HAT", "BASS", "SUB", "LEAD", "LOOP", "STOP"};
  int ids[] = {200, 201, 202, 203, 204, 205, 206, 207};

  for (int i = 0; i < 8; i++) {
    int x = (i % 2 == 0) ? 20 : 170;
    int y = 78 + (i / 2) * 38;

    instAddButton(x, y, 130, 32, labels[i], ids[i]);

    uint16_t bg = INST_PURPLE();
    if (ids[i] == 206 && electroLoop) bg = INST_GREEN();
    if (ids[i] == 207) bg = TFT_RED;

    instDrawButton(instButtons[instButtonCount - 1], bg, TFT_WHITE);
  }
}

void instDrawInstrument() {
  instButtonCount = 0;

  tft.fillScreen(TFT_BLACK);
  instHeader();
  instDrawModes();

  drawBoldText(
    "Vol " + String(instVolume),
    160,
    75,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    1
  );

  if (instMode == INST_PERCU) {
    instDrawPercu();
  }
  else if (instMode == INST_ELECTRO) {
    instDrawElectro();
  }
  else {
    instDrawKeyboard();
  }
}

// =====================================================
// SONS
// =====================================================

void instPlayPiano(int freq) {
  instAudioTone(freq, 120);
  delay(15);
  instAudioTone(freq / 2, 45, instVolume / 2);
}

void instPlayFlute(int freq) {
  int softVol = max(20, instVolume / 2);

  for (int i = 0; i < 4; i++) {
    instAudioTone(freq - 10 + i * 5, 45, softVol);
    delay(8);
  }

  instAudioTone(freq, 180, softVol);
  delay(15);
  instAudioTone(freq + 7, 90, softVol / 2);
}

void instPlayPercu(int id) {
  if (id == 100) {
    instAudioTone(90, 65, instVolume);
    instAudioTone(55, 70, instVolume);
  }
  else if (id == 101) {
    instAudioTone(650, 30, instVolume);
    instAudioTone(900, 22, instVolume / 2);
  }
  else if (id == 102) {
    instAudioTone(2400, 18, instVolume / 2);
  }
  else if (id == 103) {
    instAudioTone(190, 55, instVolume);
    instAudioTone(130, 65, instVolume / 2);
  }
  else if (id == 104) {
    for (int i = 0; i < 4; i++) {
      instAudioTone(1600 + random(-200, 200), 18, instVolume / 3);
      delay(12);
    }
  }
  else if (id == 105) {
    instAudioTone(1200, 35, instVolume / 2);
    instAudioTone(1800, 35, instVolume / 2);
  }
}

void instPlayElectro(int id) {
  if (id == 200) {
    instAudioTone(70, 60, instVolume);
    instAudioTone(45, 70, instVolume);
  }
  else if (id == 201) {
    instAudioTone(520, 35, instVolume);
    instAudioTone(820, 25, instVolume / 2);
  }
  else if (id == 202) {
    instAudioTone(2600, 18, instVolume / 3);
  }
  else if (id == 203) {
    instAudioTone(110, 90, instVolume);
    instAudioTone(98, 90, instVolume);
  }
  else if (id == 204) {
    instAudioTone(55, 140, instVolume);
  }
  else if (id == 205) {
    instAudioTone(440, 60, instVolume / 2);
    instAudioTone(660, 60, instVolume / 2);
    instAudioTone(880, 80, instVolume / 2);
  }
  else if (id == 206) {
    electroLoop = !electroLoop;
    loopStep = 0;
    instDrawInstrument();
  }
  else if (id == 207) {
    electroLoop = false;
    instAudioStop();
    instDrawInstrument();
  }
}

void instElectroLoopUpdate() {
  if (!electroLoop) return;
  if (millis() - lastLoopStep < 240) return;

  lastLoopStep = millis();

  if (loopStep == 0) instAudioTone(70, 45, instVolume / 2);
  if (loopStep == 1) instAudioTone(2200, 15, instVolume / 4);
  if (loopStep == 2) instAudioTone(120, 45, instVolume / 2);
  if (loopStep == 3) instAudioTone(650, 25, instVolume / 3);
  if (loopStep == 4) instAudioTone(90, 45, instVolume / 2);
  if (loopStep == 5) instAudioTone(2200, 15, instVolume / 4);
  if (loopStep == 6) instAudioTone(160, 45, instVolume / 2);
  if (loopStep == 7) instAudioTone(850, 25, instVolume / 3);

  loopStep++;
  if (loopStep > 7) loopStep = 0;
}

void instPlayButton(int index) {
  if (instMode == INST_PERCU) {
    instPlayPercu(index);
    return;
  }

  if (instMode == INST_ELECTRO) {
    instPlayElectro(index);
    return;
  }

  if (index < 0 || index > 7) return;

  int freq = notes[index];

  if (instMode == INST_PIANO) {
    instPlayPiano(freq);
  }
  else if (instMode == INST_FLUTE) {
    instPlayFlute(freq);
  }
}

// =====================================================
// LANCEMENT
// =====================================================

void launchInstrument() {
  appMode = MODE_INSTRUMENT;

  stopSDStartTouch();

  audioSystemStop();

  instMode = INST_PIANO;
  electroLoop = false;
  instVolume = 70;

  instDrawInstrument();

  Serial.println("INSTRUMENT PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateInstrument() {
  instElectroLoopUpdate();

  int x, y;

  if (!getTouch(x, y)) return;

  if (y < 36 && x < 45) {
    electroLoop = false;
    audioSystemStop();

    drawSubMenu(currentMenu);

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  for (int i = 0; i < instButtonCount; i++) {
    if (!instInButton(instButtons[i], x, y)) continue;

    int idx = instButtons[i].index;

    if (idx == -20) {
      instVolume -= 15;
      if (instVolume < 20) instVolume = 20;
      instDrawInstrument();
    }
    else if (idx == -21) {
      instVolume += 15;
      if (instVolume > 160) instVolume = 160;
      instDrawInstrument();
    }
    else if (idx == -10) {
      instMode = INST_PIANO;
      electroLoop = false;
      audioSystemStop();
      instDrawInstrument();
    }
    else if (idx == -11) {
      instMode = INST_FLUTE;
      electroLoop = false;
      audioSystemStop();
      instDrawInstrument();
    }
    else if (idx == -12) {
      instMode = INST_PERCU;
      electroLoop = false;
      audioSystemStop();
      instDrawInstrument();
    }
    else if (idx == -13) {
      instMode = INST_ELECTRO;
      instDrawInstrument();
    }
    else {
      instDrawButton(instButtons[i], INST_GREEN(), TFT_BLACK);
      instPlayButton(idx);

      if (idx != 206 && idx != 207) {
        delay(40);
        instDrawInstrument();
      }
    }

    while (ts.touched()) delay(5);

    if (!electroLoop) {
      audioSystemStop();
    }

    delay(35);
    return;
  }
}