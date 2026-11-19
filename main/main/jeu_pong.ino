#include "jeu_pong.h"
#include "menu_core.h"

enum PongState {
  PONG_MENU,
  PONG_PLAY,
  PONG_GAME_OVER
};

PongState pongState = PONG_MENU;

bool pongTwoPlayers = false;

float ballX = 160;
float ballY = 120;
float ballVX = 3.0;
float ballVY = 2.2;

int paddleL_Y = 95;
int paddleR_Y = 95;

const int paddleW = 8;
const int paddleH = 50;

int scoreL = 0;
int scoreR = 0;

unsigned long pongGameStartTime = 0;
unsigned long lastPongFrame = 0;

void pongDrawBackTriangle() {
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);
}

void pongDrawMenu() {
  pongState = PONG_MENU;

  tft.fillScreen(TFT_BLACK);

  pongDrawBackTriangle();

  drawBoldText(
    "PONG",
    160,
    55,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    4
  );

  tft.fillRoundRect(55, 95, 210, 42, 8, TFT_WHITE);
  tft.drawRoundRect(55, 95, 210, 42, 8, TFT_BLACK);

  drawBoldText(
    "1 JOUEUR",
    160,
    116,
    TFT_BLACK,
    TFT_WHITE,
    MC_DATUM,
    2
  );

  tft.fillRoundRect(55, 155, 210, 42, 8, TFT_WHITE);
  tft.drawRoundRect(55, 155, 210, 42, 8, TFT_BLACK);

  drawBoldText(
    "2 JOUEURS",
    160,
    176,
    TFT_BLACK,
    TFT_WHITE,
    MC_DATUM,
    2
  );
}

