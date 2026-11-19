#include "pixel_art.h"
#include "menu_core.h"

#define PA_MAX_GRID 32
#define PA_SAVE_DIR "/img"

#define PA_MODE_SIZE  0
#define PA_MODE_DRAW  1
#define PA_MODE_TITLE 2

#define PA_PEN         0
#define PA_LINE        1
#define PA_RECT        2
#define PA_CIRCLE      3
#define PA_FILL_CIRCLE 4

uint16_t paGrid[PA_MAX_GRID][PA_MAX_GRID];

int paMode = PA_MODE_SIZE;

int paGridSize = 16;
int paCell = 14;

const int paCanvasX = 94;
const int paCanvasY = 8;
const int paCanvasMax = 224;
int paCanvasSize = 224;

int paTool = PA_PEN;

uint16_t paCurrentColor;
uint16_t paPalette[14];
int paPaletteCount = 0;

int paR = 80;
int paG = 150;
int paB = 230;

int paStartR = -1;
int paStartC = -1;

bool paToolDrawer = false;
bool paColorDrawer = false;
bool paRgbDrawer = false;

String paTitle = "";

unsigned long paLastTouch = 0;

// =====================================================
// BASE
// =====================================================

void paDrawBack() {
  tft.fillRect(0, 0, 42, 36, TFT_WHITE);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_BLACK);
}

void paUpdateCurrentColor() {
  paCurrentColor = tft.color565(paR, paG, paB);
}

void paAddPaletteColor(uint16_t col) {
  for (int i = 0; i < paPaletteCount; i++) {
    if (paPalette[i] == col) return;
  }

  if (paPaletteCount < 14) {
    paPalette[paPaletteCount++] = col;
  } else {
    for (int i = 0; i < 13; i++) {
      paPalette[i] = paPalette[i + 1];
    }

    paPalette[13] = col;
  }
}

void paClearGrid() {
  for (int r = 0; r < PA_MAX_GRID; r++) {
    for (int c = 0; c < PA_MAX_GRID; c++) {
      paGrid[r][c] = TFT_WHITE;
    }
  }
}

void paSetGridSize(int s) {
  paGridSize = s;
  paCell = paCanvasMax / paGridSize;
  if (paCell < 1) paCell = 1;

  paCanvasSize = paCell * paGridSize;
  paClearGrid();

  paStartR = -1;
  paStartC = -1;
}

bool paTouchToCell(int x, int y, int &r, int &c) {
  if (x < paCanvasX || x >= paCanvasX + paCanvasSize) return false;
  if (y < paCanvasY || y >= paCanvasY + paCanvasSize) return false;

  c = (x - paCanvasX) / paCell;
  r = (y - paCanvasY) / paCell;

  return true;
}

void paSetPixel(int r, int c, uint16_t col) {
  if (r < 0 || r >= paGridSize || c < 0 || c >= paGridSize) return;

  paGrid[r][c] = col;

  int x = paCanvasX + c * paCell;
  int y = paCanvasY + r * paCell;

  tft.fillRect(x + 1, y + 1, paCell - 1, paCell - 1, col);
  tft.drawRect(x, y, paCell, paCell, TFT_LIGHTGREY);
}

void paDrawCanvas() {
  tft.fillRect(paCanvasX, paCanvasY, paCanvasSize, paCanvasSize, TFT_WHITE);
  tft.drawRect(paCanvasX, paCanvasY, paCanvasSize, paCanvasSize, TFT_BLACK);

  for (int r = 0; r < paGridSize; r++) {
    for (int c = 0; c < paGridSize; c++) {
      int x = paCanvasX + c * paCell;
      int y = paCanvasY + r * paCell;

      tft.fillRect(x + 1, y + 1, paCell - 1, paCell - 1, paGrid[r][c]);
      tft.drawRect(x, y, paCell, paCell, TFT_LIGHTGREY);
    }
  }
}

// =====================================================
// FORMES
// =====================================================

