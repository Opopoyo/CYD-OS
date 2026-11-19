#include "dame.h"
#include "menu_core.h"


#define DAME_EMPTY 0
#define DAME_WHITE 1
#define DAME_WKING 2
#define DAME_BLACK -1
#define DAME_BKING -2

int dameBoard[8][8];

int dameMode = 0; // 0 menu, 1 solo, 2 joueurs
int dameTurn = DAME_WHITE;

int dameSelR = -1;
int dameSelC = -1;

unsigned long dameLastTouch = 0;
bool dameGameOver = false;

const int DAME_CELL = 28;
const int DAME_X = 48;
const int DAME_Y = 10;

void dameDrawMenu();
void dameDrawBoard();

bool dameIsWhite(int p) { return p > 0; }
bool dameIsBlack(int p) { return p < 0; }
bool dameIsKing(int p) { return abs(p) == 2; }

bool dameInBounds(int r, int c) {
  return r >= 0 && r < 8 && c >= 0 && c < 8;
}

bool dameOwnPiece(int p, int turn) {
  if (turn == DAME_WHITE) return dameIsWhite(p);
  return dameIsBlack(p);
}

bool dameEnemyPiece(int p, int turn) {
  if (turn == DAME_WHITE) return dameIsBlack(p);
  return dameIsWhite(p);
}

void dameDrawBack() {
  tft.fillRect(0, 0, 42, 36, TFT_WHITE);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_BLACK);
}

void dameInitBoard() {
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      dameBoard[r][c] = DAME_EMPTY;
    }
  }

  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 8; c++) {
      if ((r + c) % 2 == 1) dameBoard[r][c] = DAME_BLACK;
    }
  }

  for (int r = 5; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if ((r + c) % 2 == 1) dameBoard[r][c] = DAME_WHITE;
    }
  }

  dameTurn = DAME_WHITE;
  dameSelR = -1;
  dameSelC = -1;
  dameGameOver = false;
}