void pongDrawHeader() {
  tft.fillRect(0, 0, 320, 24, TFT_BLACK);

  drawBoldText(
    String(scoreL),
    120,
    13,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  drawBoldText(
    String(scoreR),
    200,
    13,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  tft.drawLine(0, 24, 320, 24, TFT_DARKGREY);
}

void pongDrawCenterLine() {
  for (int y = 30; y < 240; y += 16) {
    tft.drawFastVLine(160, y, 8, TFT_DARKGREY);
  }
}

void pongEraseBall() {
  tft.fillCircle((int)ballX, (int)ballY, 5, TFT_BLACK);
}

void pongDrawBall() {
  tft.fillCircle((int)ballX, (int)ballY, 5, TFT_WHITE);
}

void pongErasePaddles() {
  tft.fillRect(8, paddleL_Y - 2, paddleW + 2, paddleH + 4, TFT_BLACK);
  tft.fillRect(304, paddleR_Y - 2, paddleW + 2, paddleH + 4, TFT_BLACK);
}

void pongDrawPaddles() {
  tft.fillRoundRect(8, paddleL_Y, paddleW, paddleH, 3, TFT_WHITE);
  tft.fillRoundRect(304, paddleR_Y, paddleW, paddleH, 3, TFT_WHITE);
}

float pongSpeed() {
  int totalScore = scoreL + scoreR;

  unsigned long elapsed = millis() - pongGameStartTime;
  int timeLevel = elapsed / 6000;

  float s = 3.0 + totalScore * 0.25 + timeLevel * 0.45;

  if (s > 10.0) {
    s = 10.0;
  }

  return s;
}

void pongResetBall(int direction) {
  ballX = 160;
  ballY = 120;

  float s = pongSpeed();

  ballVX = s * direction;

  ballVY = random(0, 2) == 0 ? 2.2 : -2.2;

  if (scoreL + scoreR > 5) {
    ballVY *= 1.3;
  }

  delay(350);
}

void pongDrawGame() {
  tft.fillScreen(TFT_BLACK);

  pongDrawHeader();
  pongDrawCenterLine();
  pongDrawPaddles();
  pongDrawBall();
}

void pongStartGame(bool twoPlayers) {
  pongTwoPlayers = twoPlayers;
  pongState = PONG_PLAY;

  pongGameStartTime = millis();
  
  scoreL = 0;
  scoreR = 0;

  paddleL_Y = 95;
  paddleR_Y = 95;

  ballX = 160;
  ballY = 120;

  ballVX = 3.0;
  ballVY = 2.2;

  pongDrawGame();
  
  lastPongFrame = millis();
}

void pongGameOver() {
  pongState = PONG_GAME_OVER;

  tft.fillScreen(TFT_BLACK);

  pongDrawBackTriangle();

  drawBoldText(
    "GAME OVER",
    160,
    75,
    TFT_RED,
    TFT_BLACK,
    MC_DATUM,
    4
  );

  String result = "";

  if (scoreL > scoreR) {
    result = "Gauche gagne";
  } else {
    result = "Droite gagne";
  }

  drawBoldText(
    result,
    160,
    118,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  drawBoldText(
    String(scoreL) + " - " + String(scoreR),
    160,
    145,
    TFT_GREEN,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  tft.fillRoundRect(95, 175, 130, 38, 8, TFT_GREEN);
  tft.drawRoundRect(95, 175, 130, 38, 8, TFT_WHITE);

  drawBoldText(
    "START",
    160,
    194,
    TFT_BLACK,
    TFT_GREEN,
    MC_DATUM,
    2
  );
}

void pongHandleTouchGame() {
  int x, y;

  if (!getTouch(x, y)) {
    return;
  }

  if (y < 30) {
    return;
  }

  if (pongTwoPlayers) {
    if (x < 160) {
      paddleL_Y = constrain(y - paddleH / 2, 28, 190);
    } else {
      paddleR_Y = constrain(y - paddleH / 2, 28, 190);
    }
  } else {
    paddleL_Y = constrain(y - paddleH / 2, 28, 190);
  }
}

void pongMoveAI() {
  if (pongTwoPlayers) {
    return;
  }

  int center = paddleR_Y + paddleH / 2;

  int totalScore = scoreL + scoreR;
  int aiSpeed = 3 + totalScore / 2;

  if (aiSpeed > 8) {
    aiSpeed = 8;
  }

  if (center < ballY - 6) {
    paddleR_Y += aiSpeed;
  } else if (center > ballY + 6) {
    paddleR_Y -= aiSpeed;
  }

  paddleR_Y = constrain(paddleR_Y, 28, 190);
}

void pongUpdateBall() {
  pongEraseBall();

  ballX += ballVX;
  ballY += ballVY;

  if (ballY <= 30) {
    ballY = 30;
    ballVY = -ballVY;
  }

  if (ballY >= 235) {
    ballY = 235;
    ballVY = -ballVY;
  }

  // raquette gauche
  if (
    ballX <= 20 &&
    ballX >= 8 &&
    ballY >= paddleL_Y &&
    ballY <= paddleL_Y + paddleH
  ) {
    ballX = 21;
    ballVX = pongSpeed();

    int impact = ballY - (paddleL_Y + paddleH / 2);
    ballVY = impact * 0.12;
  }

  // raquette droite
  if (
    ballX >= 299 &&
    ballX <= 312 &&
    ballY >= paddleR_Y &&
    ballY <= paddleR_Y + paddleH
  ) {
    ballX = 298;
    ballVX = -pongSpeed();

    int impact = ballY - (paddleR_Y + paddleH / 2);
    ballVY = impact * 0.12;
  }

  if (ballX < 0) {
    scoreR++;
    pongDrawHeader();

    if (scoreR >= 7) {
      pongGameOver();
      return;
    }

    pongResetBall(1);
  }

  if (ballX > 320) {
    scoreL++;
    pongDrawHeader();

    if (scoreL >= 7) {
      pongGameOver();
      return;
    }

    pongResetBall(-1);
  }

  pongDrawBall();
}

void pongFrame() {
  if (millis() - lastPongFrame < 22) {
    return;
  }

  lastPongFrame = millis();

  pongErasePaddles();

  pongHandleTouchGame();
  pongMoveAI();

  pongDrawPaddles();
  pongUpdateBall();
  pongDrawCenterLine();
}

void launchJeuPong() {
  appMode = MODE_JEU_PONG;

  stopSDStartTouch();

  pongDrawMenu();

  Serial.println("PONG PRET");
}

void updateJeuPong() {
  int x, y;

  if (pongState == PONG_MENU) {
    if (getTouch(x, y)) {
      if (y < 36 && x < 45) {
        drawSubMenu(currentMenu);

        while (ts.touched()) delay(5);
        delay(80);
        return;
      }

      if (x >= 55 && x <= 265 && y >= 95 && y <= 137) {
        pongStartGame(false);

        while (ts.touched()) delay(5);
        delay(100);
        return;
      }

      if (x >= 55 && x <= 265 && y >= 155 && y <= 197) {
        pongStartGame(true);

        while (ts.touched()) delay(5);
        delay(100);
        return;
      }
    }

    return;
  }

  if (pongState == PONG_PLAY) {
    pongFrame();
    return;
  }

  if (pongState == PONG_GAME_OVER) {
    if (getTouch(x, y)) {
      if (y < 36 && x < 45) {
        pongDrawMenu();

        while (ts.touched()) delay(5);
        delay(80);
        return;
      }

      if (x >= 95 && x <= 225 && y >= 175 && y <= 213) {
        pongDrawMenu();

        while (ts.touched()) delay(5);
        delay(80);
        return;
      }
    }
  }
}