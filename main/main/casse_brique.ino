#include "casse_brique.h"
#include "menu_core.h"


#define CB_NONE 0
#define CB_BIG_PAD 1
#define CB_SMALL_PAD 2
#define CB_MULTI 3
#define CB_FAST 4
#define CB_SLOW 5
#define CB_FIRE 6
#define CB_BIG_BALL 7
#define CB_SMALL_BALL 8



CBBrick cbBricks[CB_ROWS][CB_COLS];
CBBall cbBalls[CB_MAX_BALLS];
CBBonus cbBonus[CB_MAX_BONUS];
CBShot cbShots[CB_MAX_SHOTS];

int cbLevel = 1;
int cbLives = 3;
int cbScore = 0;
int cbBrickCount = 0;

int cbPaddleX = 120;
int cbPaddleW = 72;
int cbPaddleY = 222;

bool cbPlaying = false;
bool cbFireMode = false;

unsigned long cbLastFrame = 0;
unsigned long cbLastShot = 0;

const int cbBrickW = 30;
const int cbBrickH = 16;
const int cbBrickX = 10;
const int cbBrickY = 36;

// =====================================================
// COULEURS / GRAPHISME
// =====================================================

uint16_t cbGradient(int r, int c) {
  int red = 60 + r * 25;
  int green = 80 + c * 12;
  int blue = 220 - r * 18;

  red = constrain(red, 0, 255);
  green = constrain(green, 0, 255);
  blue = constrain(blue, 0, 255);

  return tft.color565(red, green, blue);
}

uint16_t cbBonusColor(int type) {
  if (type == CB_BIG_PAD) return TFT_GREEN;
  if (type == CB_SMALL_PAD) return TFT_RED;
  if (type == CB_MULTI) return TFT_CYAN;
  if (type == CB_FAST) return TFT_ORANGE;
  if (type == CB_SLOW) return TFT_BLUE;
  if (type == CB_FIRE) return TFT_YELLOW;
  if (type == CB_BIG_BALL) return TFT_PURPLE;
  if (type == CB_SMALL_BALL) return TFT_PINK;
  return TFT_WHITE;
}

void cbDrawBack() {
  tft.fillRect(0, 0, 42, 34, TFT_BLACK);
  tft.fillTriangle(10, 17, 30, 6, 30, 28, TFT_WHITE);
}