void dameDrawMenuButton(int x, int y, int w, int h, String txt) {
  tft.fillRoundRect(x, y, w, h, 10, TFT_BLACK);
  tft.drawRoundRect(x, y, w, h, 10, TFT_DARKGREY);
  drawBoldText(txt, x + w / 2, y + h / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

void dameDrawMenu() {
  dameMode = 0;
  dameSelR = -1;
  dameSelC = -1;

  tft.fillScreen(TFT_WHITE);
  dameDrawBack();

  drawBoldText("Jeu de dames", 160, 58, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);

  dameDrawMenuButton(50, 98, 220, 44, "1 joueur");
  dameDrawMenuButton(50, 158, 220, 44, "2 joueurs");
}

void dameDrawPiece(int r, int c, int p) {
  if (p == DAME_EMPTY) return;

  int x = DAME_X + c * DAME_CELL + DAME_CELL / 2;
  int y = DAME_Y + r * DAME_CELL + DAME_CELL / 2;

  uint16_t col = dameIsWhite(p) ? TFT_WHITE : TFT_BLACK;
  uint16_t edge = dameIsWhite(p) ? TFT_BLACK : TFT_WHITE;

  tft.fillCircle(x, y, 11, col);
  tft.drawCircle(x, y, 11, edge);
  tft.drawCircle(x, y, 10, edge);

  if (dameIsKing(p)) {
    drawBoldText("D", x, y, edge, col, MC_DATUM, 2);
  }
}

void dameDrawBoard() {
  tft.fillScreen(TFT_WHITE);
  dameDrawBack();

  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      int x = DAME_X + c * DAME_CELL;
      int y = DAME_Y + r * DAME_CELL;

      uint16_t sq = ((r + c) % 2 == 0) ? TFT_LIGHTGREY : TFT_DARKGREY;

      tft.fillRect(x, y, DAME_CELL, DAME_CELL, sq);
      tft.drawRect(x, y, DAME_CELL, DAME_CELL, TFT_BLACK);

      if (r == dameSelR && c == dameSelC) {
        tft.drawRect(x + 2, y + 2, DAME_CELL - 4, DAME_CELL - 4, TFT_YELLOW);
        tft.drawRect(x + 3, y + 3, DAME_CELL - 6, DAME_CELL - 6, TFT_YELLOW);
      }

      dameDrawPiece(r, c, dameBoard[r][c]);
    }
  }

  String turnTxt = dameTurn == DAME_WHITE ? "Blanc" : "Noir";
  drawBoldText("Tour: " + turnTxt, 160, 235, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);

  if (dameGameOver) {
    tft.fillRoundRect(43, 84, 234, 72, 10, TFT_BLACK);
    drawBoldText("FIN DE PARTIE", 160, 105, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
    drawBoldText("Retour = menu", 160, 132, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
    dameDrawBack();
  }
}

bool dameClearPath(int r1, int c1, int r2, int c2) {
  int dr = r2 > r1 ? 1 : -1;
  int dc = c2 > c1 ? 1 : -1;

  int r = r1 + dr;
  int c = c1 + dc;

  while (r != r2 && c != c2) {
    if (dameBoard[r][c] != DAME_EMPTY) return false;
    r += dr;
    c += dc;
  }

  return true;
}

bool dameKingCaptureInfo(int r1, int c1, int r2, int c2, int &capR, int &capC) {
  capR = -1;
  capC = -1;

  int p = dameBoard[r1][c1];

  int dr = r2 > r1 ? 1 : -1;
  int dc = c2 > c1 ? 1 : -1;

  int enemies = 0;

  int r = r1 + dr;
  int c = c1 + dc;

  while (r != r2 && c != c2) {
    int q = dameBoard[r][c];

    if (q != DAME_EMPTY) {
      if (dameOwnPiece(q, dameTurn)) return false;
      if (dameEnemyPiece(q, dameTurn)) {
        enemies++;
        capR = r;
        capC = c;
      }
    }

    r += dr;
    c += dc;
  }

  return enemies == 1;
}

bool damePieceCanCapture(int r, int c) {
  int p = dameBoard[r][c];
  if (p == DAME_EMPTY) return false;

  if (dameIsKing(p)) {
    for (int r2 = 0; r2 < 8; r2++) {
      for (int c2 = 0; c2 < 8; c2++) {
        if (dameBoard[r2][c2] != DAME_EMPTY) continue;
        if (abs(r2 - r) != abs(c2 - c)) continue;

        int cr, cc;
        if (dameKingCaptureInfo(r, c, r2, c2, cr, cc)) return true;
      }
    }

    return false;
  }

  int dirs[4][2] = {
    {-1, -1}, {-1, 1},
    {1, -1}, {1, 1}
  };

  for (int i = 0; i < 4; i++) {
    int dr = dirs[i][0];
    int dc = dirs[i][1];

    int r1 = r + dr;
    int c1 = c + dc;
    int r2 = r + dr * 2;
    int c2 = c + dc * 2;

    if (!dameInBounds(r2, c2)) continue;

    if (dameEnemyPiece(dameBoard[r1][c1], p > 0 ? DAME_WHITE : DAME_BLACK) &&
        dameBoard[r2][c2] == DAME_EMPTY) {
      return true;
    }
  }

  return false;
}

bool dameTryMove(int r1, int c1, int r2, int c2) {
  if (!dameInBounds(r1, c1) || !dameInBounds(r2, c2)) return false;
  if (dameBoard[r2][c2] != DAME_EMPTY) return false;

  int p = dameBoard[r1][c1];

  if (!dameOwnPiece(p, dameTurn)) return false;

  int dr = r2 - r1;
  int dc = c2 - c1;

  if (abs(dr) != abs(dc)) return false;

  bool captured = false;

  // Dame : déplacement libre sur diagonale + capture longue.
  if (dameIsKing(p)) {
    int capR, capC;

    if (dameKingCaptureInfo(r1, c1, r2, c2, capR, capC)) {
      dameBoard[capR][capC] = DAME_EMPTY;
      captured = true;
    } else {
      if (!dameClearPath(r1, c1, r2, c2)) return false;
    }

    dameBoard[r2][c2] = p;
    dameBoard[r1][c1] = DAME_EMPTY;
  }

  // Pion normal : avance simple ou capture diagonale de 2 cases.
  else {
    if (abs(dr) == 1 && abs(dc) == 1) {
      if (p == DAME_WHITE && dr != -1) return false;
      if (p == DAME_BLACK && dr != 1) return false;

      dameBoard[r2][c2] = p;
      dameBoard[r1][c1] = DAME_EMPTY;
    }

    else if (abs(dr) == 2 && abs(dc) == 2) {
      int mr = (r1 + r2) / 2;
      int mc = (c1 + c2) / 2;

      if (!dameEnemyPiece(dameBoard[mr][mc], dameTurn)) return false;

      dameBoard[r2][c2] = p;
      dameBoard[r1][c1] = DAME_EMPTY;
      dameBoard[mr][mc] = DAME_EMPTY;

      captured = true;
    }

    else {
      return false;
    }
  }

  if (dameBoard[r2][c2] == DAME_WHITE && r2 == 0) dameBoard[r2][c2] = DAME_WKING;
  if (dameBoard[r2][c2] == DAME_BLACK && r2 == 7) dameBoard[r2][c2] = DAME_BKING;

  if (captured && damePieceCanCapture(r2, c2)) {
    dameSelR = r2;
    dameSelC = c2;
  } else {
    dameSelR = -1;
    dameSelC = -1;
    dameTurn = -dameTurn;
  }

  return true;
}

int dameCountPieces(int turn) {
  int n = 0;

  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (dameOwnPiece(dameBoard[r][c], turn)) n++;
    }
  }

  return n;
}

