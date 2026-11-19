#include "jeu_chat.h"
#include "menu_core.h"

int catX = 160;
int catY = 140;

int mouseX = 60;
int mouseY = 80;

int dogX = 280;
int dogY = 200;
int dogDX = -8;
int dogDY = -6;

int scoreChat = 0;

bool gameOver = false;

unsigned long gameStartTime = 0;
unsigned long lastMouseMove = 0;
unsigned long lastDogMove = 0;
unsigned long lastCactusAdd = 0;

const int MAX_CACTUS = 8;

int cactusX[MAX_CACTUS];
int cactusY[MAX_CACTUS];
int cactusCount = 0;

// =====================================================
// COULEURS
// =====================================================

uint16_t CHAT_COLOR() {
  return tft.color565(255, 220, 0);   // jaune
}

uint16_t CACTUS_COLOR() {
  return tft.color565(0, 180, 60);    // vert
}

// =====================================================
// CHAT ROUGE PLUS PETIT
// =====================================================

void drawCat(int x, int y) {
  uint16_t c = CHAT_COLOR();

  // corps
  tft.fillCircle(x, y + 5, 10, c);

  // tete
  tft.fillCircle(x, y - 8, 9, c);

  // oreilles
  tft.fillTriangle(
    x - 8, y - 13,
    x - 3, y - 23,
    x,     y - 12,
    c
  );

  tft.fillTriangle(
    x + 8, y - 13,
    x + 3, y - 23,
    x,     y - 12,
    c
  );

  // queue
  tft.drawLine(x + 9, y + 5, x + 18, y - 3, c);
  tft.drawLine(x + 10, y + 6, x + 19, y - 2, c);

  // yeux
  tft.fillCircle(x - 3, y - 9, 1, TFT_BLACK);
  tft.fillCircle(x + 3, y - 9, 1, TFT_BLACK);

  // nez
  tft.fillCircle(x, y - 5, 1, TFT_BLACK);
}

void eraseCat(int x, int y) {
  tft.fillRect(x - 23, y - 27, 48, 45, TFT_BLACK);
}

// =====================================================
// SOURIS
// =====================================================

void drawMouse(int x, int y) {
  tft.fillCircle(x, y, 6, TFT_LIGHTGREY);

  tft.fillCircle(x - 3, y - 5, 2, TFT_WHITE);
  tft.fillCircle(x + 3, y - 5, 2, TFT_WHITE);

  tft.drawPixel(x, y, TFT_BLACK);
}

void eraseMouse(int x, int y) {
  tft.fillRect(x - 10, y - 10, 20, 20, TFT_BLACK);
}

// =====================================================
// CHIEN
// =====================================================

void drawDog(int x, int y) {
  tft.fillRoundRect(x - 14, y - 8, 28, 16, 5, TFT_BROWN);
  tft.fillCircle(x + 13, y - 8, 8, TFT_BROWN);

  tft.fillCircle(x + 16, y - 10, 2, TFT_BLACK);

  tft.drawLine(x - 14, y - 3, x - 24, y - 10, TFT_BROWN);
}

void eraseDog(int x, int y) {
  tft.fillRect(x - 28, y - 20, 52, 34, TFT_BLACK);
}

// =====================================================
// CACTUS MAGENTA PLUS PETITS
// =====================================================

void drawCactus(int x, int y) {
  uint16_t c = CACTUS_COLOR();

  tft.fillRoundRect(
    x - 3,
    y - 12,
    6,
    24,
    3,
    c
  );

  tft.fillRoundRect(
    x - 9,
    y - 4,
    5,
    12,
    2,
    c
  );

  tft.fillRoundRect(
    x + 4,
    y - 7,
    5,
    14,
    2,
    c
  );
}

void drawAllCactus() {
  for (int i = 0; i < cactusCount; i++) {
    drawCactus(
      cactusX[i],
      cactusY[i]
    );
  }
}

