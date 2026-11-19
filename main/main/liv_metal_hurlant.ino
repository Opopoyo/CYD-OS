#include "liv_metal_hurlant.h"
#include "menu_core.h"
#include "text_engine.h"

#define MH_PATH "/livres/metal_urgen/"
#define MH_FONT_TEXT  "Papernotes.vlw"
#define MH_FONT_TITLE "Papernotes.vlw"

#define MH_MAX_TEXT 6200
#define MH_MAX_CHOICES 4

// Moins de caractères par page = texte plus grand / plus lisible
#define MH_PAGE_CHARS 260

String mhSceneId = "s000";
String mhTitle = "Metal Urgen";
String mhText = "";

String mhChoiceLabel[MH_MAX_CHOICES];
String mhChoiceTarget[MH_MAX_CHOICES];

int mhChoiceCount = 0;
int mhPage = 0;

bool mhChoiceScreen = false;
unsigned long mhLastTouch = 0;

// Couleurs mode clair
uint16_t MH_BG;
uint16_t MH_PANEL;
uint16_t MH_TEXT;
uint16_t MH_TITLE;
uint16_t MH_BTN;
uint16_t MH_BTN_TEXT;

// =====================================================
// SD
// =====================================================

String mhFilePath(String id) {
  id.trim();
  return String(MH_PATH) + id + ".txt";
}

String mhReadFile(String path) {
  String out = "";

  if (!startSD()) {
    stopSDStartTouch();
    return "";
  }

  File f = SD.open(path, FILE_READ);

  if (!f) {
    stopSDStartTouch();
    return "";
  }

  while (f.available() && out.length() < MH_MAX_TEXT) {
    out += (char)f.read();
  }

  f.close();
  stopSDStartTouch();

  return out;
}

// =====================================================
// SCENES
// =====================================================

void mhClearChoices() {
  mhChoiceCount = 0;

  for (int i = 0; i < MH_MAX_CHOICES; i++) {
    mhChoiceLabel[i] = "";
    mhChoiceTarget[i] = "";
  }
}

void mhParseChoice(String line) {
  if (mhChoiceCount >= MH_MAX_CHOICES) return;

  int eq = line.indexOf('=');
  int sep = line.lastIndexOf('|');

  if (eq < 0 || sep < 0) return;

  String label = line.substring(eq + 1, sep);
  String target = line.substring(sep + 1);

  label.trim();
  target.trim();

  mhChoiceLabel[mhChoiceCount] = label;
  mhChoiceTarget[mhChoiceCount] = target;
  mhChoiceCount++;
}

void mhParseScene(String raw) {
  mhTitle = "Metal Urgen";
  mhText = "";
  mhClearChoices();

  raw.replace("\r", "");

  int pos = 0;

  while (pos < raw.length()) {
    int next = raw.indexOf('\n', pos);
    if (next < 0) next = raw.length();

    String line = raw.substring(pos, next);
    String trimLine = line;
    trimLine.trim();

    if (trimLine.startsWith("TITLE=")) {
      mhTitle = trimLine.substring(6);
      mhTitle.trim();
    }

    else if (trimLine.startsWith("CHOIX")) {
      mhParseChoice(trimLine);
    }

    else {
      mhText += line;
      mhText += "\n";
    }

    pos = next + 1;
  }

  mhText = teClean(mhText);
  mhPage = 0;
  mhChoiceScreen = false;
}

bool mhLoadScene(String id) {
  id.trim();

  String path = mhFilePath(id);
  String raw = mhReadFile(path);

  if (raw.length() == 0) {
    mhSceneId = id;
    mhTitle = "Erreur SD";
    mhText =
      "Impossible de charger :\n\n" +
      path +
      "\n\nVerifie que le dossier existe :\n/livres/metal_urgen/";

    mhClearChoices();
    mhPage = 0;
    mhChoiceScreen = false;
    return false;
  }

  mhSceneId = id;
  mhParseScene(raw);
  return true;
}

// =====================================================
// POLICES
// =====================================================

void mhLoadTextFont() {
  if (!teLoadFont(MH_FONT_TEXT)) {
    teSetFont(2);
  }
}

void mhLoadTitleFont() {
  if (!teLoadFont(MH_FONT_TITLE)) {
    teSetFont(2);
  }
}

// =====================================================
// PAGINATION
// =====================================================

int mhPageCount() {
  return tePageCount(mhText, MH_PAGE_CHARS);
}

String mhPageText() {
  return teGetPage(mhText, mhPage, MH_PAGE_CHARS);
}

// =====================================================
// DESIGN CLAIR
// =====================================================

void mhInitColors() {
  MH_BG       = tft.color565(236, 229, 207);
  MH_PANEL    = tft.color565(255, 250, 232);
  MH_TEXT     = tft.color565(24, 21, 18);
  MH_TITLE    = tft.color565(80, 35, 20);
  MH_BTN      = tft.color565(90, 62, 45);
  MH_BTN_TEXT = TFT_WHITE;
}

void mhBackground() {
  tft.fillScreen(MH_BG);

  // Petit effet papier
  for (int i = 0; i < 80; i++) {
    int x = random(0, 320);
    int y = random(32, 214);
    uint16_t c = tft.color565(220 + random(0, 20), 210 + random(0, 18), 190 + random(0, 18));
    tft.drawPixel(x, y, c);
  }
}

