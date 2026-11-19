#include "snake.h"
#include "menu_core.h"

#define SNAKE_COLS 20
#define SNAKE_ROWS 13
#define SNAKE_CELL 15

#define SNAKE_X 10
#define SNAKE_Y 32

#define SNAKE_MAX_LEN 180

struct SnakePart {
  int x;
  int y;
};

SnakePart snakeBody[SNAKE_MAX_LEN];

int snakeLen = 5;
int snakeDir = 1;      // 0 haut, 1 droite, 2 bas, 3 gauche
int snakeNextDir = 1;

int foodX = 8;
int foodY = 6;

int bonusX = -1;
int bonusY = -1;
bool bonusActive = false;

int snakeScore = 0;
int snakeLevel = 1;
int snakeSpeed = 170;

bool snakeGameOver = false;
bool snakeRunning = false;

unsigned long snakeLastMove = 0;
unsigned long snakeLastTouch = 0;

int snakeTouchStartX = -1;
int snakeTouchStartY = -1;

// =====================================================
// COULEURS FEERIQUES
// =====================================================

uint16_t snColorSky(int y) {
  int r = 10 + y / 8;
  int g = 20 + y / 4;
  int b = 55 + y / 2;

  r = constrain(r, 0, 80);
  g = constrain(g, 0, 120);
  b = constrain(b, 70, 220);

  return tft.color565(r, g, b);
}

uint16_t snRainbow(int i) {
  switch (i % 7) {
    case 0: return tft.color565(255, 80, 120);
    case 1: return tft.color565(255, 150, 60);
    case 2: return tft.color565(255, 230, 80);
    case 3: return tft.color565(80, 220, 120);
    case 4: return tft.color565(80, 190, 255);
    case 5: return tft.color565(130, 110, 255);
  }

  return tft.color565(220, 100, 255);
}

uint16_t snLeafColor(int i) {
  int g = 110 + (i * 17) % 120;
  int r = 20 + (i * 11) % 80;
  return tft.color565(r, g, 70);
}

// =====================================================
// DECOR
// =====================================================

void snDrawLeaf(int x, int y, uint16_t col) {
  tft.fillEllipse(x, y, 5, 9, col);
  tft.drawLine(x, y + 7, x + 7, y + 13, TFT_DARKGREEN);
}

void snDrawFlower(int x, int y) {
  uint16_t p1 = tft.color565(255, 120, 200);
  uint16_t p2 = tft.color565(255, 220, 70);

  tft.fillCircle(x - 4, y, 3, p1);
  tft.fillCircle(x + 4, y, 3, p1);
  tft.fillCircle(x, y - 4, 3, p1);
  tft.fillCircle(x, y + 4, 3, p1);
  tft.fillCircle(x, y, 3, p2);
}

void snDrawBackground() {
  for (int y = 0; y < 240; y += 4) {
    tft.fillRect(0, y, 320, 4, snColorSky(y));
  }

  // étoiles / poussières féeriques
  for (int i = 0; i < 35; i++) {
    int x = random(0, 320);
    int y = random(30, 220);
    uint16_t col = snRainbow(i);
    tft.drawPixel(x, y, col);
    if (i % 4 == 0) tft.drawPixel(x + 1, y, col);
  }

  // bordure végétale
  for (int i = 0; i < 18; i++) {
    snDrawLeaf(5 + i * 18, 228 + random(-3, 3), snLeafColor(i));
  }

  for (int i = 0; i < 6; i++) {
    snDrawFlower(28 + i * 48, 220 + random(-2, 2));
  }

  // zone de jeu
  tft.fillRoundRect(SNAKE_X - 4, SNAKE_Y - 4, SNAKE_COLS * SNAKE_CELL + 8, SNAKE_ROWS * SNAKE_CELL + 8, 8, TFT_BLACK);
  tft.drawRoundRect(SNAKE_X - 4, SNAKE_Y - 4, SNAKE_COLS * SNAKE_CELL + 8, SNAKE_ROWS * SNAKE_CELL + 8, 8, tft.color565(120, 255, 170));

  for (int r = 0; r < SNAKE_ROWS; r++) {
    for (int c = 0; c < SNAKE_COLS; c++) {
      uint16_t bg = ((r + c) % 2 == 0)
        ? tft.color565(12, 40, 32)
        : tft.color565(18, 54, 42);

      tft.fillRect(SNAKE_X + c * SNAKE_CELL, SNAKE_Y + r * SNAKE_CELL, SNAKE_CELL, SNAKE_CELL, bg);
    }
  }
}