// =====================================================
// INTERFACE
// =====================================================

void drawGameHeader() {
  tft.fillRect(
    0,
    0,
    320,
    32,
    TFT_BLACK
  );

  tft.fillTriangle(
    11,
    16,
    31,
    5,
    31,
    27,
    TFT_WHITE
  );

  drawBoldText(
    "Chat & Souris",
    150,
    16,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  drawBoldText(
    String(scoreChat),
    300,
    16,
    TFT_GREEN,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  tft.drawLine(
    0,
    32,
    320,
    32,
    TFT_DARKGREY
  );
}

// =====================================================
// DIFFICULTE
// =====================================================

int currentMouseDelay() {
  unsigned long elapsed =
    millis() - gameStartTime;

  int d =
    650 -
    (elapsed / 4000) * 60;

  if (d < 170) {
    d = 170;
  }

  return d;
}

int currentMouseStep() {
  unsigned long elapsed =
    millis() - gameStartTime;

  int s =
    18 +
    (elapsed / 5000) * 3;

  if (s > 42) {
    s = 42;
  }

  return s;
}

// =====================================================
// POSITIONS
// =====================================================

void newMousePosition() {
  eraseMouse(mouseX, mouseY);

  mouseX = random(20, 300);
  mouseY = random(50, 220);

  drawMouse(mouseX, mouseY);
}

bool nearPoint(
  int x1,
  int y1,
  int x2,
  int y2,
  int dist2
) {
  int dx = x1 - x2;
  int dy = y1 - y2;

  return (dx * dx + dy * dy) < dist2;
}

bool catTouchMouse() {
  return nearPoint(
    catX,
    catY,
    mouseX,
    mouseY,
    520
  );
}

bool catTouchDog() {
  return nearPoint(
    catX,
    catY,
    dogX,
    dogY,
    720
  );
}

bool catTouchCactus() {
  for (int i = 0; i < cactusCount; i++) {
    if (
      nearPoint(
        catX,
        catY,
        cactusX[i],
        cactusY[i],
        520
      )
    ) {
      return true;
    }
  }

  return false;
}

void addCactus() {
  if (cactusCount >= MAX_CACTUS) {
    return;
  }

  int x = random(35, 285);
  int y = random(55, 215);

  if (nearPoint(x, y, catX, catY, 2500)) {
    return;
  }

  if (nearPoint(x, y, mouseX, mouseY, 1600)) {
    return;
  }

  if (nearPoint(x, y, dogX, dogY, 1600)) {
    return;
  }

  cactusX[cactusCount] = x;
  cactusY[cactusCount] = y;

  cactusCount++;

  drawCactus(x, y);
}

// =====================================================
// GAME OVER
// =====================================================

void drawStartButton() {
  tft.fillRoundRect(
    95,
    155,
    130,
    38,
    8,
    TFT_GREEN
  );

  tft.drawRoundRect(
    95,
    155,
    130,
    38,
    8,
    TFT_WHITE
  );

  drawBoldText(
    "START",
    160,
    174,
    TFT_BLACK,
    TFT_GREEN,
    MC_DATUM,
    2
  );
}

void showGameOver() {
  gameOver = true;

  tft.fillScreen(TFT_BLACK);

  // bouton retour
  tft.fillRect(
    0,
    0,
    45,
    36,
    TFT_WHITE
  );

  tft.fillTriangle(
    11,
    18,
    31,
    7,
    31,
    29,
    TFT_BLACK
  );

  drawBoldText(
    "GAME OVER",
    160,
    78,
    TFT_RED,
    TFT_BLACK,
    MC_DATUM,
    4
  );

  drawBoldText(
    "Score : " + String(scoreChat),
    160,
    118,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  drawStartButton();

  drawBoldText(
    "Retour",
    22,
    46,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    1
  );
}

// =====================================================
// AFFICHAGE GLOBAL
// =====================================================

void redrawGame() {
  tft.fillScreen(TFT_BLACK);

  drawGameHeader();
  drawAllCactus();
  drawMouse(mouseX, mouseY);
  drawDog(dogX, dogY);
  drawCat(catX, catY);
}

// =====================================================
// LANCEMENT
// =====================================================

void launchJeuChat() {
  appMode = MODE_JEU_CHAT;

  scoreChat = 0;
  gameOver = false;

  catX = 160;
  catY = 140;

  mouseX = random(30, 290);
  mouseY = random(55, 220);

  dogX = 280;
  dogY = 200;

  dogDX = -8;
  dogDY = -6;

  cactusCount = 0;

  gameStartTime = millis();
  lastMouseMove = millis();
  lastDogMove = millis();
  lastCactusAdd = millis();

  stopSDStartTouch();

  redrawGame();

  Serial.println("JEU CHAT PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateJeuChat() {
  int x, y;

  // GAME OVER
  if (gameOver) {
    if (getTouch(x, y)) {

      // retour menu
      if (y < 45 && x < 50) {
        drawSubMenu(currentMenu);

        while (ts.touched()) {
          delay(5);
        }

        delay(100);

        return;
      }

      // bouton START
      if (
        x >= 95 &&
        x <= 225 &&
        y >= 155 &&
        y <= 193
      ) {
        launchJeuChat();

        while (ts.touched()) {
          delay(5);
        }

        delay(100);

        return;
      }

      while (ts.touched()) {
        delay(5);
      }

      delay(80);
    }

    return;
  }

  // TOUCH NORMAL
  if (getTouch(x, y)) {
    if (y < 32 && x < 45) {
      drawSubMenu(currentMenu);

      while (ts.touched()) {
        delay(5);
      }

      delay(80);

      return;
    }

    if (y > 36) {
      eraseCat(catX, catY);

      catX = constrain(x, 24, 296);
      catY = constrain(y, 54, 218);

      drawAllCactus();
      drawMouse(mouseX, mouseY);
      drawDog(dogX, dogY);
      drawCat(catX, catY);

      if (catTouchMouse()) {
        scoreChat++;

        drawGameHeader();

        newMousePosition();

        if (scoreChat % 2 == 0) {
          addCactus();
        }
      }

      if (
        catTouchCactus() ||
        catTouchDog()
      ) {
        showGameOver();
        return;
      }
    }
  }

  // SOURIS
  if (
    millis() - lastMouseMove >
    currentMouseDelay()
  ) {
    lastMouseMove = millis();

    eraseMouse(mouseX, mouseY);

    int step = currentMouseStep();

    mouseX += random(
      -step,
      step + 1
    );

    mouseY += random(
      -step,
      step + 1
    );

    mouseX = constrain(
      mouseX,
      15,
      305
    );

    mouseY = constrain(
      mouseY,
      45,
      225
    );

    drawMouse(mouseX, mouseY);
  }

  // CHIEN
  if (millis() - lastDogMove > 120) {
    lastDogMove = millis();

    eraseDog(dogX, dogY);

    if (dogX < catX) {
      dogDX = 5;
    }

    if (dogX > catX) {
      dogDX = -5;
    }

    if (dogY < catY) {
      dogDY = 4;
    }

    if (dogY > catY) {
      dogDY = -4;
    }

    dogX += dogDX;
    dogY += dogDY;

    dogX = constrain(
      dogX,
      25,
      295
    );

    dogY = constrain(
      dogY,
      50,
      220
    );

    drawAllCactus();
    drawMouse(mouseX, mouseY);
    drawDog(dogX, dogY);
    drawCat(catX, catY);

    if (catTouchDog()) {
      showGameOver();
      return;
    }
  }

  // APPARITION CACTUS
  if (millis() - lastCactusAdd > 7000) {
    lastCactusAdd = millis();
    addCactus();
  }
}