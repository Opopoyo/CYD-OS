#include "ecriture.h"
#include "menu_core.h"
#include "text_engine.h"

#define WR_MAX_TEXT 2400
#define WR_PAGE_CHARS 300

String wrText = "";
String wrTitle = "";

int wrCursor = 0;
int wrPage = 0;

bool wrTitleMode = false;

unsigned long wrLastTouch = 0;
unsigned long wrLastBlink = 0;
bool wrCursorVisible = true;

#define WR_SIDE_W 42
#define WR_TEXT_X 46
#define WR_TEXT_Y 6
#define WR_TEXT_W 269
#define WR_TEXT_H 72

#define WR_KB_Y 82

String wrCleanFileName(String title) {
  title.trim();
  if (title.length() == 0) title = "SansTitre";

  String out = "";

  for (int i = 0; i < title.length(); i++) {
    char c = title[i];

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9')) {
      out += c;
    }
    else if (c == ' ') {
      out += "_";
    }
  }

  if (out.length() == 0) out = "SansTitre";
  return "/Archives/" + out + ".txt";
}

int wrPageCount() {
  return tePageCount(wrText, WR_PAGE_CHARS);
}

String wrPageText() {
  return teGetPage(wrText, wrPage, WR_PAGE_CHARS);
}

void wrClampCursor() {
  if (wrCursor < 0) wrCursor = 0;
  if (wrCursor > wrText.length()) wrCursor = wrText.length();

  int cursorPage = wrCursor / WR_PAGE_CHARS;
  if (cursorPage < 0) cursorPage = 0;
  if (cursorPage >= wrPageCount()) cursorPage = wrPageCount() - 1;

  wrPage = cursorPage;
}

void wrBackspaceUtf8() {
  if (wrCursor <= 0 || wrText.length() == 0) return;

  int cut = wrCursor - 1;

  while (cut > 0 && (((uint8_t)wrText[cut] & 0xC0) == 0x80)) {
    cut--;
  }

  wrText.remove(cut, wrCursor - cut);
  wrCursor = cut;
  wrClampCursor();
}

void wrInsertText(String s) {
  if (wrText.length() + s.length() >= WR_MAX_TEXT) return;

  wrText = wrText.substring(0, wrCursor) + s + wrText.substring(wrCursor);
  wrCursor += s.length();

  wrClampCursor();
}

void wrMoveCursorLeft() {
  if (wrCursor <= 0) return;

  wrCursor--;

  while (wrCursor > 0 && (((uint8_t)wrText[wrCursor] & 0xC0) == 0x80)) {
    wrCursor--;
  }

  wrClampCursor();
}

void wrMoveCursorRight() {
  if (wrCursor >= wrText.length()) return;

  wrCursor++;

  while (wrCursor < wrText.length() &&
         (((uint8_t)wrText[wrCursor] & 0xC0) == 0x80)) {
    wrCursor++;
  }

  wrClampCursor();
}

void wrButton(int x, int y, int w, int h, String label, uint16_t bg, uint16_t fg = TFT_WHITE) {
  tft.fillRoundRect(x, y, w, h, 6, bg);
  tft.drawRoundRect(x, y, w, h, 6, TFT_DARKGREY);
  teDrawString(label, x + w / 2, y + h / 2, fg, bg, MC_DATUM, 1);
}

void wrDrawSideBar() {
  tft.fillRect(0, 0, WR_SIDE_W, 240, TFT_BLACK);
  tft.drawFastVLine(WR_SIDE_W - 1, 0, 240, TFT_DARKGREY);

  tft.fillTriangle(10, 18, 30, 7, 30, 29, TFT_WHITE);

  wrButton(5, 45, 32, 25, "<", TFT_DARKGREY);
  wrButton(5, 76, 32, 25, ">", TFT_DARKGREY);
  wrButton(5, 112, 32, 31, "SV", TFT_GREEN, TFT_BLACK);
}

void wrDrawTextBox() {
  uint16_t bg = tft.color565(10, 12, 22);
  uint16_t frame = tft.color565(80, 180, 255);
  uint16_t fg = tft.color565(235, 235, 220);

  tft.fillRoundRect(WR_TEXT_X, WR_TEXT_Y, WR_TEXT_W, WR_TEXT_H, 8, bg);
  tft.drawRoundRect(WR_TEXT_X, WR_TEXT_Y, WR_TEXT_W, WR_TEXT_H, 8, frame);

  String pageTxt = wrPageText();

  int localCursor = wrCursor - wrPage * WR_PAGE_CHARS;
  if (localCursor < 0) localCursor = 0;
  if (localCursor > pageTxt.length()) localCursor = pageTxt.length();

  String shown = pageTxt;

  if (wrCursorVisible) {
    shown = pageTxt.substring(0, localCursor) + "|" + pageTxt.substring(localCursor);
  }

  if (shown.length() == 0) {
    teDrawString("Tape ton texte...", WR_TEXT_X + 10, WR_TEXT_Y + 14, TFT_DARKGREY, bg, TL_DATUM, 2);
  }
  else {
    teDrawWrapped(
      shown,
      WR_TEXT_X + 8,
      WR_TEXT_Y + 8,
      WR_TEXT_W - 16,
      WR_TEXT_Y + WR_TEXT_H - 10,
      16,
      fg,
      bg,
      1
    );
  }

  teDrawString(
    String(wrPage + 1) + "/" + String(wrPageCount()),
    WR_TEXT_X + WR_TEXT_W - 25,
    WR_TEXT_Y + WR_TEXT_H - 8,
    TFT_CYAN,
    bg,
    MC_DATUM,
    1
  );
}