void snDrawHud() {
  tft.fillRect(0, 0, 320, 28, TFT_BLACK);

  tft.fillTriangle(11, 15, 31, 5, 31, 25, TFT_WHITE);

  drawBoldText("Snake Fee", 112, 14, tft.color565(150, 255, 180), TFT_BLACK, MC_DATUM, 2);
  drawBoldText("S " + String(snakeScore), 220, 14, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);
  drawBoldText("N " + String(snakeLevel), 285, 14, TFT_CYAN, TFT_BLACK, MC_DATUM, 1);
}

void snDrawCell(int x, int y, uint16_t col) {
  int px = SNAKE_X + x * SNAKE_CELL;
  int py = SNAKE_Y + y * SNAKE_CELL;

  tft.fillRoundRect(px + 1, py + 1, SNAKE_CELL - 2, SNAKE_CELL - 2, 5, col);
  tft.drawRoundRect(px + 2, py + 2, SNAKE_CELL - 4, SNAKE_CELL - 4, 4, TFT_BLACK);
}

void snClearCell(int x, int y) {
  uint16_t bg = ((x + y) % 2 == 0)
    ? tft.color565(12, 40, 32)
    : tft.color565(18, 54, 42);

  tft.fillRect(SNAKE_X + x * SNAKE_CELL, SNAKE_Y + y * SNAKE_CELL, SNAKE_CELL, SNAKE_CELL, bg);
}

void snDrawFood() {
  int px = SNAKE_X + foodX * SNAKE_CELL + SNAKE_CELL / 2;
  int py = SNAKE_Y + foodY * SNAKE_CELL + SNAKE_CELL / 2;

  tft.fillCircle(px, py, 6, TFT_RED);
  tft.fillCircle(px - 2, py - 2, 2, TFT_WHITE);
  tft.drawLine(px + 2, py - 6, px + 7, py - 10, TFT_GREEN);
  tft.fillEllipse(px + 8, py - 11, 3, 5, TFT_GREEN);
}

void snDrawBonus() {
  if (!bonusActive) return;

  int px = SNAKE_X + bonusX * SNAKE_CELL + SNAKE_CELL / 2;
  int py = SNAKE_Y + bonusY * SNAKE_CELL + SNAKE_CELL / 2;

  tft.fillCircle(px, py, 7, TFT_YELLOW);
  tft.drawCircle(px, py, 7, TFT_WHITE);
  drawBoldText("*", px, py, TFT_PURPLE, TFT_YELLOW, MC_DATUM, 2);
}

// =====================================================
// JEU
// =====================================================

bool snIsSnakeAt(int x, int y) {
  for (int i = 0; i < snakeLen; i++) {
    if (snakeBody[i].x == x && snakeBody[i].y == y) return true;
  }

  return false;
}

void snPlaceFood() {
  do {
    foodX = random(0, SNAKE_COLS);
    foodY = random(0, SNAKE_ROWS);
  } while (snIsSnakeAt(foodX, foodY));
}

void snMaybeBonus() {
  if (bonusActive) return;
  if (random(100) > 25) return;

  do {
    bonusX = random(0, SNAKE_COLS);
    bonusY = random(0, SNAKE_ROWS);
  } while (snIsSnakeAt(bonusX, bonusY) || (bonusX == foodX && bonusY == foodY));

  bonusActive = true;
}

void snDrawSnake() {
  for (int i = snakeLen - 1; i >= 0; i--) {
    uint16_t col = snRainbow(i);

    if (i == 0) col = tft.color565(90, 255, 120);

    snDrawCell(snakeBody[i].x, snakeBody[i].y, col);

    if (i == 0) {
      int px = SNAKE_X + snakeBody[i].x * SNAKE_CELL;
      int py = SNAKE_Y + snakeBody[i].y * SNAKE_CELL;

      tft.fillCircle(px + 5, py + 5, 2, TFT_BLACK);
      tft.fillCircle(px + 10, py + 5, 2, TFT_BLACK);
    }
  }
}

void snGameOver() {
  snakeRunning = false;
  snakeGameOver = true;

  tft.fillRoundRect(40, 76, 240, 84, 12, tft.color565(80, 20, 90));
  tft.drawRoundRect(40, 76, 240, 84, 12, TFT_YELLOW);

  drawBoldText("GAME OVER", 160, 100, TFT_YELLOW, tft.color565(80, 20, 90), MC_DATUM, 2);
  drawBoldText("Score " + String(snakeScore), 160, 125, TFT_WHITE, tft.color565(80, 20, 90), MC_DATUM, 2);
  drawBoldText("touche pour rejouer", 160, 148, TFT_CYAN, tft.color565(80, 20, 90), MC_DATUM, 1);
}