void paDrawLineCells(int r1, int c1, int r2, int c2, uint16_t col) {
  int dx = abs(c2 - c1);
  int dy = -abs(r2 - r1);
  int sx = c1 < c2 ? 1 : -1;
  int sy = r1 < r2 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    paSetPixel(r1, c1, col);

    if (r1 == r2 && c1 == c2) break;

    int e2 = 2 * err;

    if (e2 >= dy) {
      err += dy;
      c1 += sx;
    }

    if (e2 <= dx) {
      err += dx;
      r1 += sy;
    }
  }
}

void paDrawRectCells(int r1, int c1, int r2, int c2, uint16_t col) {
  int rMin = min(r1, r2);
  int rMax = max(r1, r2);
  int cMin = min(c1, c2);
  int cMax = max(c1, c2);

  for (int c = cMin; c <= cMax; c++) {
    paSetPixel(rMin, c, col);
    paSetPixel(rMax, c, col);
  }

  for (int r = rMin; r <= rMax; r++) {
    paSetPixel(r, cMin, col);
    paSetPixel(r, cMax, col);
  }
}

void paDrawCircleCells(int r1, int c1, int r2, int c2, uint16_t col, bool filled) {
  int radius = max(abs(r2 - r1), abs(c2 - c1));

  for (int yy = -radius; yy <= radius; yy++) {
    for (int xx = -radius; xx <= radius; xx++) {
      int d = xx * xx + yy * yy;
      int rr = radius * radius;

      if (filled) {
        if (d <= rr) paSetPixel(r1 + yy, c1 + xx, col);
      } else {
        if (d >= rr - radius && d <= rr + radius) {
          paSetPixel(r1 + yy, c1 + xx, col);
        }
      }
    }
  }
}

void paApplyCell(int r, int c) {
  paAddPaletteColor(paCurrentColor);

  if (paTool == PA_PEN) {
    paSetPixel(r, c, paCurrentColor);
    return;
  }

  if (paStartR < 0) {
    paStartR = r;
    paStartC = c;

    int x = paCanvasX + c * paCell;
    int y = paCanvasY + r * paCell;

    tft.drawRect(x + 2, y + 2, paCell - 4, paCell - 4, TFT_YELLOW);
    return;
  }

  if (paTool == PA_LINE) {
    paDrawLineCells(paStartR, paStartC, r, c, paCurrentColor);
  }

  else if (paTool == PA_RECT) {
    paDrawRectCells(paStartR, paStartC, r, c, paCurrentColor);
  }

  else if (paTool == PA_CIRCLE) {
    paDrawCircleCells(paStartR, paStartC, r, c, paCurrentColor, false);
  }

  else if (paTool == PA_FILL_CIRCLE) {
    paDrawCircleCells(paStartR, paStartC, r, c, paCurrentColor, true);
  }

  paStartR = -1;
  paStartC = -1;
}

// =====================================================
// TIROIRS / OUTILS
// =====================================================

void paDrawIconButton(int x, int y, String txt, uint16_t bg) {
  tft.fillCircle(x, y, 16, bg);
  tft.drawCircle(x, y, 16, TFT_BLACK);
  drawBoldText(txt, x, y, TFT_WHITE, bg, MC_DATUM, 2);
}

void paDrawColorDot(int x, int y, uint16_t col) {
  tft.fillCircle(x, y, 13, col);
  tft.drawCircle(x, y, 13, TFT_BLACK);
}