bool dameHasAnyMove(int turn) {
  int oldTurn = dameTurn;
  dameTurn = turn;

  for (int r1 = 0; r1 < 8; r1++) {
    for (int c1 = 0; c1 < 8; c1++) {
      if (!dameOwnPiece(dameBoard[r1][c1], turn)) continue;

      for (int r2 = 0; r2 < 8; r2++) {
        for (int c2 = 0; c2 < 8; c2++) {
          if (!dameInBounds(r2, c2)) continue;
          if (dameBoard[r2][c2] != DAME_EMPTY) continue;
          if (abs(r2 - r1) != abs(c2 - c1)) continue;

          int p = dameBoard[r1][c1];

          if (dameIsKing(p)) {
            int cr, cc;
            if (dameKingCaptureInfo(r1, c1, r2, c2, cr, cc) || dameClearPath(r1, c1, r2, c2)) {
              dameTurn = oldTurn;
              return true;
            }
          } else {
            int dr = r2 - r1;
            int dc = c2 - c1;

            if (abs(dr) == 1 && abs(dc) == 1) {
              if ((p == DAME_WHITE && dr == -1) || (p == DAME_BLACK && dr == 1)) {
                dameTurn = oldTurn;
                return true;
              }
            }

            if (abs(dr) == 2 && abs(dc) == 2) {
              int mr = (r1 + r2) / 2;
              int mc = (c1 + c2) / 2;

              if (dameEnemyPiece(dameBoard[mr][mc], turn)) {
                dameTurn = oldTurn;
                return true;
              }
            }
          }
        }
      }
    }
  }

  dameTurn = oldTurn;
  return false;
}

void dameCheckGameOver() {
  if (dameCountPieces(DAME_WHITE) == 0 || dameCountPieces(DAME_BLACK) == 0) {
    dameGameOver = true;
    return;
  }

  if (!dameHasAnyMove(dameTurn)) {
    dameGameOver = true;
  }
}

