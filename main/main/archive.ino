#include "archive.h"
#include "menu_core.h"
#include "text_engine.h"

#define AR_MAX_FILES 24
#define AR_PAGE_CHARS 520

String arNames[AR_MAX_FILES];
String arPaths[AR_MAX_FILES];
int arCount = 0;
int arListPage = 0;

String arTitle = "";
String arBody = "";
int arTextPage = 0;

enum ArMode {
  AR_LIST,
  AR_VIEW
};

ArMode arMode = AR_LIST;

unsigned long arLastTouch = 0;

void arButton(int x, int y, int w, int h, String txt, uint16_t bg, uint16_t fg = TFT_WHITE) {
  tft.fillRoundRect(x, y, w, h, 7, bg);
  tft.drawRoundRect(x, y, w, h, 7, TFT_DARKGREY);
  teDrawString(txt, x + w / 2, y + h / 2, fg, bg, MC_DATUM, 1);
}

void arHeader(String title) {
  tft.fillRect(0, 0, 320, 32, TFT_BLACK);
  tft.fillTriangle(11, 16, 31, 5, 31, 27, TFT_WHITE);
  teDrawString(title, 160, 16, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

bool arIsTxt(String name) {
  name.toLowerCase();
  return name.endsWith(".txt");
}

void arScanFiles() {
  arCount = 0;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir("/Archives");

  File dir = SD.open("/Archives");

  if (!dir) {
    stopSDStartTouch();
    return;
  }

  while (arCount < AR_MAX_FILES) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (!entry.isDirectory()) {
      String path = String(entry.name());

      if (!path.startsWith("/")) {
        path = "/Archives/" + path;
      }

      if (arIsTxt(path)) {
        arPaths[arCount] = path;

        String name = path;
        name.replace("/Archives/", "");
        name.replace(".txt", "");

        arNames[arCount] = name;
        arCount++;
      }
    }

    entry.close();
  }

  dir.close();
  stopSDStartTouch();
}

void arDrawList() {
  arMode = AR_LIST;

  tft.fillScreen(TFT_BLACK);
  arHeader("Archive");

  int start = arListPage * 5;

  if (arCount == 0) {
    teDrawString("Aucun texte sauvegarde", 160, 110, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    arButton(100, 170, 120, 30, "SCAN", TFT_GREEN, TFT_BLACK);
    return;
  }

  for (int i = 0; i < 5; i++) {
    int idx = start + i;
    if (idx >= arCount) break;

    int y = 42 + i * 32;

    tft.fillRoundRect(12, y, 296, 27, 7, TFT_WHITE);
    tft.drawRoundRect(12, y, 296, 27, 7, TFT_CYAN);

    String n = arNames[idx];
    if (n.length() > 32) n = n.substring(0, 32);

    teDrawString(n, 160, y + 14, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
  }

  arButton(16, 212, 60, 22, "<", TFT_DARKGREY);
  arButton(84, 212, 60, 22, ">", TFT_DARKGREY);
  arButton(152, 212, 70, 22, "SCAN", TFT_GREEN, TFT_BLACK);
  arButton(230, 212, 76, 22, String(arListPage + 1), TFT_BLUE);
}

void arOpenFile(int index) {
  if (index < 0 || index >= arCount) return;

  arTitle = "";
  arBody = "";
  arTextPage = 0;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  File f = SD.open(arPaths[index], FILE_READ);

  if (!f) {
    stopSDStartTouch();
    return;
  }

  bool titleDone = false;
  bool separatorPassed = false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.replace("\r", "");

    if (!titleDone) {
      arTitle = line;
      titleDone = true;
      continue;
    }

    if (!separatorPassed) {
      if (line == "-----") {
        separatorPassed = true;
      }
      continue;
    }

    arBody += line;

    if (f.available()) {
      arBody += "\n"; // IMPORTANT : restaure les retours à la ligne
    }
  }

  f.close();
  stopSDStartTouch();

  if (arTitle.length() == 0) arTitle = arNames[index];
  if (arBody.length() == 0) arBody = "(texte vide)";

  arDrawView();
}

int arPageCount() {
  return tePageCount(arBody, AR_PAGE_CHARS);
}

String arPageText() {
  return teGetPage(arBody, arTextPage, AR_PAGE_CHARS);
}

void arDrawView() {
  arMode = AR_VIEW;

  tft.fillScreen(TFT_WHITE);

  tft.fillRect(0, 0, 320, 30, TFT_BLACK);
  tft.fillTriangle(11, 16, 31, 5, 31, 27, TFT_WHITE);

  String title = arTitle;
  if (title.length() > 26) title = title.substring(0, 26);

  teDrawString(title, 168, 15, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  teDrawWrapped(arPageText(), 8, 38, 304, 204, 14, TFT_BLACK, TFT_WHITE, 1);

  tft.fillRect(0, 214, 320, 26, TFT_BLACK);

  arButton(8, 217, 56, 20, "<", TFT_DARKGREY);
  arButton(72, 217, 56, 20, ">", TFT_DARKGREY);
  arButton(136, 217, 72, 20, "Liste", TFT_BLUE);
  arButton(216, 217, 96, 20, String(arTextPage + 1) + "/" + String(arPageCount()), TFT_BLACK);
}

void launchArchive() {
  appMode = MODE_ARCHIVE;

  teSetFont(2);

  stopSDStartTouch();

  arListPage = 0;
  arTextPage = 0;

  arScanFiles();
  arDrawList();

  Serial.println("ARCHIVE PRETE");
}

void updateArchive() {
  int x, y;

  if (!getTouch(x, y)) return;
  if (millis() - arLastTouch < 160) return;

  arLastTouch = millis();

  if (y < 32 && x < 45) {
    drawSubMenu(currentMenu);
    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (arMode == AR_LIST) {
    int start = arListPage * 5;

    for (int i = 0; i < 5; i++) {
      int idx = start + i;
      if (idx >= arCount) break;

      int by = 42 + i * 32;

      if (x >= 12 && x <= 308 && y >= by && y <= by + 27) {
        arOpenFile(idx);
        while (ts.touched()) delay(5);
        delay(80);
        return;
      }
    }

    if (x >= 16 && x <= 76 && y >= 212 && y <= 234) {
      arListPage--;
      if (arListPage < 0) arListPage = 0;
      arDrawList();
    }

    else if (x >= 84 && x <= 144 && y >= 212 && y <= 234) {
      int maxPage = (arCount - 1) / 5;
      arListPage++;
      if (arListPage > maxPage) arListPage = maxPage;
      arDrawList();
    }

    else if (x >= 152 && x <= 222 && y >= 212 && y <= 234) {
      arScanFiles();
      arDrawList();
    }
  }

  else if (arMode == AR_VIEW) {
    if (x >= 8 && x <= 64 && y >= 217 && y <= 237) {
      arTextPage--;
      if (arTextPage < 0) arTextPage = 0;
      arDrawView();
    }

    else if (x >= 72 && x <= 128 && y >= 217 && y <= 237) {
      arTextPage++;
      if (arTextPage >= arPageCount()) arTextPage = arPageCount() - 1;
      arDrawView();
    }

    else if (x >= 136 && x <= 208 && y >= 217 && y <= 237) {
      arDrawList();
    }
  }

  while (ts.touched()) delay(5);
  delay(80);
}