void mhButton(int x, int y, int w, int h, String label, uint16_t bg, uint16_t fg) {
  tft.fillRoundRect(x, y, w, h, 7, bg);
  tft.drawRoundRect(x, y, w, h, 7, tft.color565(70, 50, 35));

  teDrawString(
    label,
    x + w / 2,
    y + h / 2,
    fg,
    bg,
    MC_DATUM,
    1
  );
}

void mhHeader() {
  tft.fillRect(0, 0, 320, 34, tft.color565(56, 38, 26));

  // Triangle retour
  tft.fillTriangle(10, 17, 31, 6, 31, 28, TFT_WHITE);

  mhLoadTitleFont();

  String t = mhTitle;
  if (t.length() > 24) {
    t = t.substring(0, 24);
  }

  teDrawString(
    t,
    170,
    17,
    TFT_WHITE,
    tft.color565(56, 38, 26),
    MC_DATUM,
    1
  );

  mhLoadTextFont();
}

void mhFooter() {
  tft.fillRect(0, 214, 320, 26, tft.color565(56, 38, 26));

  // Boutons petits en bas à droite
  mhButton(214, 217, 31, 20, "<", MH_BTN, MH_BTN_TEXT);
  mhButton(250, 217, 31, 20, ">", MH_BTN, MH_BTN_TEXT);

  teDrawString(
    String(mhPage + 1) + "/" + String(mhPageCount()),
    302,
    227,
    TFT_WHITE,
    tft.color565(56, 38, 26),
    MC_DATUM,
    1
  );

  // Le bouton choix apparaît uniquement à la dernière page
  if (mhChoiceCount > 0 && mhPage >= mhPageCount() - 1) {
    mhButton(8, 217, 88, 20, "Choix", tft.color565(120, 55, 80), TFT_WHITE);
  }
}

void mhDrawPage() {
  mhChoiceScreen = false;

  mhBackground();
  mhHeader();

  // Grande page claire
  tft.fillRoundRect(6, 39, 308, 170, 8, MH_PANEL);
  tft.drawRoundRect(6, 39, 308, 170, 8, tft.color565(190, 170, 130));

  mhLoadTextFont();

  teDrawWrapped(
    mhPageText(),
    14,
    49,
    292,
    198,
    25,        // lignes plus espacées
    MH_TEXT,
    MH_PANEL,
    2
  );

  mhFooter();
}

void mhDrawChoices() {
  mhChoiceScreen = true;

  mhBackground();
  mhHeader();

  tft.fillRoundRect(10, 42, 300, 166, 10, MH_PANEL);
  tft.drawRoundRect(10, 42, 300, 166, 10, tft.color565(170, 130, 90));

  mhLoadTitleFont();

  teDrawString(
    "Que suivez-vous ?",
    160,
    64,
    MH_TITLE,
    MH_PANEL,
    MC_DATUM,
    1
  );

  mhLoadTextFont();

  for (int i = 0; i < mhChoiceCount; i++) {
    int y = 84 + i * 34;

    mhButton(
      22,
      y,
      276,
      29,
      mhChoiceLabel[i],
      tft.color565(90, 62, 45),
      TFT_WHITE
    );
  }

  mhButton(100, 216, 120, 20, "Retour page", tft.color565(70, 90, 120), TFT_WHITE);
}

// =====================================================
// ACTIONS
// =====================================================

void mhExit() {
  teUnloadFont();
  drawSubMenu(currentMenu);
}

void mhGoChoice(int i) {
  if (i < 0 || i >= mhChoiceCount) return;

  String target = mhChoiceTarget[i];
  target.trim();

  mhLoadScene(target);
  mhDrawPage();
}

// =====================================================
// LANCEMENT
// =====================================================

void launchMetalHurlant() {
  appMode = MODE_METAL_HURLANT;

  stopSDStartTouch();

  randomSeed(millis());
  mhInitColors();

  mhLoadTextFont();

  mhSceneId = "s000";
  mhLoadScene(mhSceneId);

  mhDrawPage();

  Serial.println("METAL URGEN PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateMetalHurlant() {
  int x, y;

  if (!getTouch(x, y)) return;
  if (millis() - mhLastTouch < 170) return;

  mhLastTouch = millis();

  // Retour menu
  if (y < 34 && x < 45) {
    mhExit();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (mhChoiceScreen) {
    for (int i = 0; i < mhChoiceCount; i++) {
      int by = 84 + i * 34;

      if (x >= 22 && x <= 298 && y >= by && y <= by + 29) {
        mhGoChoice(i);

        while (ts.touched()) delay(5);
        delay(80);
        return;
      }
    }

    if (x >= 100 && x <= 220 && y >= 216 && y <= 236) {
      mhDrawPage();

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }
  }

  else {
    // Page précédente
    if (x >= 214 && x <= 245 && y >= 217 && y <= 237) {
      mhPage--;
      if (mhPage < 0) mhPage = 0;
      mhDrawPage();
    }

    // Page suivante
    else if (x >= 250 && x <= 281 && y >= 217 && y <= 237) {
      mhPage++;
      if (mhPage >= mhPageCount()) {
        mhPage = mhPageCount() - 1;
      }

      mhDrawPage();
    }

    // Choix uniquement dernière page
    else if (
      mhChoiceCount > 0 &&
      mhPage >= mhPageCount() - 1 &&
      x >= 8 && x <= 96 &&
      y >= 217 && y <= 237
    ) {
      mhDrawChoices();
    }
  }

  while (ts.touched()) delay(5);
  delay(80);
}