void dameAI() {
  if (dameMode != 1 || dameTurn != DAME_BLACK || dameGameOver) return;

  delay(180);

  int r1List[80], c1List[80], r2List[80], c2List[80];
  int scoreList[80];
  int count = 0;

  for (int r1 = 0; r1 < 8; r1++) {
    for (int c1 = 0; c1 < 8; c1++) {
      int p = dameBoard[r1][c1];

      if (!dameIsBlack(p)) continue;

      for (int r2 = 0; r2 < 8; r2++) {
        for (int c2 = 0; c2 < 8; c2++) {
          if (dameBoard[r2][c2] != DAME_EMPTY) continue;
          if (abs(r2 - r1) != abs(c2 - c1)) continue;

          int score = 1;

          if (dameIsKing(p)) {
            int cr, cc;

            if (dameKingCaptureInfo(r1, c1, r2, c2, cr, cc)) {
              score = 10;
            } else if (!dameClearPath(r1, c1, r2, c2)) {
              continue;
            }
          } else {
            int dr = r2 - r1;
            int dc = c2 - c1;

            if (abs(dr) == 1 && abs(dc) == 1) {
              if (dr != 1) continue;
              score = 1;
            } else if (abs(dr) == 2 && abs(dc) == 2) {
              int mr = (r1 + r2) / 2;
              int mc = (c1 + c2) / 2;

              if (!dameIsWhite(dameBoard[mr][mc])) continue;
              score = 10;
            } else {
              continue;
            }
          }

          if (count < 80) {
            r1List[count] = r1;
            c1List[count] = c1;
            r2List[count] = r2;
            c2List[count] = c2;
            scoreList[count] = score;
            count++;
          }
        }
      }
    }
  }

  if (count == 0) {
    dameGameOver = true;
    dameDrawBoard();
    return;
  }

  int bestScore = -1;

  for (int i = 0; i < count; i++) {
    if (scoreList[i] > bestScore) bestScore = scoreList[i];
  }

  int candidates[80];
  int candidateCount = 0;

  for (int i = 0; i < count; i++) {
    if (scoreList[i] == bestScore) {
      candidates[candidateCount++] = i;
    }
  }

  int choice = candidates[random(candidateCount)];

  dameSelR = r1List[choice];
  dameSelC = c1List[choice];

  dameTryMove(r1List[choice], c1List[choice], r2List[choice], c2List[choice]);

  dameCheckGameOver();
  dameDrawBoard();
}

void launchDame() {
  appMode = MODE_DAME;
  dameDrawMenu();
}

void updateDame() {
  int x, y;

  if (getTouch(x, y)) {
    if (millis() - dameLastTouch < 180) return;
    dameLastTouch = millis();

    // Retour toujours actif, même fin de partie et même mode 1 joueur.
    if (y < 36 && x < 45) {
      if (dameMode == 0) {
        drawSubMenu(currentMenu);
      } else {
        dameDrawMenu();
      }

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    if (dameMode == 0) {
      if (x >= 50 && x <= 270 && y >= 98 && y <= 142) {
        dameMode = 1;
        dameInitBoard();
        dameDrawBoard();
      }

      else if (x >= 50 && x <= 270 && y >= 158 && y <= 202) {
        dameMode = 2;
        dameInitBoard();
        dameDrawBoard();
      }

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    if (dameGameOver) {
      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    int c = (x - DAME_X) / DAME_CELL;
    int r = (y - DAME_Y) / DAME_CELL;

    if (!dameInBounds(r, c)) return;

    int p = dameBoard[r][c];

    if (dameSelR < 0) {
      if (dameOwnPiece(p, dameTurn)) {
        dameSelR = r;
        dameSelC = c;
        dameDrawBoard();
      }
    } else {
      if (r == dameSelR && c == dameSelC) {
        dameSelR = -1;
        dameSelC = -1;
        dameDrawBoard();
      }

      else if (dameOwnPiece(p, dameTurn)) {
        dameSelR = r;
        dameSelC = c;
        dameDrawBoard();
      }

      else {
        if (dameTryMove(dameSelR, dameSelC, r, c)) {
          dameCheckGameOver();
          dameDrawBoard();
        }
      }
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (dameMode == 1 && dameTurn == DAME_BLACK && !dameGameOver) {
    dameAI();
  }
}