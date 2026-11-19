#include "peinture.h"
#include "menu_core.h"


#define PAINT_SCREEN_W 320
#define PAINT_SCREEN_H 212
#define PAINT_Y 28

#define PAINT_W 160
#define PAINT_H 106
#define PAINT_SCALE 2



uint16_t paintCanvas[PAINT_H][PAINT_W];

uint16_t paintColor;
uint16_t oldPaintColor;

int brushSize = 3;
int brushShape = 0; // 0 rond, 1 carre, 2 ligne, 3 croix
int brushPattern = 0; // 0 plein, 1 pointille, 2 trame

bool toolsOpen = false;
bool paletteOpen = false;
bool eraserMode = false;

// =====================================================
// COULEURS CORRIGEES
// =====================================================

uint16_t C_BG()      { return tft.color565(255, 255, 255); }
uint16_t C_PEN()     { return tft.color565(35, 35, 35); }
uint16_t C_RED()     { return tft.color565(210, 70, 70); }
uint16_t C_GREEN()   { return tft.color565(70, 170, 95); }
uint16_t C_BLUE()    { return tft.color565(70, 120, 210); }
uint16_t C_YELLOW()  { return tft.color565(235, 205, 80); }
uint16_t C_ORANGE()  { return tft.color565(230, 145, 65); }
uint16_t C_PINK()    { return tft.color565(230, 120, 170); }
uint16_t C_PURPLE()  { return tft.color565(150, 105, 210); }
uint16_t C_CYAN()    { return tft.color565(90, 190, 200); }
uint16_t C_BROWN()   { return tft.color565(145, 95, 55); }
uint16_t C_GREY()    { return tft.color565(150, 150, 150); }

// =====================================================
// MEMOIRE DESSIN
// =====================================================
void restoreCanvasArea(int y1, int y2);

void paintClearMemory(uint16_t color) {
  for (int y = 0; y < PAINT_H; y++) {
    for (int x = 0; x < PAINT_W; x++) {
      paintCanvas[y][x] = color;
    }
  }
}

void paintSetPixelMemory(int sx, int sy, uint16_t color) {
  int mx = sx / PAINT_SCALE;
  int my = (sy - PAINT_Y) / PAINT_SCALE;

  if (mx >= 0 && mx < PAINT_W && my >= 0 && my < PAINT_H) {
    paintCanvas[my][mx] = color;
  }
}

bool paintPatternOk(int x, int y) {
  if (brushPattern == 0) return true;
  if (brushPattern == 1) return ((x + y) % 4 == 0);
  if (brushPattern == 2) return ((x % 6 == 0) || (y % 6 == 0));
  return true;
}

void paintDrawBrushMemory(int sx, int sy, int r, uint16_t color) {
  int sx2 = sx / PAINT_SCALE;
  int cy = (sy - PAINT_Y) / PAINT_SCALE;
  int r2 = max(1, r / PAINT_SCALE);

  for (int yy = -r2; yy <= r2; yy++) {
    for (int xx = -r2; xx <= r2; xx++) {
      bool draw = false;

      if (brushShape == 0) {
        draw = (xx * xx + yy * yy <= r2 * r2);
      }
      else if (brushShape == 1) {
        draw = true;
      }
      else if (brushShape == 2) {
        draw = abs(yy) <= 1;
      }
      else if (brushShape == 3) {
        draw = abs(xx) <= 1 || abs(yy) <= 1;
      }

      if (draw) {
        int px = sx2 + xx;
        int py = cy + yy;

        if (
          px >= 0 &&
          px < PAINT_W &&
          py >= 0 &&
          py < PAINT_H &&
          paintPatternOk(px, py)
        ) {
          paintCanvas[py][px] = color;
        }
      }
    }
  }
}

// =====================================================
// DESSIN ECRAN
// =====================================================

void paintDrawBrushScreen(int x, int y) {
  if (brushShape == 0) {
    if (brushPattern == 0) {
      tft.fillCircle(x, y, brushSize, paintColor);
    } else {
      for (int yy = -brushSize; yy <= brushSize; yy++) {
        for (int xx = -brushSize; xx <= brushSize; xx++) {
          if (
            xx * xx + yy * yy <= brushSize * brushSize &&
            paintPatternOk(x + xx, y + yy)
          ) {
            tft.drawPixel(x + xx, y + yy, paintColor);
          }
        }
      }
    }
  }

  else if (brushShape == 1) {
    if (brushPattern == 0) {
      tft.fillRect(x - brushSize, y - brushSize, brushSize * 2, brushSize * 2, paintColor);
    } else {
      for (int yy = -brushSize; yy <= brushSize; yy++) {
        for (int xx = -brushSize; xx <= brushSize; xx++) {
          if (paintPatternOk(x + xx, y + yy)) {
            tft.drawPixel(x + xx, y + yy, paintColor);
          }
        }
      }
    }
  }

  else if (brushShape == 2) {
    tft.drawFastHLine(x - brushSize * 2, y, brushSize * 4, paintColor);
    tft.drawFastHLine(x - brushSize * 2, y + 1, brushSize * 4, paintColor);
  }

  else if (brushShape == 3) {
    tft.drawFastHLine(x - brushSize, y, brushSize * 2, paintColor);
    tft.drawFastVLine(x, y - brushSize, brushSize * 2, paintColor);
  }
}

