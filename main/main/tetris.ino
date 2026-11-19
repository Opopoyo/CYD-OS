#include "tetris.h"
#include "menu_core.h"
#include "audio_system.h"

#define TET_FW 10
#define TET_FH 19
#define TET_BLOCK 15

#define TET_PANEL_W 70
#define TET_X 78
#define TET_Y 26
#define TET_W_PIX (TET_FW * TET_BLOCK)
#define TET_H_PIX (TET_FH * TET_BLOCK)

#define TET_AUDIO_PIN 26
#define TET_AUDIO_RES 8

char tetPiece[7][17];
char tetField[TET_FW * TET_FH];

int tetCurrent = 0;
int tetNext = 0;
int tetRot = 0;
int tetPX = 3;
int tetPY = 0;

int tetScore = 0;
int tetLevel = 1;
int tetLines = 0;
int tetSpeed = 700;

bool tetGameOver = false;
unsigned long tetLastFall = 0;

bool tetTouching = false;
bool tetMoved = false;
int tetStartX = 0;
int tetStartY = 0;
int tetLastX = 0;
int tetLastY = 0;

uint16_t tetCols[10];

int tetMusicVolume = 35;
bool tetMusicEnabled = true;

int tetMusicNotes[] = {
  659,494,523,587,523,494,440,440,
  523,659,587,523,494,523,587,659,
  523,440,440,0,
  587,698,880,784,698,659,523,659,
  587,523,494,494,523,587,659,
  523,440,440,0
};

int tetMusicDur[] = {
  180,90,90,180,90,90,180,90,
  90,180,90,90,180,90,180,180,
  180,180,220,80,
  180,90,180,90,90,180,90,180,
  90,90,180,90,90,180,180,
  180,180,220,120
};

int tetMusicIndex = 0;
unsigned long tetNextMusic = 0;
bool tetMusicOn = false;

// =====================================================
// MUSIQUE
// =====================================================

void tetMusicStart() {
  if (!tetMusicEnabled || tetMusicVolume <= 0) return;

  tetMusicIndex = 0;
  tetNextMusic = 0;
  tetMusicOn = true;

  pinMode(TET_AUDIO_PIN, OUTPUT);
  ledcAttach(TET_AUDIO_PIN, 1000, TET_AUDIO_RES);
}

void tetMusicStop() {
  ledcWriteTone(TET_AUDIO_PIN, 0);
  ledcWrite(TET_AUDIO_PIN, 0);
  ledcDetach(TET_AUDIO_PIN);
  pinMode(TET_AUDIO_PIN, INPUT_PULLDOWN);
  tetMusicOn = false;
}

void tetMusicUpdate() {
  if (!tetMusicOn) return;

  if (tetMusicVolume <= 0) {
    ledcWriteTone(TET_AUDIO_PIN, 0);
    ledcWrite(TET_AUDIO_PIN, 0);
    return;
  }

  if (millis() < tetNextMusic) return;

  int count = sizeof(tetMusicNotes) / sizeof(tetMusicNotes[0]);

  int note = tetMusicNotes[tetMusicIndex];
  int dur = tetMusicDur[tetMusicIndex];

  if (note > 0) {
    ledcWriteTone(TET_AUDIO_PIN, note);
    ledcWrite(TET_AUDIO_PIN, map(tetMusicVolume, 0, 100, 0, 45));
  } else {
    ledcWriteTone(TET_AUDIO_PIN, 0);
    ledcWrite(TET_AUDIO_PIN, 0);
  }

  tetNextMusic = millis() + dur;
  tetMusicIndex++;
  if (tetMusicIndex >= count) tetMusicIndex = 0;
}

// =====================================================
// TACTILE PORTRAIT
// =====================================================

bool tetGetTouchPortrait(int &x, int &y) {
  int lx, ly;
  if (!getTouch(lx, ly)) return false;

  x = 239 - ly;
  y = lx;

  x = constrain(x, 0, 239);
  y = constrain(y, 0, 319);

  return true;
}

bool tetTouchInBoard(int x, int y) {
  return x >= TET_X &&
         x <= TET_X + TET_W_PIX &&
         y >= TET_Y &&
         y <= TET_Y + TET_H_PIX;
}