void wrDrawFixedLayout() {
  tft.fillScreen(TFT_BLACK);
  wrDrawSideBar();
  wrDrawTextBox();
}

void wrSaveWithTitle() {
  wrTitle.trim();
  if (wrTitle.length() == 0) wrTitle = "SansTitre";

  if (!startSD()) {
    stopSDStartTouch();
    wrDrawFixedLayout();
    teKeyboardDraw();
    return;
  }

  SD.mkdir("/Archives");

  String path = wrCleanFileName(wrTitle);
  File f = SD.open(path, FILE_WRITE);

  if (f) {
    f.println(wrTitle);
    f.println("-----");
    f.print(wrText);
    f.close();
  }

  stopSDStartTouch();

  tft.fillScreen(TFT_BLACK);
  teDrawString("Sauvegarde OK", 160, 105, TFT_GREEN, TFT_BLACK, MC_DATUM, 2);
  teDrawString(wrTitle, 160, 135, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  delay(1000);
  drawSubMenu(currentMenu);
}

void wrAskTitle() {
  wrTitleMode = true;
  teKeyboardOpen("Titre", wrTitle);
}

void wrHandleLeftPanel(int x, int y) {
  if (x < WR_SIDE_W && y < 36) {
    drawSubMenu(currentMenu);
    return;
  }

  if (x >= 5 && x <= 37 && y >= 45 && y <= 70) {
    wrMoveCursorLeft();
    wrDrawTextBox();
    return;
  }

  if (x >= 5 && x <= 37 && y >= 76 && y <= 101) {
    wrMoveCursorRight();
    wrDrawTextBox();
    return;
  }

  if (x >= 5 && x <= 37 && y >= 112 && y <= 143) {
    wrAskTitle();
    return;
  }
}

void launchWriting() {
  appMode = MODE_WRITING;

  teSetFont(2);

  wrText = "";
  wrTitle = "";
  wrCursor = 0;
  wrPage = 0;
  wrTitleMode = false;

  wrLastBlink = millis();
  wrCursorVisible = true;

  stopSDStartTouch();

  wrDrawFixedLayout();

  // Clavier directement ouvert
  teKeyboardOpen("Ecriture", "");

  // On redessine la zone texte au-dessus du clavier
  wrDrawSideBar();
  wrDrawTextBox();

  Serial.println("ECRITURE PRETE");
}

void updateWriting() {
  int x, y;

  if (getTouch(x, y)) {
    if (millis() - wrLastTouch < 120) return;
    wrLastTouch = millis();

    if (x < WR_SIDE_W) {
      wrHandleLeftPanel(x, y);

      while (ts.touched()) delay(5);
      delay(60);
      return;
    }
  }

  if (teKeyboardActive) {
    teKeyboardUpdate();

    if (teKeyboardChanged) {
      wrInsertText(teKeyboardLastInput);

      teKeyboardChanged = false;
      teKeyboardLastInput = "";

      wrDrawTextBox();
    }

    

    if (teKeyboardDone) {
      if (wrTitleMode) {
        wrTitle = teKeyboardValue;
        wrTitleMode = false;
        teKeyboardDone = false;
        wrSaveWithTitle();
        return;
      }

      wrInsertText(teKeyboardValue);
      teKeyboardDone = false;

      teKeyboardOpen("Ecriture", "");
      wrDrawSideBar();
      wrDrawTextBox();
      return;
    }

    if (teKeyboardCancel) {
      if (wrTitleMode) {
        wrTitleMode = false;
        teKeyboardCancel = false;
        teKeyboardOpen("Ecriture", "");
        wrDrawSideBar();
        wrDrawTextBox();
        return;
      }

      teKeyboardCancel = false;
      return;
    }

    // Entrée directe caractère par caractère impossible ici :
    // le clavier commun renvoie son texte avec OK.
    return;
  }

  if (millis() - wrLastBlink > 520) {
    wrLastBlink = millis();
    wrCursorVisible = !wrCursorVisible;
    wrDrawTextBox();
  }
}