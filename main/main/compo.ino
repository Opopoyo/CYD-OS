#include "compo.h"
#include "menu_core.h"
#include "audio_system.h"
#include "audio_wav_player.h"

#define COMPO_SOUND_DIR "/Sampler/Sounds"
#define COMPO_PROJECT_DIR "/Sampler/Compositions"

#define COMPO_MAX_SOUNDS 25
#define COMPO_TRACKS 4
#define COMPO_STEPS 8

String compoSoundNames[COMPO_MAX_SOUNDS];
String compoSoundPaths[COMPO_MAX_SOUNDS];

int compoSoundCount = 0;
int compoSelectedSound = 0;

int compoGrid[COMPO_TRACKS][COMPO_STEPS];

int compoBpm = 110;
int compoVolume = 80;
int compoCurrentStep = 0;

bool compoPlaying = false;

unsigned long compoLastStep = 0;
unsigned long compoLastTouch = 0;

WavPlayer compoWav;

// =====================================================
// OUTILS
// =====================================================

bool compoIsWav(String name) {
  String n = name;
  n.toLowerCase();
  return n.endsWith(".wav");
}

String compoCleanName(String path) {
  String out = path;
  out.replace("/Sampler/Sounds/", "");
  out.replace("Sampler/Sounds/", "");

  if (out.length() > 16) {
    out = out.substring(0, 16);
  }

  return out;
}

void compoClearGrid() {
  for (int t = 0; t < COMPO_TRACKS; t++) {
    for (int s = 0; s < COMPO_STEPS; s++) {
      compoGrid[t][s] = -1;
    }
  }
}

// =====================================================
// SCAN SONS SAMPLER
// =====================================================

void compoScanSounds() {
  compoSoundCount = 0;
  compoSelectedSound = 0;

  if (!startSD()) {
    Serial.println("COMPO SD ECHEC");
    return;
  }

  SD.mkdir("/Sampler");
  SD.mkdir(COMPO_SOUND_DIR);
  SD.mkdir(COMPO_PROJECT_DIR);

  File dir = SD.open(COMPO_SOUND_DIR);

  if (!dir) {
    Serial.println("Dossier /Sampler/Sounds introuvable");
    stopSDStartTouch();
    return;
  }

  while (compoSoundCount < COMPO_MAX_SOUNDS) {
    File entry = dir.openNextFile();

    if (!entry) break;

    if (!entry.isDirectory()) {
      String path = String(entry.name());

      if (!path.startsWith("/")) {
        path = String(COMPO_SOUND_DIR) + "/" + path;
      }

      if (compoIsWav(path)) {
        compoSoundPaths[compoSoundCount] = path;
        compoSoundNames[compoSoundCount] = compoCleanName(path);

        Serial.print("Son Compo : ");
        Serial.println(compoSoundPaths[compoSoundCount]);

        compoSoundCount++;
      }
    }

    entry.close();
  }

  dir.close();
  stopSDStartTouch();
}

// =====================================================
// SAUVEGARDE COMPOSITION
// =====================================================

String compoNextProjectName() {
  if (!startSD()) return "/Sampler/Compositions/compo.txt";

  SD.mkdir("/Sampler");
  SD.mkdir(COMPO_PROJECT_DIR);

  for (int i = 1; i < 999; i++) {
    char buf[55];
    sprintf(buf, "/Sampler/Compositions/compo%03d.txt", i);

    if (!SD.exists(buf)) {
      stopSDStartTouch();
      return String(buf);
    }
  }

  stopSDStartTouch();
  return "/Sampler/Compositions/compo999.txt";
}

