#include "galerie.h"

GalButton galButtons[10];
int galButtonCount = 0;

String galNames[60];
String galPaths[60];
int galCount = 0;
int galPage = 0;

bool galViewing = false;
int galCurrentIndex = -1;

const int GAL_PER_PAGE = 6;

// =====================================================
// BOUTONS
// =====================================================

void galAddButton(int x, int y, int w, int h, String label, int index) {
  galButtons[galButtonCount].x = x;
  galButtons[galButtonCount].y = y;
  galButtons[galButtonCount].w = w;
  galButtons[galButtonCount].h = h;
  galButtons[galButtonCount].label = label;
  galButtons[galButtonCount].index = index;
  galButtonCount++;
}

bool galInButton(GalButton b, int x, int y) {
  return x >= b.x && x <= b.x + b.w &&
         y >= b.y && y <= b.y + b.h;
}

// =====================================================
// BMP 24 BITS
// =====================================================

uint16_t galRead16(File &f) {
  uint16_t result;
  result  = f.read();
  result |= f.read() << 8;
  return result;
}

uint32_t galRead32(File &f) {
  uint32_t result;
  result  = f.read();
  result |= f.read() << 8;
  result |= f.read() << 16;
  result |= f.read() << 24;
  return result;
}

void galDrawBmp(
  const char* filename,
  int x0,
  int y0,
  int maxW,
  int maxH
) {
  File bmpFile = SD.open(filename);

  if (!bmpFile) {
    return;
  }

  if (galRead16(bmpFile) != 0x4D42) {
    bmpFile.close();
    return;
  }

  galRead32(bmpFile);
  galRead32(bmpFile);

  uint32_t imageOffset = galRead32(bmpFile);
  galRead32(bmpFile);

  int32_t bmpWidth  = galRead32(bmpFile);
  int32_t bmpHeight = galRead32(bmpFile);

  if (galRead16(bmpFile) != 1) {
    bmpFile.close();
    return;
  }

  uint16_t depth = galRead16(bmpFile);

  if (depth != 24) {
    bmpFile.close();
    return;
  }

  if (galRead32(bmpFile) != 0) {
    bmpFile.close();
    return;
  }

  bool flip = true;

  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;

  float scaleX = (float)bmpWidth / maxW;
  float scaleY = (float)bmpHeight / maxH;

  float scale = max(scaleX, scaleY);

  if (scale < 1.0) {
    scale = 1.0;
  }

  int drawW = bmpWidth / scale;
  int drawH = bmpHeight / scale;

  if (drawW < 1) drawW = 1;
  if (drawH < 1) drawH = 1;

  int xStart = x0 + (maxW - drawW) / 2;
  int yStart = y0 + (maxH - drawH) / 2;

  for (int y = 0; y < drawH; y++) {
    int srcY = y * scale;

    uint32_t rowPos;

    if (flip) {
      rowPos = imageOffset + (bmpHeight - 1 - srcY) * rowSize;
    } else {
      rowPos = imageOffset + srcY * rowSize;
    }

    for (int x = 0; x < drawW; x++) {
      int srcX = x * scale;

      bmpFile.seek(rowPos + srcX * 3);

      uint8_t b = bmpFile.read();
      uint8_t g = bmpFile.read();
      uint8_t r = bmpFile.read();

      uint16_t color = tft.color565(255 - r, 255 - g, 255 - b);

      tft.drawPixel(xStart + x, yStart + y, color);
    }
  }

  bmpFile.close();
}

bool galIsBmp(String path) {
  String lower = path;
  lower.toLowerCase();
  return lower.endsWith(".bmp");
}

bool galIsJpg(String path) {
  String lower = path;
  lower.toLowerCase();
  return lower.endsWith(".jpg") || lower.endsWith(".jpeg");
}

// =====================================================
// HEADER
// =====================================================

void galDrawHeader() {
  tft.fillRect(0, 0, 320, 36, TFT_BLACK);

  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);
  galAddButton(0, 0, 45, 36, "BACK", -1);

  drawBoldText("Galerie", 135, 18, TFT_WHITE, TFT_BLACK, MC_DATUM, 4);

  if (!galViewing) {
    int totalPages = (galCount + GAL_PER_PAGE - 1) / GAL_PER_PAGE;
    if (totalPages < 1) totalPages = 1;

    tft.fillTriangle(232, 18, 248, 8, 248, 28, TFT_WHITE);
    galAddButton(222, 0, 36, 36, "PREV", -2);

    String pageText = String(galPage + 1) + "/" + String(totalPages);
    drawBoldText(pageText, 274, 18, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

    tft.fillTriangle(315, 18, 299, 8, 299, 28, TFT_WHITE);
    galAddButton(286, 0, 34, 36, "NEXT", -3);
  }
}

// =====================================================
// CHARGEMENT FICHIERS
// =====================================================

void galLoadImages() {
  galCount = 0;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir("/img");

  File dir = SD.open("/img");

  while (dir) {
    File entry = dir.openNextFile();

    if (!entry) break;

    if (!entry.isDirectory() && galCount < 60) {
      String name = String(entry.name());

      String lower = name;
      lower.toLowerCase();

      if (
        lower.endsWith(".jpg") ||
        lower.endsWith(".jpeg") ||
        lower.endsWith(".bmp")
      ) {
        String cleanName = name;
        cleanName.replace("/img/", "");

        galNames[galCount] = cleanName;
        galPaths[galCount] = "/img/" + cleanName;

        galCount++;
      }
    }

    entry.close();
  }

  if (dir) dir.close();

  stopSDStartTouch();
}