// =====================================================
// LOGIQUE TETRIS
// =====================================================

int tetRotateIndex(int px, int py, int r) {
  switch (r % 4) {
    case 0: return py * 4 + px;
    case 1: return 12 + py - px * 4;
    case 2: return 15 - py * 4 - px;
    case 3: return 3 - py + px * 4;
  }
  return 0;
}

bool tetFit(int piece, int rot, int x, int y) {
  for (int px = 0; px < 4; px++) {
    for (int py = 0; py < 4; py++) {
      if (tetPiece[piece][tetRotateIndex(px, py, rot)] == '.') continue;

      int fx = x + px;
      int fy = y + py;

      if (fx < 0 || fx >= TET_FW || fy < 0 || fy >= TET_FH) return false;
      if (tetField[fy * TET_FW + fx] != 0) return false;
    }
  }

  return true;
}

void tetInitPieces() {
  strcpy(tetPiece[0], "..X...X...X...X.");
  strcpy(tetPiece[1], "..X..XX...X.....");
  strcpy(tetPiece[2], ".....XX..XX.....");
  strcpy(tetPiece[3], "..X..XX..X......");
  strcpy(tetPiece[4], ".X...XX...X.....");
  strcpy(tetPiece[5], ".X...X...XX.....");
  strcpy(tetPiece[6], "..X...X..XX.....");
}

void tetInitColors() {
  tetCols[0] = TFT_BLACK;
  tetCols[1] = tft.color565(255, 60, 130);
  tetCols[2] = tft.color565(80, 255, 130);
  tetCols[3] = tft.color565(70, 180, 255);
  tetCols[4] = tft.color565(255, 240, 80);
  tetCols[5] = tft.color565(255, 150, 40);
  tetCols[6] = tft.color565(80, 255, 255);
  tetCols[7] = tft.color565(220, 90, 255);
  tetCols[8] = TFT_WHITE;
  tetCols[9] = tft.color565(60, 190, 110);
}

void tetNewPiece() {
  tetCurrent = tetNext;
  tetNext = random(0, 7);
  tetRot = 0;
  tetPX = TET_FW / 2 - 2;
  tetPY = 0;

  if (!tetFit(tetCurrent, tetRot, tetPX, tetPY)) {
    tetGameOver = true;
    tetMusicStop();
  }
}

// =====================================================
// GRAPHISME
// =====================================================

uint16_t tetBg(int y) {
  return tft.color565(
    constrain(8 + y / 12, 0, 90),
    constrain(2 + y / 30, 0, 40),
    constrain(35 + y / 3, 45, 170)
  );
}

void tetDrawPanelButton(int x, int y, int w, int h, String txt, uint16_t bg) {
  tft.fillRoundRect(x, y, w, h, 5, bg);
  tft.drawRoundRect(x, y, w, h, 5, TFT_WHITE);
  drawBoldText(txt, x + w / 2, y + h / 2, TFT_WHITE, bg, MC_DATUM, 1);
}

void tetDrawBack() {
  tft.fillTriangle(8, 18, 28, 7, 28, 29, TFT_WHITE);
}

void tetDrawMiniBlock(int x, int y, uint16_t col) {
  tft.fillRoundRect(x + 1, y + 1, 10, 10, 2, col);
  tft.drawRoundRect(x + 1, y + 1, 10, 10, 2, TFT_BLACK);
  tft.drawFastHLine(x + 3, y + 3, 6, TFT_WHITE);
}

void tetDrawNextPiece() {
  tft.fillRoundRect(8, 146, 54, 54, 7, TFT_BLACK);
  tft.drawRoundRect(8, 146, 54, 54, 7, TFT_CYAN);

  drawBoldText("NEXT", 35, 138, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);

  for (int px = 0; px < 4; px++) {
    for (int py = 0; py < 4; py++) {
      if (tetPiece[tetNext][tetRotateIndex(px, py, 0)] != '.') {
        tetDrawMiniBlock(15 + px * 11, 154 + py * 11, tetCols[tetNext + 1]);
      }
    }
  }
}

