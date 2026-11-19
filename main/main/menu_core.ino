#include "menu_core.h"
#include "menu_registry.h"
#include "text_engine.h"

// =====================================================
// ETAT MENU
// =====================================================

Button buttons[20];
int buttonCount = 0;
int currentMenu = -1;
int currentPage = 0;

const int APPS_PER_PAGE = 6;

// =====================================================
// ICONES
// =====================================================

const char* getMainIcon(int index) {
  if (index < 0 || index >= menuCount) return ICON_SD;
  return menuIcons[index];
}

const char* getSubIcon(String label) {
  return getAppIconByLabel(label);
}

// =====================================================
// SD / JPG
// =====================================================

bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return false;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

bool startSD() {
  digitalWrite(XPT2046_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  sdSPI.end();
  delay(5);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI, 10000000)) {
    Serial.println("SD ECHEC");
    return false;
  }

  return true;
}

void stopSDStartTouch() {
  SD.end();
  delay(5);

  digitalWrite(SD_CS, HIGH);

  touchscreenSPI.end();
  delay(5);

  touchscreenSPI.begin(
    XPT2046_CLK,
    XPT2046_MISO,
    XPT2046_MOSI,
    XPT2046_CS
  );

  ts.begin(touchscreenSPI);
  ts.setRotation(1);
}

void drawJpgFromSD(const char* filename, int x, int y) {
  if (!SD.exists(filename)) {
    tft.fillRect(x, y, 43, 43, TFT_WHITE);
    tft.drawRect(x, y, 43, 43, TFT_RED);

    tft.setTextColor(TFT_RED, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("IMG", x + 21, y + 21, 1);

    Serial.print("Icone introuvable : ");
    Serial.println(filename);
    return;
  }

  TJpgDec.drawSdJpg(x, y, filename);
}

// =====================================================
// TEXTE / POLICES MENU
// =====================================================

void drawBoldText(
  String txt,
  int x,
  int y,
  uint16_t fg,
  uint16_t bg,
  uint8_t datum,
  uint8_t font
) {
  tft.setTextColor(fg, bg);
  tft.setTextDatum(datum);

  tft.drawString(txt, x, y, font);
  tft.drawString(txt, x + 1, y, font);
}

void menuLoadTitleFont() {
  if (!teLoadFont("Plank20.vlw")) {
    teSetFont(4);
  }
}

void menuLoadButtonFont() {
  if (!teLoadFont("Papernotes18.vlw")) {
    teSetFont(2);
  }
}

void menuFinishFont() {
  teFinishDraw();
}

// =====================================================
// TACTILE
// =====================================================

bool getTouch(int &x, int &y) {
  if (!ts.touched()) return false;

  TS_Point p1 = ts.getPoint();
  delay(3);

  if (!ts.touched()) return false;

  TS_Point p2 = ts.getPoint();

  int rawX = (p1.x + p2.x) / 2;
  int rawY = (p1.y + p2.y) / 2;

  x = map(rawX, TS_MIN_X, TS_MAX_X, 0, SCREEN_W);
  y = map(rawY, TS_MIN_Y, TS_MAX_Y, 0, SCREEN_H);

  x = constrain(x, 0, SCREEN_W);
  y = constrain(y, 0, SCREEN_H);

  return true;
}

// =====================================================
// BOUTONS
// =====================================================

void addButton(int x, int y, int w, int h, String label, int index) {
  if (buttonCount >= 20) return;

  buttons[buttonCount].x = x;
  buttons[buttonCount].y = y;
  buttons[buttonCount].w = w;
  buttons[buttonCount].h = h;
  buttons[buttonCount].label = label;
  buttons[buttonCount].index = index;

  buttonCount++;
}

bool inButton(Button b, int x, int y) {
  return (
    x >= b.x &&
    x <= b.x + b.w &&
    y >= b.y &&
    y <= b.y + b.h
  );
}

void drawPressed(Button b) {
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_GREEN);
  tft.drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 8, TFT_GREEN);
}

// =====================================================
// WIFI
// =====================================================

void drawWifiIndicator() {
  tft.fillCircle(313, 233, 5, TFT_BLACK);

  if (WiFi.status() == WL_CONNECTED) {
    tft.fillCircle(313, 233, 3, TFT_GREEN);
  }
}

// =====================================================
// UI MENU
// =====================================================

void drawTitle(const char* title, bool withBack) {
  tft.fillRect(0, 0, 320, 36, TFT_BLACK);

  if (withBack) {
    tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);
    addButton(0, 0, 45, 36, "BACK", -1);
  }

  menuLoadTitleFont();

  teDrawString(
    String(title),
    160,
    18,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  menuFinishFont();
}