// =====================================================
// MINIATURE
// =====================================================

void galDrawTile(GalButton b, String path) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, TFT_WHITE);

  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_BLACK);
  tft.drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 8, TFT_BLACK);

  if (startSD()) {
    if (SD.exists(path)) {
      if (galIsBmp(path)) {
        galDrawBmp(
          path.c_str(),
          b.x + 6,
          b.y + 4,
          45,
          43
        );
      } else {
        TJpgDec.setSwapBytes(true);
        TJpgDec.setJpgScale(4);
        TJpgDec.drawSdJpg(b.x + 8, b.y + 5, path.c_str());
        TJpgDec.setJpgScale(1);
        TJpgDec.setSwapBytes(true);
      }
    } else {
      drawBoldText("IMG", b.x + 72, b.y + 24, TFT_RED, TFT_WHITE, MC_DATUM, 2);
    }

    stopSDStartTouch();
  }

  String shortName = b.label;

  if (shortName.length() > 12) {
    shortName = shortName.substring(0, 12);
  }

  drawBoldText(
    shortName,
    b.x + 72,
    b.y + 48,
    TFT_BLACK,
    TFT_WHITE,
    MC_DATUM,
    1
  );
}

// =====================================================
// LISTE GALERIE
// =====================================================

void galDrawList() {
  appMode = MODE_GALERIE;
  galViewing = false;
  galButtonCount = 0;

  tft.fillScreen(TFT_BLACK);

  galLoadImages();

  int totalPages = (galCount + GAL_PER_PAGE - 1) / GAL_PER_PAGE;

  if (totalPages < 1) {
    totalPages = 1;
  }

  if (galPage < 0) {
    galPage = 0;
  }

  if (galPage >= totalPages) {
    galPage = totalPages - 1;
  }

  galDrawHeader();

  if (galCount == 0) {
    drawBoldText(
      "Aucune image",
      160,
      120,
      TFT_WHITE,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    return;
  }

  int startIndex = galPage * GAL_PER_PAGE;
  int endIndex = startIndex + GAL_PER_PAGE;

  if (endIndex > galCount) {
    endIndex = galCount;
  }

  int visibleIndex = 0;

  for (int i = startIndex; i < endIndex; i++) {
    int x = (visibleIndex % 2 == 0) ? 10 : 165;
    int y = 44 + (visibleIndex / 2) * 62;

    galAddButton(
      x,
      y,
      145,
      55,
      galNames[i],
      i
    );

    galDrawTile(
      galButtons[galButtonCount - 1],
      galPaths[i]
    );

    visibleIndex++;
  }
}

// =====================================================
// OUVERTURE IMAGE
// =====================================================

void galOpenImage(int index) {
  galViewing = true;
  galCurrentIndex = index;
  galButtonCount = 0;

  tft.fillScreen(TFT_BLACK);

  galAddButton(0, 0, 320, 240, "IMAGE_BACK", -4);

  if (!startSD()) {
    drawBoldText(
      "Erreur SD",
      160,
      120,
      TFT_RED,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    stopSDStartTouch();
    return;
  }

  if (!SD.exists(galPaths[index])) {
    drawBoldText(
      "Image introuvable",
      160,
      120,
      TFT_RED,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    stopSDStartTouch();
    return;
  }

  if (galIsBmp(galPaths[index])) {
    galDrawBmp(
      galPaths[index].c_str(),
      0,
      0,
      320,
      240
    );
  } else {
    TJpgDec.setSwapBytes(true);
    TJpgDec.setJpgScale(1);
    TJpgDec.drawSdJpg(0, 0, galPaths[index].c_str());
    TJpgDec.setSwapBytes(true);
  }

  stopSDStartTouch();
}

// =====================================================
// LANCEMENT / UPDATE
// =====================================================

void launchGalerie() {
  galPage = 0;
  galDrawList();

  Serial.println("GALERIE PRETE");
}

void updateGalerie() {
  int x, y;

  if (getTouch(x, y)) {
    for (int i = 0; i < galButtonCount; i++) {
      if (galInButton(galButtons[i], x, y)) {

        if (galButtons[i].index == -1) {
          if (galViewing) {
            galDrawList();
          } else {
            drawSubMenu(currentMenu);
          }
        }

        else if (galButtons[i].index == -2) {
          if (!galViewing && galPage > 0) {
            galPage--;
            galDrawList();
          }
        }

        else if (galButtons[i].index == -3) {
          int totalPages = (galCount + GAL_PER_PAGE - 1) / GAL_PER_PAGE;

          if (totalPages < 1) {
            totalPages = 1;
          }

          if (!galViewing && galPage < totalPages - 1) {
            galPage++;
            galDrawList();
          }
        }

        else if (galButtons[i].index == -4) {
          galDrawList();
        }

        else {
          galOpenImage(galButtons[i].index);
        }

        while (ts.touched()) {
          delay(5);
        }

        delay(80);
        break;
      }
    }
  }
}