void tetDrawPanel() {
  tft.fillRoundRect(2, 2, 66, 316, 8, TFT_BLACK);
  tft.drawRoundRect(2, 2, 66, 316, 8, TFT_MAGENTA);
  tft.drawRoundRect(4, 4, 62, 312, 7, TFT_CYAN);

  tetDrawBack();

  drawBoldText("TETRIS", 35, 45, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);

  drawBoldText("SCORE", 35, 68, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  drawBoldText(String(tetScore), 35, 84, TFT_GREEN, TFT_BLACK, MC_DATUM, 1);

  drawBoldText("NIV", 35, 105, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  drawBoldText(String(tetLevel), 35, 121, TFT_CYAN, TFT_BLACK, MC_DATUM, 1);

  tetDrawNextPiece();

  drawBoldText("VOL " + String(tetMusicVolume), 35, 222, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  tetDrawPanelButton(8, 238, 24, 24, "V-", TFT_DARKGREY);
  tetDrawPanelButton(36, 238, 24, 24, "V+", TFT_DARKGREY);

  if (tetGameOver) {
    tetDrawPanelButton(8, 286, 52, 26, "START", TFT_GREEN);
  }
}

void tetDrawBoardStars() {
  for (int i = 0; i < 45; i++) {
    int x = random(TET_X, TET_X + TET_W_PIX);
    int y = random(TET_Y, TET_Y + TET_H_PIX);

    uint16_t c = TFT_WHITE;
    if (i % 4 == 0) c = TFT_CYAN;
    if (i % 5 == 0) c = TFT_YELLOW;

    tft.drawPixel(x, y, c);
    if (i % 7 == 0) tft.drawPixel(x + 1, y, c);
  }
}

void tetDrawBoardFrame() {
  tft.fillRoundRect(
    TET_X - 6,
    TET_Y - 6,
    TET_W_PIX + 12,
    TET_H_PIX + 12,
    9,
    TFT_BLACK
  );

  tetDrawBoardStars();

  tft.drawRoundRect(TET_X - 6, TET_Y - 6, TET_W_PIX + 12, TET_H_PIX + 12, 9, TFT_MAGENTA);
  tft.drawRoundRect(TET_X - 4, TET_Y - 4, TET_W_PIX + 8, TET_H_PIX + 8, 8, TFT_CYAN);
  tft.drawRoundRect(TET_X - 2, TET_Y - 2, TET_W_PIX + 4, TET_H_PIX + 4, 6, TFT_YELLOW);
}

void tetDrawBlock(int x, int y, uint16_t col) {
  int px = TET_X + x * TET_BLOCK;
  int py = TET_Y + y * TET_BLOCK;

  if (col == TFT_BLACK) {
    tft.fillRect(px, py, TET_BLOCK, TET_BLOCK, TFT_BLACK);
    tft.drawRect(px, py, TET_BLOCK, TET_BLOCK, tft.color565(18, 16, 35));
    return;
  }

  tft.fillRoundRect(px + 1, py + 1, TET_BLOCK - 2, TET_BLOCK - 2, 3, col);
  tft.drawFastHLine(px + 3, py + 3, TET_BLOCK - 6, TFT_WHITE);
  tft.drawFastVLine(px + 3, py + 3, TET_BLOCK - 6, TFT_WHITE);
  tft.drawFastHLine(px + 3, py + TET_BLOCK - 4, TET_BLOCK - 6, TFT_DARKGREY);
  tft.drawFastVLine(px + TET_BLOCK - 4, py + 3, TET_BLOCK - 6, TFT_DARKGREY);
  tft.fillCircle(px + 4, py + 4, 2, TFT_WHITE);
  tft.drawRoundRect(px + 1, py + 1, TET_BLOCK - 2, TET_BLOCK - 2, 3, TFT_BLACK);
}

void tetDrawField() {
  tetDrawBoardFrame();

  for (int y = 0; y < TET_FH; y++) {
    for (int x = 0; x < TET_FW; x++) {
      int v = tetField[y * TET_FW + x];
      if (v > 0) tetDrawBlock(x, y, tetCols[v]);
    }
  }
}

void tetDrawPiece(bool erase) {
  for (int px = 0; px < 4; px++) {
    for (int py = 0; py < 4; py++) {
      if (tetPiece[tetCurrent][tetRotateIndex(px, py, tetRot)] != '.') {
        tetDrawBlock(
          tetPX + px,
          tetPY + py,
          erase ? TFT_BLACK : tetCols[tetCurrent + 1]
        );
      }
    }
  }
}

void tetDrawDecor() {
  for (int y = 0; y < 320; y += 4) {
    tft.fillRect(0, y, 240, 4, tetBg(y));
  }

  tetDrawBoardFrame();
  tetDrawPanel();
}

void tetDrawScene() {
  tetDrawDecor();
  tetDrawField();
  tetDrawPanel();
}

// =====================================================
// ACTIONS
// =====================================================

void tetClearLines() {
  int lines = 0;

  for (int y = 0; y < TET_FH; y++) {
    bool full = true;

    for (int x = 0; x < TET_FW; x++) {
      if (tetField[y * TET_FW + x] == 0) {
        full = false;
        break;
      }
    }

    if (full) {
      lines++;

      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < TET_FW; x++) {
          tetField[yy * TET_FW + x] = tetField[(yy - 1) * TET_FW + x];
        }
      }

      for (int x = 0; x < TET_FW; x++) {
        tetField[x] = 0;
      }
    }
  }

  if (lines > 0) {
    tetLines += lines;
    tetScore += (1 << lines) * 100;

    tetLevel = 1 + tetLines / 4;
    tetSpeed = 700 - (tetLevel - 1) * 48;
    if (tetSpeed < 130) tetSpeed = 130;

    tetDrawPanel();
  }
}

void tetLockPiece() {
  for (int px = 0; px < 4; px++) {
    for (int py = 0; py < 4; py++) {
      if (tetPiece[tetCurrent][tetRotateIndex(px, py, tetRot)] != '.') {
        int fx = tetPX + px;
        int fy = tetPY + py;

        if (fx >= 0 && fx < TET_FW && fy >= 0 && fy < TET_FH) {
          tetField[fy * TET_FW + fx] = tetCurrent + 1;
        }
      }
    }
  }

  tetScore += 25;
  tetClearLines();
  tetNewPiece();

  tetDrawScene();

  if (!tetGameOver) {
    tetDrawPiece(false);
  }
}

void tetDown() {
  if (tetGameOver) return;

  tetDrawPiece(true);

  if (tetFit(tetCurrent, tetRot, tetPX, tetPY + 1)) {
    tetPY++;
    tetDrawPiece(false);
  } else {
    tetDrawPiece(false);
    tetLockPiece();
  }

  tetDrawPanel();
}

void tetMove(int dx) {
  if (tetGameOver) return;

  tetDrawPiece(true);

  if (tetFit(tetCurrent, tetRot, tetPX + dx, tetPY)) {
    tetPX += dx;
  }

  tetDrawPiece(false);
}

void tetRotatePiece() {
  if (tetGameOver) return;

  tetDrawPiece(true);

  int nr = tetRot + 1;

  if (tetFit(tetCurrent, nr, tetPX, tetPY)) {
    tetRot = nr;
  }
  else if (tetFit(tetCurrent, nr, tetPX - 1, tetPY)) {
    tetPX--;
    tetRot = nr;
  }
  else if (tetFit(tetCurrent, nr, tetPX + 1, tetPY)) {
    tetPX++;
    tetRot = nr;
  }

  tetDrawPiece(false);
}

void tetFastDrop() {
  if (tetGameOver) return;

  while (tetFit(tetCurrent, tetRot, tetPX, tetPY + 1)) {
    tetDown();
    delay(8);
  }
}

void tetDrawGameOver() {
  tft.fillRoundRect(76, 126, 156, 76, 10, TFT_BLACK);
  tft.drawRoundRect(76, 126, 156, 76, 10, TFT_YELLOW);

  drawBoldText("GAME OVER", 154, 148, TFT_RED, TFT_BLACK, MC_DATUM, 2);
  drawBoldText("Score " + String(tetScore), 154, 174, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  tetDrawPanel();
}

void tetRestart() {
  for (int y = 0; y < TET_FH; y++) {
    for (int x = 0; x < TET_FW; x++) {
      tetField[y * TET_FW + x] = 0;
    }
  }

  tetScore = 0;
  tetLevel = 1;
  tetLines = 0;
  tetSpeed = 700;
  tetGameOver = false;

  tetNext = random(0, 7);
  tetNewPiece();

  tetDrawScene();
  tetDrawPiece(false);

  tetMusicStop();
  tetMusicStart();

  tetLastFall = millis();
}

// =====================================================
// CONTROLES DU JEU : UNIQUEMENT DANS LE CADRE
// =====================================================

void tetTouchDown(int x, int y) {
  if (!tetTouchInBoard(x, y)) {
    tetTouching = false;
    return;
  }

  if (!tetTouching) {
    tetTouching = true;
    tetMoved = false;
    tetStartX = x;
    tetStartY = y;
    tetLastX = x;
    tetLastY = y;
    return;
  }

  int dx = x - tetLastX;
  int dy = y - tetLastY;

  if (abs(dx) > 20 && abs(dx) > abs(dy)) {
    tetMove(dx > 0 ? -1 : 1);
    tetMoved = true;
    tetLastX = x;
    tetLastY = y;
  }

  if (dy > 22 && abs(dy) > abs(dx)) {
    tetDown();
    tetMoved = true;
    tetLastX = x;
    tetLastY = y;
  }

  if (dy < -42 && abs(dy) > abs(dx)) {
    tetFastDrop();
    tetMoved = true;
    tetLastX = x;
    tetLastY = y;
  }
}

void tetTouchUp() {
  if (!tetTouching) return;

  int dx = abs(tetLastX - tetStartX);
  int dy = abs(tetLastY - tetStartY);

  if (!tetMoved && dx < 12 && dy < 12) {
    tetRotatePiece();
  }

  tetTouching = false;
}

// =====================================================
// BOUTONS PANNEAU GAUCHE
// =====================================================

void tetExitToMenu() {
  tetMusicStop();
  audioSystemStop();

  tft.setRotation(1);
  drawSubMenu(currentMenu);
}

void tetHandlePanelTouch(int x, int y) {
  tetTouching = false;

  // Triangle retour
  if (x >= 0 && x <= 70 && y >= 0 && y <= 38) {
    tetExitToMenu();
    return;
  }

  // Volume -
  if (x >= 6 && x <= 34 && y >= 232 && y <= 268) {
    tetMusicVolume -= 10;
    if (tetMusicVolume < 0) tetMusicVolume = 0;

    if (tetMusicVolume == 0) {
      ledcWriteTone(TET_AUDIO_PIN, 0);
      ledcWrite(TET_AUDIO_PIN, 0);
    } else if (!tetMusicOn) {
      tetMusicStart();
    }

    tetDrawScene();
    if (!tetGameOver) tetDrawPiece(false);
    return;
  }

  // Volume +
  if (x >= 34 && x <= 66 && y >= 232 && y <= 268) {
    tetMusicVolume += 10;
    if (tetMusicVolume > 100) tetMusicVolume = 100;

    if (!tetMusicOn && tetMusicVolume > 0) tetMusicStart();

    tetDrawScene();
    if (!tetGameOver) tetDrawPiece(false);
    return;
  }

  // Start game over
  if (tetGameOver && x >= 6 && x <= 66 && y >= 278 && y <= 318) {
    tetRestart();
    return;
  }
}

// =====================================================
// LANCEMENT / UPDATE
// =====================================================

void launchTetris() {
  appMode = MODE_TETRIS;

  stopSDStartTouch();

  tft.setRotation(2);

  tetMusicVolume = 35;
  tetMusicEnabled = true;

  tetInitPieces();
  tetInitColors();

  randomSeed(millis());
  tetRestart();

  Serial.println("TETRIS PRET");
}

void updateTetris() {
  tetMusicUpdate();

  int x, y;

  if (tetGetTouchPortrait(x, y)) {
    // Priorité absolue au panneau gauche
    if (x < TET_PANEL_W) {
      tetHandlePanelTouch(x, y);

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    // Le jeu ne répond QUE dans le cadre des blocs
    if (!tetGameOver && tetTouchInBoard(x, y)) {
      tetTouchDown(x, y);
    } else {
      tetTouching = false;
    }
  } else {
    tetTouchUp();
  }

  if (appMode != MODE_TETRIS) return;

  if (!tetGameOver && millis() - tetLastFall > tetSpeed) {
    tetLastFall = millis();
    tetDown();
  }

  if (tetGameOver) {
    tetDrawGameOver();
  }
}