void snResetGame() {
  snakeLen = 5;
  snakeDir = 1;
  snakeNextDir = 1;

  snakeScore = 0;
  snakeLevel = 1;
  snakeSpeed = 170;

  snakeGameOver = false;
  snakeRunning = true;
  bonusActive = false;

  int startX = 7;
  int startY = 6;

  for (int i = 0; i < snakeLen; i++) {
    snakeBody[i].x = startX - i;
    snakeBody[i].y = startY;
  }

  snPlaceFood();

  snDrawBackground();
  snDrawHud();
  snDrawFood();
  snDrawSnake();

  snakeLastMove = millis();
}

void snLevelUp() {
  snakeLevel = 1 + snakeScore / 60;
  snakeSpeed = 170 - (snakeLevel - 1) * 12;
  if (snakeSpeed < 70) snakeSpeed = 70;
}

void snMove() {
  snakeDir = snakeNextDir;

  int nx = snakeBody[0].x;
  int ny = snakeBody[0].y;

  if (snakeDir == 0) ny--;
  if (snakeDir == 1) nx++;
  if (snakeDir == 2) ny++;
  if (snakeDir == 3) nx--;

  if (nx < 0 || nx >= SNAKE_COLS || ny < 0 || ny >= SNAKE_ROWS) {
    snGameOver();
    return;
  }

  for (int i = 0; i < snakeLen; i++) {
    if (snakeBody[i].x == nx && snakeBody[i].y == ny) {
      snGameOver();
      return;
    }
  }

  bool eat = (nx == foodX && ny == foodY);
  bool eatBonus = bonusActive && nx == bonusX && ny == bonusY;

  int oldTailX = snakeBody[snakeLen - 1].x;
  int oldTailY = snakeBody[snakeLen - 1].y;

  for (int i = snakeLen - 1; i > 0; i--) {
    snakeBody[i] = snakeBody[i - 1];
  }

  snakeBody[0].x = nx;
  snakeBody[0].y = ny;

  if (eat && snakeLen < SNAKE_MAX_LEN) {
    snakeLen++;
    snakeBody[snakeLen - 1].x = oldTailX;
    snakeBody[snakeLen - 1].y = oldTailY;

    snakeScore += 10;
    snLevelUp();
    snPlaceFood();
    snMaybeBonus();
  } else {
    snClearCell(oldTailX, oldTailY);
  }

  if (eatBonus) {
    bonusActive = false;
    snakeScore += 35;

    if (snakeLen < SNAKE_MAX_LEN - 2) {
      snakeLen += 2;
      snakeBody[snakeLen - 1] = snakeBody[snakeLen - 3];
      snakeBody[snakeLen - 2] = snakeBody[snakeLen - 3];
    }

    snLevelUp();
  }

  snDrawFood();
  snDrawBonus();
  snDrawSnake();
  snDrawHud();
}

// =====================================================
// TACTILE
// =====================================================

void snHandleSwipe(int x, int y) {
  if (snakeTouchStartX < 0) {
    snakeTouchStartX = x;
    snakeTouchStartY = y;
    return;
  }

  int dx = x - snakeTouchStartX;
  int dy = y - snakeTouchStartY;

  if (abs(dx) < 18 && abs(dy) < 18) return;

  if (abs(dx) > abs(dy)) {
    if (dx > 0 && snakeDir != 3) snakeNextDir = 1;
    if (dx < 0 && snakeDir != 1) snakeNextDir = 3;
  } else {
    if (dy > 0 && snakeDir != 0) snakeNextDir = 2;
    if (dy < 0 && snakeDir != 2) snakeNextDir = 0;
  }

  snakeTouchStartX = x;
  snakeTouchStartY = y;
}

// =====================================================
// LANCEMENT / UPDATE
// =====================================================

void launchSnake() {
  appMode = MODE_SNAKE;

  stopSDStartTouch();
  randomSeed(millis());

  snResetGame();

  Serial.println("SNAKE FEE PRET");
}

void updateSnake() {
  int x, y;

  if (getTouch(x, y)) {
    if (y < 30 && x < 45) {
      snakeRunning = false;
      drawSubMenu(currentMenu);

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    if (snakeGameOver) {
      snResetGame();

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    snHandleSwipe(x, y);
  } else {
    snakeTouchStartX = -1;
    snakeTouchStartY = -1;
  }

  if (!snakeRunning) return;

  if (millis() - snakeLastMove >= snakeSpeed) {
    snakeLastMove = millis();
    snMove();
  }
}