void drawMenuButton(Button b, const char* iconPath) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, TFT_WHITE);

  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_BLACK);
  tft.drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 8, TFT_BLACK);

  drawJpgFromSD(iconPath, b.x + 6, b.y + 6);

  menuLoadButtonFont();

  teDrawString(
    b.label,
    b.x + 58,
    b.y + 27,
    TFT_BLACK,
    TFT_WHITE,
    ML_DATUM,
    2
  );

  menuFinishFont();
}

void drawPageControls(int page, int totalPages) {
  if (totalPages <= 1) return;

  int y = 229;

  if (page > 0) {
    tft.fillTriangle(262, y, 272, y - 6, 272, y + 6, TFT_WHITE);
    addButton(254, 218, 28, 22, "PAGE_PREV", -20);
  }

  if (page < totalPages - 1) {
    tft.fillTriangle(306, y, 296, y - 6, 296, y + 6, TFT_WHITE);
    addButton(288, 218, 28, 22, "PAGE_NEXT", -21);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(String(page + 1) + "/" + String(totalPages), 284, 229, 1);
}

// =====================================================
// MENU PRINCIPAL
// =====================================================

void drawMainMenu() {
  appMode = MODE_MENU;
  currentMenu = -1;
  currentPage = 0;
  buttonCount = 0;

  tft.fillScreen(TFT_BLACK);
  drawTitle("CYD", false);

  if (!startSD()) {
    drawBoldText("Erreur SD", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    stopSDStartTouch();
    return;
  }

  for (int i = 0; i < menuCount; i++) {
    int x = (i % 2 == 0) ? 10 : 165;
    int y = 44 + (i / 2) * 62;

    addButton(x, y, 145, 55, String(menuNames[i]), i);

    drawMenuButton(
      buttons[buttonCount - 1],
      getMainIcon(i)
    );
  }

  drawWifiIndicator();

  stopSDStartTouch();
}

// =====================================================
// SOUS-MENU PAGINE
// =====================================================

void drawSubMenuPage(int menuIndex, int page) {
  appMode = MODE_MENU;
  currentMenu = menuIndex;
  currentPage = page;
  buttonCount = 0;

  tft.fillScreen(TFT_BLACK);
  drawTitle(menuNames[menuIndex], true);

  if (!startSD()) {
    drawBoldText("Erreur SD", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    stopSDStartTouch();
    return;
  }

  int appCount = getAppCount(menuIndex);
  int totalPages = (appCount + APPS_PER_PAGE - 1) / APPS_PER_PAGE;

  if (totalPages < 1) totalPages = 1;

  if (currentPage < 0) currentPage = 0;
  if (currentPage >= totalPages) currentPage = totalPages - 1;

  int first = currentPage * APPS_PER_PAGE;
  int last = first + APPS_PER_PAGE;
  if (last > appCount) last = appCount;

  int visibleIndex = 0;

  for (int i = first; i < last; i++) {
    int x = (visibleIndex % 2 == 0) ? 10 : 165;
    int y = 44 + (visibleIndex / 2) * 62;

    String label = getAppName(menuIndex, i);
    const char* iconPath = getAppIcon(menuIndex, i);

    addButton(x, y, 145, 55, label, i);

    drawMenuButton(
      buttons[buttonCount - 1],
      iconPath
    );

    visibleIndex++;
  }

  drawPageControls(currentPage, totalPages);

  stopSDStartTouch();
}

void drawSubMenu(int menuIndex) {
  drawSubMenuPage(menuIndex, 0);
}

// =====================================================
// OUVERTURE APPLICATIONS
// =====================================================

void openApplication(String label) {
  if (launchAppByLabel(label)) {
    return;
  }

  tft.fillScreen(TFT_BLACK);
  buttonCount = 0;

  drawTitle(label.c_str(), true);

  drawBoldText("Application :", 160, 95, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
  drawBoldText(label, 160, 130, TFT_WHITE, TFT_BLACK, MC_DATUM, 4);
}

// =====================================================
// UPDATE MENU
// =====================================================

void updateMenuTouch() {
  int x, y;

  if (!getTouch(x, y)) return;

  for (int i = 0; i < buttonCount; i++) {
    if (!inButton(buttons[i], x, y)) continue;

    drawPressed(buttons[i]);
    delay(80);

    if (currentMenu == -1) {
      drawSubMenu(buttons[i].index);
    }
    else {
      if (buttons[i].index == -1) {
        drawMainMenu();
      }
      else if (buttons[i].index == -20) {
        drawSubMenuPage(currentMenu, currentPage - 1);
      }
      else if (buttons[i].index == -21) {
        drawSubMenuPage(currentMenu, currentPage + 1);
      }
      else {
        openApplication(buttons[i].label);
      }
    }

    while (ts.touched()) {
      delay(5);
    }

    delay(80);
    return;
  }
}