void paintDrawPoint(int x, int y) {
  if (y < PAINT_Y) return;

  paintDrawBrushMemory(x, y, brushSize, paintColor);
  paintDrawBrushScreen(x, y);
}

// =====================================================
// BARRE HAUT
// =====================================================

void drawToolCircle() {
  tft.fillCircle(302, 14, 12, TFT_WHITE);
  tft.drawCircle(302, 14, 12, TFT_BLACK);
  drawBoldText("O", 302, 14, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
}

void drawColorWitness() {
  tft.fillCircle(155, 14, 11, paintColor);
  tft.drawCircle(155, 14, 12, TFT_WHITE);
  tft.drawCircle(155, 14, 13, TFT_BLACK);
}

void drawMiniTopBar() {
  tft.fillRect(0, 0, 320, PAINT_Y, TFT_BLACK);

  tft.fillTriangle(8, 14, 25, 5, 25, 23, TFT_WHITE);

  tft.fillRoundRect(42, 4, 45, 20, 5, TFT_GREEN);
  drawBoldText("SAVE", 64, 14, TFT_BLACK, TFT_GREEN, MC_DATUM, 1);

  tft.fillRoundRect(94, 4, 38, 20, 5, TFT_WHITE);
  drawBoldText("CLR", 113, 14, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);

  drawColorWitness();
  drawToolCircle();
}

// =====================================================
// PALETTE COULEURS
// =====================================================

void drawColorButton(int x, int y, uint16_t color) {
  tft.fillCircle(x, y, 12, color);
  tft.drawCircle(x, y, 12, TFT_BLACK);
  tft.drawCircle(x, y, 13, TFT_WHITE);
}

void drawPalettePanel() {
  paletteOpen = true;
  toolsOpen = false;

  tft.fillRoundRect(10, 32, 300, 80, 8, TFT_DARKGREY);
  tft.drawRoundRect(10, 32, 300, 80, 8, TFT_WHITE);

  drawColorButton(32, 55, C_PEN());
  drawColorButton(72, 55, C_RED());
  drawColorButton(112, 55, C_GREEN());
  drawColorButton(152, 55, C_BLUE());
  drawColorButton(192, 55, C_YELLOW());
  drawColorButton(232, 55, C_ORANGE());
  drawColorButton(272, 55, C_PINK());

  drawColorButton(52, 88, C_PURPLE());
  drawColorButton(92, 88, C_CYAN());
  drawColorButton(132, 88, C_BROWN());
  drawColorButton(172, 88, C_GREY());
  drawColorButton(212, 88, C_BG());

  drawBoldText("GOM", 260, 88, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
  tft.drawRoundRect(238, 75, 45, 26, 6, TFT_WHITE);
}

void handlePaletteTouch(int x, int y) {
  if (y >= 40 && y <= 68) {
    if (x >= 20 && x <= 44)  { paintColor = C_PEN(); eraserMode = false; }
    if (x >= 60 && x <= 84)  { paintColor = C_RED(); eraserMode = false; }
    if (x >= 100 && x <= 124){ paintColor = C_GREEN(); eraserMode = false; }
    if (x >= 140 && x <= 164){ paintColor = C_BLUE(); eraserMode = false; }
    if (x >= 180 && x <= 204){ paintColor = C_YELLOW(); eraserMode = false; }
    if (x >= 220 && x <= 244){ paintColor = C_ORANGE(); eraserMode = false; }
    if (x >= 260 && x <= 284){ paintColor = C_PINK(); eraserMode = false; }
  }

  if (y >= 73 && y <= 103) {
    if (x >= 40 && x <= 64)  { paintColor = C_PURPLE(); eraserMode = false; }
    if (x >= 80 && x <= 104) { paintColor = C_CYAN(); eraserMode = false; }
    if (x >= 120 && x <= 144){ paintColor = C_BROWN(); eraserMode = false; }
    if (x >= 160 && x <= 184){ paintColor = C_GREY(); eraserMode = false; }
    if (x >= 200 && x <= 224){ paintColor = C_BG(); eraserMode = true; }
    if (x >= 238 && x <= 283){ paintColor = C_BG(); eraserMode = true; }
  }

  if (!eraserMode) oldPaintColor = paintColor;

  paletteOpen = false;
  restoreCanvasArea(32, 112);
  drawMiniTopBar();
}

// =====================================================
// OUTILS
// =====================================================

void drawSizeButton(int x, int y, int size) {
  tft.fillCircle(x, y, 13, TFT_WHITE);
  tft.drawCircle(x, y, 13, TFT_BLACK);
  tft.fillCircle(x, y, size, TFT_BLACK);
}

void drawShapeButton(int x, int y, int type) {
  tft.fillRoundRect(x - 14, y - 12, 28, 24, 5, TFT_WHITE);
  tft.drawRoundRect(x - 14, y - 12, 28, 24, 5, TFT_BLACK);

  if (type == 0) tft.fillCircle(x, y, 6, TFT_BLACK);
  if (type == 1) tft.fillRect(x - 6, y - 6, 12, 12, TFT_BLACK);
  if (type == 2) tft.drawFastHLine(x - 9, y, 18, TFT_BLACK);
  if (type == 3) {
    tft.drawFastHLine(x - 7, y, 14, TFT_BLACK);
    tft.drawFastVLine(x, y - 7, 14, TFT_BLACK);
  }
}

void drawPatternButton(int x, int y, int type) {
  tft.fillRoundRect(x - 18, y - 11, 36, 22, 5, TFT_WHITE);
  tft.drawRoundRect(x - 18, y - 11, 36, 22, 5, TFT_BLACK);

  if (type == 0) {
    tft.fillRect(x - 9, y - 5, 18, 10, TFT_BLACK);
  }
  if (type == 1) {
    for (int i = -10; i <= 10; i += 5) {
      tft.fillCircle(x + i, y, 2, TFT_BLACK);
    }
  }
  if (type == 2) {
    for (int i = -12; i <= 12; i += 6) {
      tft.drawLine(x - 14, y + i, x + 14, y + i, TFT_BLACK);
      tft.drawLine(x + i, y - 8, x + i, y + 8, TFT_BLACK);
    }
  }
}

void drawToolsPanel() {
  toolsOpen = true;
  paletteOpen = false;

  tft.fillRoundRect(10, 32, 300, 95, 8, TFT_DARKGREY);
  tft.drawRoundRect(10, 32, 300, 95, 8, TFT_WHITE);

  drawBoldText("Taille", 45, 45, TFT_WHITE, TFT_DARKGREY, MC_DATUM, 1);
  drawSizeButton(38, 70, 2);
  drawSizeButton(78, 70, 4);
  drawSizeButton(118, 70, 6);
  drawSizeButton(158, 70, 8);

  drawBoldText("Forme", 235, 45, TFT_WHITE, TFT_DARKGREY, MC_DATUM, 1);
  drawShapeButton(210, 70, 0);
  drawShapeButton(250, 70, 1);
  drawShapeButton(290, 70, 2);

  drawShapeButton(38, 108, 3);

  drawBoldText("Trame", 115, 96, TFT_WHITE, TFT_DARKGREY, MC_DATUM, 1);
  drawPatternButton(120, 108, 0);
  drawPatternButton(175, 108, 1);
  drawPatternButton(230, 108, 2);
}

void handleToolsTouch(int x, int y) {
  if (y >= 55 && y <= 86) {
    if (x >= 25 && x <= 51)   brushSize = 2;
    if (x >= 65 && x <= 91)   brushSize = 4;
    if (x >= 105 && x <= 131) brushSize = 6;
    if (x >= 145 && x <= 171) brushSize = 8;

    if (x >= 196 && x <= 224) brushShape = 0;
    if (x >= 236 && x <= 264) brushShape = 1;
    if (x >= 276 && x <= 304) brushShape = 2;
  }

  if (y >= 94 && y <= 122) {
    if (x >= 24 && x <= 52) brushShape = 3;

    if (x >= 102 && x <= 138) brushPattern = 0;
    if (x >= 157 && x <= 193) brushPattern = 1;
    if (x >= 212 && x <= 248) brushPattern = 2;
  }

  toolsOpen = false;
  restoreCanvasArea(32, 130);
  drawMiniTopBar();
}

// =====================================================
// RESTAURATION ECRAN
// =====================================================

void restoreCanvasArea(int y1, int y2) {
  if (y1 < PAINT_Y) y1 = PAINT_Y;
  if (y2 > 240) y2 = 240;

  for (int y = y1; y < y2; y++) {
    for (int x = 0; x < 320; x++) {
      int mx = x / PAINT_SCALE;
      int my = (y - PAINT_Y) / PAINT_SCALE;

      if (mx >= 0 && mx < PAINT_W && my >= 0 && my < PAINT_H) {
        tft.drawPixel(x, y, paintCanvas[my][mx]);
      }
    }
  }
}

// =====================================================
// CANVAS
// =====================================================

void clearCanvas() {
  tft.fillRect(0, PAINT_Y, PAINT_SCREEN_W, PAINT_SCREEN_H, C_BG());
  paintClearMemory(C_BG());
  toolsOpen = false;
  paletteOpen = false;
  drawMiniTopBar();
}

// =====================================================
// SAUVEGARDE BMP
// =====================================================

String paintMakeFileName() {
  if (!startSD()) {
    stopSDStartTouch();
    return "/img/dessin.bmp";
  }

  SD.mkdir("/img");

  int n = 1;
  String path;

  do {
    path = "/img/dessin_" + String(n) + ".bmp";
    n++;
  } while (SD.exists(path) && n < 9999);

  stopSDStartTouch();
  return path;
}

void savePaintBMP() {
  String filename = paintMakeFileName();

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir("/img");

  File f = SD.open(filename, FILE_WRITE);

  if (!f) {
    stopSDStartTouch();
    return;
  }

  int w = PAINT_W;
  int h = PAINT_H;
  int rowSize = w * 3;
  int fileSize = 54 + rowSize * h;

  uint8_t header[54] = {
    'B','M',
    0,0,0,0,
    0,0,0,0,
    54,0,0,0,
    40,0,0,0,
    0,0,0,0,
    0,0,0,0,
    1,0,
    24,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  };

  header[2] = fileSize;
  header[3] = fileSize >> 8;
  header[4] = fileSize >> 16;
  header[5] = fileSize >> 24;

  header[18] = w;
  header[19] = w >> 8;
  header[22] = h;
  header[23] = h >> 8;

  f.write(header, 54);

  for (int y = h - 1; y >= 0; y--) {
    for (int x = 0; x < w; x++) {
      uint16_t c = paintCanvas[y][x];

      uint8_t r = ((c >> 11) & 0x1F) << 3;
      uint8_t g = ((c >> 5) & 0x3F) << 2;
      uint8_t b = (c & 0x1F) << 3;

      f.write(255 - b);
      f.write(255 - g);
      f.write(255 - r);
    }
  }

  f.close();
  stopSDStartTouch();

  tft.fillScreen(TFT_BLACK);
  drawBoldText("Dessin sauvegarde", 160, 105, TFT_GREEN, TFT_BLACK, MC_DATUM, 2);
  drawBoldText(filename, 160, 135, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  delay(1000);
  launchPeinture();
}

// =====================================================
// LANCEMENT
// =====================================================

void launchPeinture() {
  appMode = MODE_PAINT;

  paintColor = C_PEN();
  oldPaintColor = C_PEN();

  brushSize = 3;
  brushShape = 0;
  brushPattern = 0;

  toolsOpen = false;
  paletteOpen = false;
  eraserMode = false;

  stopSDStartTouch();

  tft.fillScreen(C_BG());
  paintClearMemory(C_BG());
  drawMiniTopBar();

  Serial.println("PEINTURE PRETE");
}

// =====================================================
// UPDATE
// =====================================================

void updatePeinture() {
  int x, y;

  if (!getTouch(x, y)) return;

  if (y < PAINT_Y) {
    if (x < 35) {
      drawSubMenu(currentMenu);
      return;
    }

    if (x >= 42 && x <= 87) {
      savePaintBMP();
      return;
    }

    if (x >= 94 && x <= 132) {
      clearCanvas();
      while (ts.touched()) delay(5);
      return;
    }

    if (x >= 140 && x <= 170) {
      if (paletteOpen) {
        paletteOpen = false;
        restoreCanvasArea(32, 112);
        drawMiniTopBar();
      } else {
        drawPalettePanel();
      }

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }

    if (x >= 286 && x <= 318) {
      if (toolsOpen) {
        toolsOpen = false;
        restoreCanvasArea(32, 130);
        drawMiniTopBar();
      } else {
        drawToolsPanel();
      }

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }
  }

  if (paletteOpen && y >= 32 && y <= 112) {
    handlePaletteTouch(x, y);
    while (ts.touched()) delay(5);
    delay(60);
    return;
  }

  if (toolsOpen && y >= 32 && y <= 130) {
    handleToolsTouch(x, y);
    while (ts.touched()) delay(5);
    delay(60);
    return;
  }

  if (y >= PAINT_Y) {
    paintDrawPoint(x, y);
  }
}