void paDrawLeftBar() {
  tft.fillRect(0, 0, 90, 240, TFT_WHITE);
  paDrawBack();

  paDrawIconButton(62, 18, "S", TFT_GREEN);
  paDrawIconButton(22, 58, "X", TFT_RED);
  paDrawIconButton(62, 58, "O", TFT_BLACK);
  paDrawIconButton(22, 98, "C", paCurrentColor);
  paDrawIconButton(62, 98, "R", TFT_DARKGREY);

  drawBoldText("SAVE", 62, 38, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
}

void paDrawToolDrawer() {
  tft.fillRoundRect(4, 120, 84, 112, 8, TFT_DARKGREY);
  tft.drawRoundRect(4, 120, 84, 112, 8, TFT_BLACK);

  uint16_t bg0 = paTool == PA_PEN ? TFT_BLUE : TFT_BLACK;
  uint16_t bg1 = paTool == PA_LINE ? TFT_BLUE : TFT_BLACK;
  uint16_t bg2 = paTool == PA_RECT ? TFT_BLUE : TFT_BLACK;
  uint16_t bg3 = paTool == PA_CIRCLE ? TFT_BLUE : TFT_BLACK;
  uint16_t bg4 = paTool == PA_FILL_CIRCLE ? TFT_BLUE : TFT_BLACK;

  paDrawIconButton(24, 142, "P", bg0);
  paDrawIconButton(66, 142, "/", bg1);
  paDrawIconButton(24, 184, "R", bg2);
  paDrawIconButton(66, 184, "O", bg3);
  paDrawIconButton(45, 220, "F", bg4);
}

void paDrawColorDrawer() {
  tft.fillRoundRect(4, 120, 84, 112, 8, TFT_DARKGREY);
  tft.drawRoundRect(4, 120, 84, 112, 8, TFT_BLACK);

  for (int i = 0; i < paPaletteCount; i++) {
    int x = 22 + (i % 3) * 25;
    int y = 140 + (i / 3) * 22;

    paDrawColorDot(x, y, paPalette[i]);
  }
}

void paDrawRgbDrawer() {
  tft.fillRoundRect(4, 120, 84, 112, 8, TFT_DARKGREY);
  tft.drawRoundRect(4, 120, 84, 112, 8, TFT_BLACK);

  drawBoldText("RGB", 46, 132, TFT_WHITE, TFT_DARKGREY, MC_DATUM, 1);

  tft.fillRoundRect(9, 148, 22, 22, 5, TFT_RED);
  tft.fillRoundRect(34, 148, 22, 22, 5, TFT_GREEN);
  tft.fillRoundRect(59, 148, 22, 22, 5, TFT_BLUE);

  drawBoldText("+", 20, 159, TFT_WHITE, TFT_RED, MC_DATUM, 2);
  drawBoldText("+", 45, 159, TFT_BLACK, TFT_GREEN, MC_DATUM, 2);
  drawBoldText("+", 70, 159, TFT_WHITE, TFT_BLUE, MC_DATUM, 2);

  tft.fillRoundRect(9, 176, 22, 22, 5, TFT_BLACK);
  tft.fillRoundRect(34, 176, 22, 22, 5, TFT_BLACK);
  tft.fillRoundRect(59, 176, 22, 22, 5, TFT_BLACK);

  drawBoldText("-", 20, 187, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
  drawBoldText("-", 45, 187, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
  drawBoldText("-", 70, 187, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  tft.fillRoundRect(12, 207, 68, 18, 5, paCurrentColor);
  tft.drawRoundRect(12, 207, 68, 18, 5, TFT_BLACK);
}

void paDrawUI() {
  paMode = PA_MODE_DRAW;

  tft.fillScreen(TFT_WHITE);

  paDrawCanvas();
  paDrawLeftBar();

  if (paToolDrawer) paDrawToolDrawer();
  if (paColorDrawer) paDrawColorDrawer();
  if (paRgbDrawer) paDrawRgbDrawer();
}

// =====================================================
// CHOIX GRILLE
// =====================================================

void paDrawSizeChoice() {
  paMode = PA_MODE_SIZE;

  tft.fillScreen(TFT_WHITE);
  paDrawBack();

  drawBoldText("Pixel-Art", 160, 45, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);
  drawBoldText("Choisir la grille", 160, 72, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);

  int sizes[5] = {8, 12, 16, 24, 32};

  for (int i = 0; i < 5; i++) {
    int x = 20 + i * 60;
    int y = 112;

    tft.fillRoundRect(x, y, 50, 44, 8, TFT_BLACK);
    drawBoldText(String(sizes[i]), x + 25, y + 14, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
    drawBoldText("x" + String(sizes[i]), x + 25, y + 31, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  }
}

// =====================================================
// CLAVIER TITRE
// =====================================================

void paDrawTitleInput() {
  paMode = PA_MODE_TITLE;

  tft.fillScreen(TFT_WHITE);
  paDrawBack();

  drawBoldText("Titre image", 160, 24, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);

  tft.drawRoundRect(12, 45, 296, 36, 8, TFT_BLACK);
  drawBoldText(paTitle, 160, 63, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);

  String row1 = "AZERTYUIOP";
  String row2 = "QSDFGHJKLM";
  String row3 = "WXCVBN";

  int keyW = 28;
  int keyH = 26;

  for (int i = 0; i < row1.length(); i++) {
    String k = row1.substring(i, i + 1);
    int x = 7 + i * 31;
    int y = 92;

    tft.fillRoundRect(x, y, keyW, keyH, 5, TFT_BLACK);
    drawBoldText(k, x + keyW / 2, y + keyH / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  }

  for (int i = 0; i < row2.length(); i++) {
    String k = row2.substring(i, i + 1);
    int x = 7 + i * 31;
    int y = 124;

    tft.fillRoundRect(x, y, keyW, keyH, 5, TFT_BLACK);
    drawBoldText(k, x + keyW / 2, y + keyH / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  }

  for (int i = 0; i < row3.length(); i++) {
    String k = row3.substring(i, i + 1);
    int x = 54 + i * 31;
    int y = 156;

    tft.fillRoundRect(x, y, keyW, keyH, 5, TFT_BLACK);
    drawBoldText(k, x + keyW / 2, y + keyH / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  }

  tft.fillRoundRect(12, 204, 58, 28, 6, TFT_DARKGREY);
  drawBoldText("ESP", 41, 218, TFT_WHITE, TFT_DARKGREY, MC_DATUM, 1);

  tft.fillRoundRect(82, 204, 58, 28, 6, TFT_RED);
  drawBoldText("EFF", 111, 218, TFT_WHITE, TFT_RED, MC_DATUM, 1);

  tft.fillRoundRect(152, 204, 58, 28, 6, TFT_GREEN);
  drawBoldText("OK", 181, 218, TFT_BLACK, TFT_GREEN, MC_DATUM, 1);

  tft.fillRoundRect(222, 204, 86, 28, 6, TFT_BLACK);
  drawBoldText("ANNUL", 265, 218, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
}

char paKeyboardCharAt(int x, int y) {
  String row1 = "AZERTYUIOP";
  String row2 = "QSDFGHJKLM";
  String row3 = "WXCVBN";

  if (y >= 92 && y <= 118) {
    int id = (x - 7) / 31;
    if (id >= 0 && id < row1.length()) return row1[id];
  }

  if (y >= 124 && y <= 150) {
    int id = (x - 7) / 31;
    if (id >= 0 && id < row2.length()) return row2[id];
  }

  if (y >= 156 && y <= 182) {
    int id = (x - 54) / 31;
    if (id >= 0 && id < row3.length()) return row3[id];
  }

  return 0;
}

// =====================================================
// SAUVEGARDE BMP GALERIE
// =====================================================

void paWrite16(File &f, uint16_t v) {
  f.write(v & 0xFF);
  f.write((v >> 8) & 0xFF);
}

void paWrite32(File &f, uint32_t v) {
  f.write(v & 0xFF);
  f.write((v >> 8) & 0xFF);
  f.write((v >> 16) & 0xFF);
  f.write((v >> 24) & 0xFF);
}

String paCleanFileTitle(String s) {
  s.trim();

  if (s.length() == 0) s = "pixel";

  String out = "";

  for (int i = 0; i < s.length(); i++) {
    char ch = s[i];

    if (
      (ch >= 'A' && ch <= 'Z') ||
      (ch >= 'a' && ch <= 'z') ||
      (ch >= '0' && ch <= '9')
    ) {
      out += ch;
    } else if (ch == ' ' || ch == '-' || ch == '_') {
      out += "_";
    }
  }

  if (out.length() == 0) out = "pixel";
  if (out.length() > 18) out = out.substring(0, 18);

  return out;
}

String paMakeBmpPath(String title) {
  if (!startSD()) {
    stopSDStartTouch();
    return "/img/pixel.bmp";
  }

  SD.mkdir(PA_SAVE_DIR);

  String base = paCleanFileTitle(title);
  String path = String(PA_SAVE_DIR) + "/" + base + ".bmp";

  int n = 1;

  while (SD.exists(path) && n < 999) {
    path = String(PA_SAVE_DIR) + "/" + base + "_" + String(n) + ".bmp";
    n++;
  }

  stopSDStartTouch();
  return path;
}

void paSaveBMP(String title) {
  String path = paMakeBmpPath(title);

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir(PA_SAVE_DIR);

  File f = SD.open(path, FILE_WRITE);

  if (!f) {
    stopSDStartTouch();
    return;
  }

  int outCell = 128 / paGridSize;
  if (outCell < 1) outCell = 1;

  int imgW = paGridSize * outCell;
  int imgH = paGridSize * outCell;

  int rowSize = (imgW * 3 + 3) & ~3;
  int dataSize = rowSize * imgH;
  int fileSize = 54 + dataSize;

  f.write('B');
  f.write('M');
  paWrite32(f, fileSize);
  paWrite16(f, 0);
  paWrite16(f, 0);
  paWrite32(f, 54);

  paWrite32(f, 40);
  paWrite32(f, imgW);
  paWrite32(f, imgH);
  paWrite16(f, 1);
  paWrite16(f, 24);
  paWrite32(f, 0);
  paWrite32(f, dataSize);
  paWrite32(f, 2835);
  paWrite32(f, 2835);
  paWrite32(f, 0);
  paWrite32(f, 0);

  uint8_t pad[3] = {0, 0, 0};

  for (int y = imgH - 1; y >= 0; y--) {
    int srcR = y / outCell;

    for (int x = 0; x < imgW; x++) {
      int srcC = x / outCell;

      uint16_t c = paGrid[srcR][srcC];

      uint8_t r = ((c >> 11) & 0x1F) << 3;
      uint8_t g = ((c >> 5) & 0x3F) << 2;
      uint8_t b = (c & 0x1F) << 3;

      // Compatible avec ta Galerie actuelle :
      // elle ré-inverse les BMP à l'affichage.
      f.write(255 - b);
      f.write(255 - g);
      f.write(255 - r);
    }

    f.write(pad, rowSize - imgW * 3);
  }

  f.close();
  stopSDStartTouch();

  tft.fillScreen(TFT_BLACK);
  drawBoldText("Pixel-Art sauvegarde", 160, 100, TFT_GREEN, TFT_BLACK, MC_DATUM, 2);
  drawBoldText(path, 160, 130, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  delay(1000);
  paDrawUI();
}

// =====================================================
// LANCEMENT
// =====================================================

void launchPixelArt() {
  appMode = MODE_PIXEL_ART;

  paUpdateCurrentColor();

  paPaletteCount = 0;
  paAddPaletteColor(TFT_BLACK);
  paAddPaletteColor(TFT_WHITE);
  paAddPaletteColor(tft.color565(210, 70, 70));
  paAddPaletteColor(tft.color565(70, 170, 95));
  paAddPaletteColor(tft.color565(70, 120, 210));
  paAddPaletteColor(tft.color565(235, 205, 80));

  paTool = PA_PEN;
  paToolDrawer = false;
  paColorDrawer = false;
  paRgbDrawer = false;

  stopSDStartTouch();

  paDrawSizeChoice();

  Serial.println("PIXEL ART PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updatePixelArt() {
  int x, y;

  if (!getTouch(x, y)) return;

  if (millis() - paLastTouch < 130) return;
  paLastTouch = millis();

  // ===================================================
  // RETOUR
  // ===================================================

  if (y < 36 && x < 42) {
    if (paMode == PA_MODE_SIZE) {
      drawSubMenu(currentMenu);
    } else if (paMode == PA_MODE_TITLE) {
      paDrawUI();
    } else {
      paDrawSizeChoice();
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // ===================================================
  // CHOIX TAILLE
  // ===================================================

  if (paMode == PA_MODE_SIZE) {
    if (y >= 112 && y <= 156) {
      if (x >= 20 && x <= 70) {
        paSetGridSize(8);
        paDrawUI();
      } else if (x >= 80 && x <= 130) {
        paSetGridSize(12);
        paDrawUI();
      } else if (x >= 140 && x <= 190) {
        paSetGridSize(16);
        paDrawUI();
      } else if (x >= 200 && x <= 250) {
        paSetGridSize(24);
        paDrawUI();
      } else if (x >= 260 && x <= 310) {
        paSetGridSize(32);
        paDrawUI();
      }
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // ===================================================
  // CLAVIER TITRE
  // ===================================================

  if (paMode == PA_MODE_TITLE) {
    if (x >= 12 && x <= 70 && y >= 204 && y <= 232) {
      if (paTitle.length() < 18) paTitle += " ";
      paDrawTitleInput();
    }

    else if (x >= 82 && x <= 140 && y >= 204 && y <= 232) {
      if (paTitle.length() > 0) paTitle.remove(paTitle.length() - 1);
      paDrawTitleInput();
    }

    else if (x >= 152 && x <= 210 && y >= 204 && y <= 232) {
      paSaveBMP(paTitle);
    }

    else if (x >= 222 && x <= 308 && y >= 204 && y <= 232) {
      paDrawUI();
    }

    else {
      char ch = paKeyboardCharAt(x, y);

      if (ch != 0 && paTitle.length() < 18) {
        paTitle += ch;
        paDrawTitleInput();
      }
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // ===================================================
  // MODE DESSIN
  // ===================================================

  int r, c;

  if (paTouchToCell(x, y, r, c)) {
    paApplyCell(r, c);

    while (ts.touched()) delay(5);
    delay(30);
    return;
  }

  // SAVE
  if (x >= 46 && x <= 82 && y >= 0 && y <= 36) {
    paTitle = "";
    paDrawTitleInput();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // CLEAR
  if (x >= 6 && x <= 38 && y >= 42 && y <= 74) {
    paClearGrid();
    paStartR = -1;
    paStartC = -1;
    paDrawUI();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // OUTILS
  if (x >= 46 && x <= 82 && y >= 42 && y <= 74) {
    paToolDrawer = !paToolDrawer;
    paColorDrawer = false;
    paRgbDrawer = false;
    paDrawUI();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // COULEURS
  if (x >= 6 && x <= 38 && y >= 82 && y <= 114) {
    paColorDrawer = !paColorDrawer;
    paToolDrawer = false;
    paRgbDrawer = false;
    paDrawUI();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // RGB
  if (x >= 46 && x <= 82 && y >= 82 && y <= 114) {
    paRgbDrawer = !paRgbDrawer;
    paToolDrawer = false;
    paColorDrawer = false;
    paDrawUI();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // ===================================================
  // TIROIR OUTILS
  // ===================================================

  if (paToolDrawer && x >= 4 && x <= 88 && y >= 120 && y <= 232) {
    if (x >= 8 && x <= 40 && y >= 126 && y <= 158) paTool = PA_PEN;
    else if (x >= 50 && x <= 82 && y >= 126 && y <= 158) paTool = PA_LINE;
    else if (x >= 8 && x <= 40 && y >= 168 && y <= 200) paTool = PA_RECT;
    else if (x >= 50 && x <= 82 && y >= 168 && y <= 200) paTool = PA_CIRCLE;
    else if (x >= 29 && x <= 61 && y >= 204 && y <= 236) paTool = PA_FILL_CIRCLE;

    paStartR = -1;
    paStartC = -1;
    paDrawUI();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // ===================================================
  // TIROIR COULEURS
  // ===================================================

  if (paColorDrawer && x >= 4 && x <= 88 && y >= 120 && y <= 232) {
    int col = (x - 9) / 25;
    int row = (y - 127) / 22;
    int id = row * 3 + col;

    if (id >= 0 && id < paPaletteCount) {
      paCurrentColor = paPalette[id];
      paDrawUI();
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // ===================================================
  // TIROIR RGB
  // ===================================================

  if (paRgbDrawer && x >= 4 && x <= 88 && y >= 120 && y <= 232) {
    if (y >= 148 && y <= 170) {
      if (x >= 9 && x <= 31) paR = min(255, paR + 20);
      if (x >= 34 && x <= 56) paG = min(255, paG + 20);
      if (x >= 59 && x <= 81) paB = min(255, paB + 20);
    }

    else if (y >= 176 && y <= 198) {
      if (x >= 9 && x <= 31) paR = max(0, paR - 20);
      if (x >= 34 && x <= 56) paG = max(0, paG - 20);
      if (x >= 59 && x <= 81) paB = max(0, paB - 20);
    }

    paUpdateCurrentColor();
    paAddPaletteColor(paCurrentColor);
    paDrawUI();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  while (ts.touched()) delay(5);
  delay(80);
}