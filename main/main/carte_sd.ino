#include "carte_sd.h"
#include "menu_core.h"

#define SDX_MAX_ITEMS 60
#define SDX_ITEMS_PER_PAGE 6

struct SDXItem {
  String name;
  String path;
  bool isDir;
};

SDXItem sdxItems[SDX_MAX_ITEMS];

String sdxPath = "/";
int sdxCount = 0;
int sdxPage = 0;

unsigned long sdxLastTouch = 0;

void sdxHeader() {
  tft.fillRect(0, 0, 320, 36, TFT_BLACK);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);

  String title = sdxPath;
  if (title.length() > 24) title = "..." + title.substring(title.length() - 21);

  drawBoldText(title, 170, 18, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

void sdxDrawButton(int x, int y, int w, int h, String txt, uint16_t bg, uint16_t fg) {
  tft.fillRoundRect(x, y, w, h, 8, bg);
  tft.drawRoundRect(x, y, w, h, 8, TFT_DARKGREY);
  drawBoldText(txt, x + w / 2, y + h / 2, fg, bg, MC_DATUM, 1);
}

String sdxFileName(String full) {
  int p = full.lastIndexOf("/");
  if (p < 0) return full;
  return full.substring(p + 1);
}

String sdxParentPath(String p) {
  if (p == "/") return "/";

  if (p.endsWith("/")) p.remove(p.length() - 1);

  int slash = p.lastIndexOf("/");
  if (slash <= 0) return "/";

  return p.substring(0, slash);
}

bool sdxIsTextFile(String name) {
  String n = name;
  n.toLowerCase();
  return n.endsWith(".txt") || n.endsWith(".ino") || n.endsWith(".h") || n.endsWith(".cpp");
}

void sdxScan() {
  sdxCount = 0;
  sdxPage = 0;

  if (!startSD()) return;

  File dir = SD.open(sdxPath);

  if (!dir || !dir.isDirectory()) {
    stopSDStartTouch();
    return;
  }

  while (sdxCount < SDX_MAX_ITEMS) {
    File entry = dir.openNextFile();
    if (!entry) break;

    String raw = String(entry.name());
    bool isDir = entry.isDirectory();

    String clean = raw;

    if (clean.length() == 0) {
      entry.close();
      continue;
    }

    if (!clean.startsWith("/")) {
      if (sdxPath == "/") clean = "/" + clean;
      else clean = sdxPath + "/" + clean;
    }

    sdxItems[sdxCount].path = clean;
    sdxItems[sdxCount].name = sdxFileName(clean);
    sdxItems[sdxCount].isDir = isDir;

    sdxCount++;
    entry.close();
  }

  dir.close();
  stopSDStartTouch();
}

void sdxDrawList() {
  tft.fillScreen(TFT_BLACK);
  sdxHeader();

  int totalPages = (sdxCount + SDX_ITEMS_PER_PAGE - 1) / SDX_ITEMS_PER_PAGE;
  if (totalPages < 1) totalPages = 1;

  if (sdxPage < 0) sdxPage = 0;
  if (sdxPage >= totalPages) sdxPage = totalPages - 1;

  if (sdxPath != "/") {
    sdxDrawButton(8, 42, 304, 24, ".. dossier parent", TFT_DARKGREY, TFT_WHITE);
  }

  int startY = (sdxPath == "/") ? 44 : 72;

  int first = sdxPage * SDX_ITEMS_PER_PAGE;
  int last = first + SDX_ITEMS_PER_PAGE;
  if (last > sdxCount) last = sdxCount;

  int visible = 0;

  for (int i = first; i < last; i++) {
    int y = startY + visible * 27;

    uint16_t bg = sdxItems[i].isDir ? TFT_BLUE : TFT_WHITE;
    uint16_t fg = sdxItems[i].isDir ? TFT_WHITE : TFT_BLACK;

    String icon = sdxItems[i].isDir ? "[D] " : "[F] ";
    String label = icon + sdxItems[i].name;

    if (label.length() > 31) label = label.substring(0, 31);

    sdxDrawButton(8, y, 304, 24, label, bg, fg);
    visible++;
  }

  if (sdxCount == 0) {
    drawBoldText("Dossier vide", 160, 130, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
  }

  if (totalPages > 1) {
    if (sdxPage > 0) {
      tft.fillTriangle(250, 229, 260, 222, 260, 236, TFT_WHITE);
    }

    if (sdxPage < totalPages - 1) {
      tft.fillTriangle(310, 229, 300, 222, 300, 236, TFT_WHITE);
    }

    drawBoldText(String(sdxPage + 1) + "/" + String(totalPages), 280, 229, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  }
}

void sdxOpenTextFile(String path) {
  tft.fillScreen(TFT_BLACK);
  sdxHeader();

  if (!startSD()) {
    drawBoldText("Erreur SD", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    stopSDStartTouch();
    return;
  }

  File f = SD.open(path);

  if (!f) {
    drawBoldText("Impossible ouvrir", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    stopSDStartTouch();
    return;
  }

  int y = 44;
  String line = "";
  int lines = 0;

  while (f.available() && lines < 10) {
    char ch = f.read();

    if (ch == '\n' || line.length() >= 38) {
      drawBoldText(line, 8, y, TFT_WHITE, TFT_BLACK, TL_DATUM, 1);
      y += 18;
      lines++;
      line = "";
    } else if (ch != '\r') {
      line += ch;
    }
  }

  if (line.length() > 0 && lines < 10) {
    drawBoldText(line, 8, y, TFT_WHITE, TFT_BLACK, TL_DATUM, 1);
  }

  f.close();
  stopSDStartTouch();

  drawBoldText("Toucher pour retour", 160, 225, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);

  while (ts.touched()) delay(5);
  delay(200);

  while (!ts.touched()) delay(10);
  while (ts.touched()) delay(5);

  sdxDrawList();
}

void launchCarteSD() {
  appMode = MODE_CARTE_SD;

  stopSDStartTouch();

  sdxPath = "/";
  sdxPage = 0;

  sdxScan();
  sdxDrawList();

  Serial.println("CARTE SD PRETE");
}

void updateCarteSD() {
  int x, y;

  if (!getTouch(x, y)) return;
  if (millis() - sdxLastTouch < 180) return;

  sdxLastTouch = millis();

  if (y < 36 && x < 45) {
    drawSubMenu(currentMenu);

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  int totalPages = (sdxCount + SDX_ITEMS_PER_PAGE - 1) / SDX_ITEMS_PER_PAGE;
  if (totalPages < 1) totalPages = 1;

  if (totalPages > 1 && y >= 216) {
    if (x >= 238 && x <= 270 && sdxPage > 0) {
      sdxPage--;
      sdxDrawList();
    }

    else if (x >= 292 && x <= 319 && sdxPage < totalPages - 1) {
      sdxPage++;
      sdxDrawList();
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (sdxPath != "/" && x >= 8 && x <= 312 && y >= 42 && y <= 66) {
    sdxPath = sdxParentPath(sdxPath);
    sdxScan();
    sdxDrawList();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  int startY = (sdxPath == "/") ? 44 : 72;

  if (x < 8 || x > 312 || y < startY) return;

  int visibleIndex = (y - startY) / 27;
  if (visibleIndex < 0 || visibleIndex >= SDX_ITEMS_PER_PAGE) return;

  int itemIndex = sdxPage * SDX_ITEMS_PER_PAGE + visibleIndex;
  if (itemIndex < 0 || itemIndex >= sdxCount) return;

  if (sdxItems[itemIndex].isDir) {
    sdxPath = sdxItems[itemIndex].path;
    sdxScan();
    sdxDrawList();
  } else {
    if (sdxIsTextFile(sdxItems[itemIndex].name)) {
      sdxOpenTextFile(sdxItems[itemIndex].path);
    } else {
      tft.fillRoundRect(35, 92, 250, 58, 10, TFT_WHITE);
      drawBoldText("Fichier non lisible", 160, 112, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);
      drawBoldText(sdxItems[itemIndex].name, 160, 135, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
      delay(900);
      sdxDrawList();
    }
  }

  while (ts.touched()) delay(5);
  delay(80);
}