void cbDrawHud() {
  tft.fillRect(42, 0, 278, 28, TFT_BLACK);

  drawBoldText("N" + String(cbLevel), 68, 14, TFT_CYAN, TFT_BLACK, MC_DATUM, 1);
  drawBoldText("V" + String(cbLives), 115, 14, TFT_GREEN, TFT_BLACK, MC_DATUM, 1);
  drawBoldText("S" + String(cbScore), 180, 14, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  if (cbFireMode) {
    drawBoldText("TIR", 285, 14, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);
  }
}

void cbDrawAlien(int x, int y, uint16_t col) {
  tft.fillRoundRect(x + 4, y + 3, cbBrickW - 8, cbBrickH - 5, 4, col);
  tft.fillCircle(x + 10, y + 8, 2, TFT_BLACK);
  tft.fillCircle(x + 20, y + 8, 2, TFT_BLACK);
  tft.drawFastHLine(x + 8, y + 13, 14, TFT_BLACK);
}

void cbDrawBrick(int r, int c) {
  int x = cbBrickX + c * cbBrickW;
  int y = cbBrickY + r * cbBrickH;

  if (!cbBricks[r][c].alive) {
    tft.fillRect(x, y, cbBrickW - 2, cbBrickH - 2, TFT_BLACK);
    return;
  }

  uint16_t col = cbGradient(r, c);

  if (cbBricks[r][c].alien) {
    cbDrawAlien(x, y, col);
  } else {
    tft.fillRoundRect(x, y, cbBrickW - 2, cbBrickH - 2, 3, col);
    tft.drawFastHLine(x + 2, y + 2, cbBrickW - 6, TFT_WHITE);
    tft.drawFastHLine(x + 2, y + cbBrickH - 4, cbBrickW - 6, TFT_DARKGREY);
  }

  if (cbBricks[r][c].hp > 1) {
    drawBoldText(String(cbBricks[r][c].hp), x + cbBrickW / 2, y + 8, TFT_WHITE, col, MC_DATUM, 1);
  }
}

void cbDrawAllBricks() {
  for (int r = 0; r < CB_ROWS; r++) {
    for (int c = 0; c < CB_COLS; c++) {
      cbDrawBrick(r, c);
    }
  }
}

void cbDrawPaddle() {
  tft.fillRoundRect(cbPaddleX, cbPaddleY, cbPaddleW, 9, 5, TFT_BLUE);
  tft.drawFastHLine(cbPaddleX + 4, cbPaddleY + 2, cbPaddleW - 8, TFT_CYAN);
}

void cbErasePaddle() {
  tft.fillRect(0, cbPaddleY - 2, 320, 16, TFT_BLACK);
}

void cbDrawBall(CBBall &b) {
  if (!b.active) return;
  tft.fillCircle((int)b.x, (int)b.y, b.r, b.color);
}

void cbEraseBall(CBBall &b) {
  if (!b.active) return;
  tft.fillCircle((int)b.x, (int)b.y, b.r + 1, TFT_BLACK);
}

void cbDrawBonus(CBBonus &bo) {
  if (!bo.active) return;
  uint16_t col = cbBonusColor(bo.type);
  tft.fillCircle((int)bo.x, (int)bo.y, 7, col);
  tft.drawCircle((int)bo.x, (int)bo.y, 7, TFT_WHITE);
}

void cbEraseBonus(CBBonus &bo) {
  if (!bo.active) return;
  tft.fillCircle((int)bo.x, (int)bo.y, 8, TFT_BLACK);
}

void cbDrawShot(CBShot &s) {
  if (!s.active) return;
  tft.fillRect((int)s.x - 1, (int)s.y - 5, 3, 8, TFT_YELLOW);
}

void cbEraseShot(CBShot &s) {
  if (!s.active) return;
  tft.fillRect((int)s.x - 2, (int)s.y - 6, 5, 10, TFT_BLACK);
}

// =====================================================
// NIVEAUX
// =====================================================

void cbLoadLevel(int level);

// =====================================================
// INIT
// =====================================================

void cbResetBalls() {
  for (int i = 0; i < CB_MAX_BALLS; i++) {
    cbBalls[i].active = false;
  }

  cbBalls[0].x = 160;
  cbBalls[0].y = 200;
  cbBalls[0].vx = 2.1;
  cbBalls[0].vy = -2.4;
  cbBalls[0].r = 4;
  cbBalls[0].color = TFT_WHITE;
  cbBalls[0].active = true;
}

void cbResetBonus() {
  for (int i = 0; i < CB_MAX_BONUS; i++) cbBonus[i].active = false;
  for (int i = 0; i < CB_MAX_SHOTS; i++) cbShots[i].active = false;
}

void cbStartGame() {
  cbPlaying = true;
  cbLives = 3;
  cbScore = 0;
  cbLevel = 1;
  cbPaddleW = 72;
  cbPaddleX = 124;
  cbFireMode = false;

  cbResetBonus();
  cbResetBalls();
  cbLoadLevel(cbLevel);

  tft.fillScreen(TFT_BLACK);
  cbDrawBack();
  cbDrawHud();
  cbDrawAllBricks();
  cbDrawPaddle();
}

void cbNextLevel() {
  cbLevel++;
  cbPaddleW = 72;
  cbFireMode = false;

  cbResetBonus();
  cbResetBalls();
  cbLoadLevel(cbLevel);

  tft.fillScreen(TFT_BLACK);
  cbDrawBack();
  cbDrawHud();
  cbDrawAllBricks();
  cbDrawPaddle();
}

void cbDrawMenu() {
  cbPlaying = false;

  tft.fillScreen(TFT_BLACK);
  cbDrawBack();

  drawBoldText("CASSE", 160, 62, TFT_CYAN, TFT_BLACK, MC_DATUM, 4);
  drawBoldText("BRIQUE", 160, 100, TFT_ORANGE, TFT_BLACK, MC_DATUM, 4);

  tft.fillRoundRect(70, 150, 180, 44, 10, TFT_BLUE);
  drawBoldText("JOUER", 160, 172, TFT_WHITE, TFT_BLUE, MC_DATUM, 2);
}

// =====================================================
// BONUS
// =====================================================

void cbSpawnBonus(float x, float y) {
  if (random(100) > 38) return;

  for (int i = 0; i < CB_MAX_BONUS; i++) {
    if (!cbBonus[i].active) {
      cbBonus[i].x = x;
      cbBonus[i].y = y;
      cbBonus[i].type = random(1, 9);
      cbBonus[i].active = true;
      return;
    }
  }
}

void cbAddBall() {
  for (int i = 1; i < CB_MAX_BALLS; i++) {
    if (!cbBalls[i].active) {
      cbBalls[i].x = cbBalls[0].x;
      cbBalls[i].y = cbBalls[0].y;
      cbBalls[i].vx = -cbBalls[0].vx;
      cbBalls[i].vy = cbBalls[0].vy - 0.4;
      cbBalls[i].r = cbBalls[0].r;
      cbBalls[i].color = cbBonusColor(random(3, 9));
      cbBalls[i].active = true;
      return;
    }
  }
}

void cbApplyBonus(int type) {
  if (type == CB_BIG_PAD) cbPaddleW = min(120, cbPaddleW + 18);
  if (type == CB_SMALL_PAD) cbPaddleW = max(42, cbPaddleW - 14);
  if (type == CB_MULTI) cbAddBall();
  if (type == CB_FIRE) cbFireMode = true;

  for (int i = 0; i < CB_MAX_BALLS; i++) {
    if (!cbBalls[i].active) continue;

    if (type == CB_FAST) {
      cbBalls[i].vx *= 1.18;
      cbBalls[i].vy *= 1.18;
    }

    if (type == CB_SLOW) {
      cbBalls[i].vx *= 0.82;
      cbBalls[i].vy *= 0.82;
    }

    if (type == CB_BIG_BALL) {
      cbBalls[i].r = min(8, cbBalls[i].r + 1);
      cbBalls[i].color = cbBonusColor(type);
    }

    if (type == CB_SMALL_BALL) {
      cbBalls[i].r = max(3, cbBalls[i].r - 1);
      cbBalls[i].color = cbBonusColor(type);
    }
  }
}

// =====================================================
// COLLISIONS
// =====================================================

void cbHitBrick(int r, int c, float bx, float by) {
  if (!cbBricks[r][c].alive) return;

  cbBricks[r][c].hp--;

  if (cbBricks[r][c].hp <= 0) {
    cbBricks[r][c].alive = false;
    cbBrickCount--;
    cbScore += cbBricks[r][c].alien ? 25 : 10;
    cbSpawnBonus(bx, by);
  } else {
    cbScore += 3;
  }

  cbDrawBrick(r, c);
  cbDrawHud();

  if (cbBrickCount <= 0) {
    delay(350);
    cbNextLevel();
  }
}

bool cbBallBrickCollision(CBBall &b) {
  for (int r = 0; r < CB_ROWS; r++) {
    for (int c = 0; c < CB_COLS; c++) {
      if (!cbBricks[r][c].alive) continue;

      int x = cbBrickX + c * cbBrickW;
      int y = cbBrickY + r * cbBrickH;
      int w = cbBrickW - 2;
      int h = cbBrickH - 2;

      if (
        b.x + b.r >= x &&
        b.x - b.r <= x + w &&
        b.y + b.r >= y &&
        b.y - b.r <= y + h
      ) {
        cbHitBrick(r, c, b.x, b.y);
        return true;
      }
    }
  }

  return false;
}

void cbShoot() {
  if (!cbFireMode) return;
  if (millis() - cbLastShot < 260) return;

  cbLastShot = millis();

  for (int i = 0; i < CB_MAX_SHOTS; i++) {
    if (!cbShots[i].active) {
      cbShots[i].x = cbPaddleX + cbPaddleW / 2;
      cbShots[i].y = cbPaddleY - 4;
      cbShots[i].active = true;
      return;
    }
  }
}

void cbUpdateShots() {
  for (int i = 0; i < CB_MAX_SHOTS; i++) {
    if (!cbShots[i].active) continue;

    cbEraseShot(cbShots[i]);

    cbShots[i].y -= 6;

    if (cbShots[i].y < 28) {
      cbShots[i].active = false;
      continue;
    }

    bool hit = false;

    for (int r = 0; r < CB_ROWS && !hit; r++) {
      for (int c = 0; c < CB_COLS && !hit; c++) {
        if (!cbBricks[r][c].alive) continue;

        int x = cbBrickX + c * cbBrickW;
        int y = cbBrickY + r * cbBrickH;

        if (
          cbShots[i].x >= x &&
          cbShots[i].x <= x + cbBrickW &&
          cbShots[i].y >= y &&
          cbShots[i].y <= y + cbBrickH
        ) {
          cbHitBrick(r, c, cbShots[i].x, cbShots[i].y);
          cbShots[i].active = false;
          hit = true;
        }
      }
    }

    cbDrawShot(cbShots[i]);
  }
}

void cbUpdateBonus() {
  for (int i = 0; i < CB_MAX_BONUS; i++) {
    if (!cbBonus[i].active) continue;

    cbEraseBonus(cbBonus[i]);

    cbBonus[i].y += 2.2;

    if (
      cbBonus[i].y >= cbPaddleY - 4 &&
      cbBonus[i].y <= cbPaddleY + 14 &&
      cbBonus[i].x >= cbPaddleX &&
      cbBonus[i].x <= cbPaddleX + cbPaddleW
    ) {
      cbApplyBonus(cbBonus[i].type);
      cbBonus[i].active = false;
      cbErasePaddle();
      cbDrawPaddle();
      continue;
    }

    if (cbBonus[i].y > 240) {
      cbBonus[i].active = false;
      continue;
    }

    cbDrawBonus(cbBonus[i]);
  }
}

void cbUpdateBalls() {
  int activeCount = 0;

  for (int i = 0; i < CB_MAX_BALLS; i++) {
    if (!cbBalls[i].active) continue;

    CBBall &b = cbBalls[i];

    cbEraseBall(b);

    b.x += b.vx;
    b.y += b.vy;

    if (b.x < b.r) {
      b.x = b.r;
      b.vx = -b.vx;
    }

    if (b.x > 319 - b.r) {
      b.x = 319 - b.r;
      b.vx = -b.vx;
    }

    if (b.y < 28 + b.r) {
      b.y = 28 + b.r;
      b.vy = -b.vy;
    }

    if (
      b.y + b.r >= cbPaddleY &&
      b.y - b.r <= cbPaddleY + 10 &&
      b.x >= cbPaddleX &&
      b.x <= cbPaddleX + cbPaddleW &&
      b.vy > 0
    ) {
      float hit = (b.x - cbPaddleX) / cbPaddleW;
      b.vx = (hit - 0.5) * 5.2;
      b.vy = -abs(b.vy);
      b.y = cbPaddleY - b.r - 1;
    }

    if (cbBallBrickCollision(b)) {
      b.vy = -b.vy;
      b.color = cbBonusColor(random(1, 9));
    }

    if (b.y > 245) {
      b.active = false;
    } else {
      cbDrawBall(b);
      activeCount++;
    }
  }

  if (activeCount == 0) {
    cbLives--;
    cbDrawHud();

    if (cbLives <= 0) {
      cbPlaying = false;
      tft.fillRoundRect(55, 88, 210, 64, 10, TFT_RED);
      drawBoldText("GAME OVER", 160, 110, TFT_WHITE, TFT_RED, MC_DATUM, 2);
      drawBoldText("touche retour", 160, 134, TFT_WHITE, TFT_RED, MC_DATUM, 1);
      return;
    }

    delay(400);
    cbResetBalls();
    cbErasePaddle();
    cbPaddleX = 124;
    cbDrawPaddle();
  }
}

// =====================================================
// LANCEMENT / UPDATE
// =====================================================

void launchCasseBrique() {
  appMode = MODE_CASSE_BRIQUE;
  stopSDStartTouch();
  randomSeed(millis());
  cbDrawMenu();
  Serial.println("CASSE BRIQUE PRET");
}

void updateCasseBrique() {
  int x, y;

  if (getTouch(x, y)) {
    if (y < 36 && x < 45) {
      drawSubMenu(currentMenu);
      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    if (!cbPlaying) {
      if (x >= 70 && x <= 250 && y >= 150 && y <= 194) {
        cbStartGame();
      }

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    cbErasePaddle();

    cbPaddleX = x - cbPaddleW / 2;
    cbPaddleX = constrain(cbPaddleX, 0, 320 - cbPaddleW);

    cbDrawPaddle();

    if (cbFireMode) cbShoot();
  }

  if (!cbPlaying) return;

  if (millis() - cbLastFrame < 24) return;
  cbLastFrame = millis();

  cbUpdateBalls();
  cbUpdateBonus();
  cbUpdateShots();
}