void compoSaveProject() {
  String path = compoNextProjectName();

  if (!startSD()) {
    drawBoldText("Erreur SD", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    delay(800);
    compoDrawUI();
    return;
  }

  SD.mkdir("/Sampler");
  SD.mkdir(COMPO_PROJECT_DIR);

  File f = SD.open(path, FILE_WRITE);

  if (!f) {
    stopSDStartTouch();
    drawBoldText("Erreur fichier", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    delay(800);
    compoDrawUI();
    return;
  }

  f.println("COMPO_V1");
  f.println(compoBpm);
  f.println(compoVolume);

  for (int t = 0; t < COMPO_TRACKS; t++) {
    for (int s = 0; s < COMPO_STEPS; s++) {
      f.print(compoGrid[t][s]);
      if (s < COMPO_STEPS - 1) f.print(",");
    }
    f.println();
  }

  f.println("SOUNDS");

  for (int i = 0; i < compoSoundCount; i++) {
    f.println(compoSoundPaths[i]);
  }

  f.close();
  stopSDStartTouch();

  compoDrawUI();

  tft.fillRoundRect(35, 88, 250, 68, 10, TFT_BLACK);
  tft.drawRoundRect(35, 88, 250, 68, 10, TFT_GREEN);
  drawBoldText("Compo sauvegardee", 160, 108, TFT_GREEN, TFT_BLACK, MC_DATUM, 2);
  drawBoldText(path, 160, 136, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  delay(1200);
  compoDrawUI();
}

// =====================================================
// AUDIO
// =====================================================

void compoPlaySound(int soundIndex) {
  if (soundIndex < 0 || soundIndex >= compoSoundCount) return;

  if (!startSD()) return;

  wavPlayerOpen(
    compoWav,
    compoSoundPaths[soundIndex],
    compoVolume
  );

  unsigned long startTime = millis();

  while (wavPlayerIsPlaying(compoWav)) {
    wavPlayerUpdate(compoWav);

    if (millis() - startTime > 3500) {
      wavPlayerStop(compoWav);
      break;
    }

    delay(1);
  }

  wavPlayerStop(compoWav);
  stopSDStartTouch();
}

void compoStepPlayback() {
  if (!compoPlaying) return;

  unsigned long interval = 60000UL / compoBpm / 2;

  if (millis() - compoLastStep < interval) return;

  compoLastStep = millis();

  int oldStep = compoCurrentStep;

  compoCurrentStep++;
  if (compoCurrentStep >= COMPO_STEPS) {
    compoCurrentStep = 0;
  }

  compoDrawCellColumn(oldStep);
  compoDrawCellColumn(compoCurrentStep);

  for (int t = 0; t < COMPO_TRACKS; t++) {
    int snd = compoGrid[t][compoCurrentStep];

    if (snd >= 0 && snd < compoSoundCount) {
      compoPlaySound(snd);
    }
  }
}

// =====================================================
// UI
// =====================================================

uint16_t compoBg() {
  return tft.color565(8, 8, 25);
}

uint16_t compoPanel() {
  return tft.color565(30, 30, 45);
}

uint16_t compoTrackColor(int t) {
  if (t == 0) return tft.color565(255, 80, 130);
  if (t == 1) return tft.color565(80, 210, 255);
  if (t == 2) return tft.color565(120, 255, 120);
  return tft.color565(255, 210, 80);
}

void compoButton(int x, int y, int w, int h, String txt, uint16_t bg, uint16_t fg = TFT_WHITE) {
  tft.fillRoundRect(x, y, w, h, 7, bg);
  tft.drawRoundRect(x, y, w, h, 7, TFT_DARKGREY);
  drawBoldText(txt, x + w / 2, y + h / 2, fg, bg, MC_DATUM, 1);
}

void compoHeader() {
  tft.fillRect(0, 0, 320, 34, TFT_BLACK);
  tft.fillTriangle(11, 17, 31, 6, 31, 28, TFT_WHITE);

  drawBoldText("Compo", 82, 17, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  compoButton(
    130,
    5,
    46,
    24,
    compoPlaying ? "STOP" : "PLAY",
    compoPlaying ? TFT_RED : TFT_GREEN,
    TFT_BLACK
  );

  compoButton(182, 5, 42, 24, "SAV", TFT_ORANGE, TFT_BLACK);
  compoButton(230, 5, 42, 24, "SCAN", TFT_BLUE);
  compoButton(278, 5, 36, 24, "V" + String(compoVolume), TFT_PURPLE);
}

void compoDrawCell(int t, int s) {
  int x = 62 + s * 30;
  int y = 56 + t * 34;

  bool active = compoGrid[t][s] >= 0;
  uint16_t col = active ? compoTrackColor(t) : compoPanel();

  tft.fillRoundRect(x, y, 24, 24, 6, col);
  tft.drawRoundRect(x, y, 24, 24, 6, TFT_BLACK);

  if (s == compoCurrentStep && compoPlaying) {
    tft.drawRoundRect(x - 2, y - 2, 28, 28, 6, TFT_YELLOW);
  }

  if (active) {
    drawBoldText(
      String(compoGrid[t][s] + 1),
      x + 12,
      y + 12,
      TFT_BLACK,
      col,
      MC_DATUM,
      1
    );
  }
}

void compoDrawCellColumn(int step) {
  if (step < 0 || step >= COMPO_STEPS) return;

  for (int t = 0; t < COMPO_TRACKS; t++) {
    compoDrawCell(t, step);
  }
}

void compoDrawGrid() {
  for (int s = 0; s < COMPO_STEPS; s++) {
    drawBoldText(
      String(s + 1),
      74 + s * 30,
      44,
      TFT_LIGHTGREY,
      compoBg(),
      MC_DATUM,
      1
    );
  }

  for (int t = 0; t < COMPO_TRACKS; t++) {
    drawBoldText(
      "T" + String(t + 1),
      40,
      68 + t * 34,
      compoTrackColor(t),
      compoBg(),
      MC_DATUM,
      1
    );

    for (int s = 0; s < COMPO_STEPS; s++) {
      compoDrawCell(t, s);
    }
  }
}

void compoDrawSoundPanel() {
  tft.fillRoundRect(8, 196, 304, 38, 8, TFT_BLACK);
  tft.drawRoundRect(8, 196, 304, 38, 8, TFT_DARKGREY);

  if (compoSoundCount <= 0) {
    drawBoldText("Aucun sample dans /Sampler/Sounds", 160, 215, TFT_RED, TFT_BLACK, MC_DATUM, 1);
    return;
  }

  String txt =
    "Son " +
    String(compoSelectedSound + 1) +
    "/" +
    String(compoSoundCount) +
    " : " +
    compoSoundNames[compoSelectedSound];

  drawBoldText(txt, 160, 208, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  compoButton(16, 216, 32, 16, "<", TFT_DARKGREY);
  compoButton(54, 216, 32, 16, ">", TFT_DARKGREY);
  compoButton(250, 216, 52, 16, "TEST", TFT_GREEN, TFT_BLACK);
}

void compoDrawUI() {
  tft.fillScreen(compoBg());

  compoHeader();

  drawBoldText("BPM", 22, 48, TFT_WHITE, compoBg(), MC_DATUM, 1);
  drawBoldText(String(compoBpm), 22, 66, TFT_CYAN, compoBg(), MC_DATUM, 2);

  compoButton(8, 88, 34, 24, "-", TFT_DARKGREY);
  compoButton(8, 116, 34, 24, "+", TFT_DARKGREY);

  compoDrawGrid();
  compoDrawSoundPanel();
}

// =====================================================
// LANCEMENT
// =====================================================

void launchCompo() {
  appMode = MODE_COMPO;

  stopSDStartTouch();
  audioSystemStop();

  wavPlayerInit(compoWav);

  compoBpm = 110;
  compoVolume = 80;
  compoCurrentStep = 0;
  compoPlaying = false;

  compoClearGrid();
  compoScanSounds();

  compoDrawUI();

  Serial.println("COMPO WAV SAMPLER PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateCompo() {
  compoStepPlayback();

  int x, y;

  if (!getTouch(x, y)) return;

  if (millis() - compoLastTouch < 170) return;
  compoLastTouch = millis();

  if (y < 34 && x < 45) {
    compoPlaying = false;
    wavPlayerStop(compoWav);
    audioSystemStop();

    drawSubMenu(currentMenu);

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // PLAY / STOP
  if (x >= 130 && x <= 176 && y >= 5 && y <= 29) {
    compoPlaying = !compoPlaying;
    compoLastStep = millis();
    compoCurrentStep = 0;
    compoDrawUI();
  }

  // SAVE
  else if (x >= 182 && x <= 224 && y >= 5 && y <= 29) {
    compoSaveProject();
  }

  // SCAN
  else if (x >= 230 && x <= 272 && y >= 5 && y <= 29) {
    compoScanSounds();
    compoDrawUI();
  }

  // VOLUME
  else if (x >= 278 && x <= 314 && y >= 5 && y <= 29) {
    compoVolume += 10;
    if (compoVolume > 100) compoVolume = 10;

    wavPlayerSetVolume(compoWav, compoVolume);
    compoDrawUI();
  }

  // BPM -
  else if (x >= 8 && x <= 42 && y >= 88 && y <= 112) {
    compoBpm -= 5;
    if (compoBpm < 60) compoBpm = 60;
    compoDrawUI();
  }

  // BPM +
  else if (x >= 8 && x <= 42 && y >= 116 && y <= 140) {
    compoBpm += 5;
    if (compoBpm > 180) compoBpm = 180;
    compoDrawUI();
  }

  // Son précédent
  else if (x >= 16 && x <= 48 && y >= 216 && y <= 232) {
    if (compoSoundCount > 0) {
      compoSelectedSound--;
      if (compoSelectedSound < 0) compoSelectedSound = compoSoundCount - 1;
      compoDrawUI();
    }
  }

  // Son suivant
  else if (x >= 54 && x <= 86 && y >= 216 && y <= 232) {
    if (compoSoundCount > 0) {
      compoSelectedSound++;
      if (compoSelectedSound >= compoSoundCount) compoSelectedSound = 0;
      compoDrawUI();
    }
  }

  // TEST
  else if (x >= 250 && x <= 302 && y >= 216 && y <= 232) {
    if (compoSoundCount > 0) {
      compoPlaySound(compoSelectedSound);
      compoDrawUI();
    }
  }

  // Grille
  else if (x >= 62 && x <= 302 && y >= 56 && y <= 192) {
    int step = (x - 62) / 30;
    int track = (y - 56) / 34;

    if (
      track >= 0 &&
      track < COMPO_TRACKS &&
      step >= 0 &&
      step < COMPO_STEPS &&
      compoSoundCount > 0
    ) {
      if (compoGrid[track][step] == compoSelectedSound) {
        compoGrid[track][step] = -1;
      } else {
        compoGrid[track][step] = compoSelectedSound;
      }

      compoDrawUI();
    }
  }

  while (ts.touched()) delay(5);
  delay(80);
}