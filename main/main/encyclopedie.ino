#include "encyclopedie.h"
#include "menu_core.h"
#include "wifi_app.h"
#include "text_engine.h"

#include <WiFi.h>

#define ENC_MAX_RESULTS 5
#define ENC_PAGE_CHARS 420
#define ENC_FONT 2

#define ENC_ARTICLE_FONT "Roman14.vlw"

enum EncState {
  ENC_SEARCH,
  ENC_RESULTS,
  ENC_ARTICLE
};

EncState encState = ENC_SEARCH;

String encQuery = "";
String encLastQuery = "";

String encResults[ENC_MAX_RESULTS];
int encResultPageIds[ENC_MAX_RESULTS];
int encResultCount = 0;

String encArticleTitle = "";
String encArticleText = "";
int encPage = 0;

unsigned long encLastTouch = 0;

EncKey encKeys[34];
int encKeyCount = 0;

const char* encRow1[] = {"A","Z","E","R","T","Y","U","I","O","P"};
const char* encRow2[] = {"Q","S","D","F","G","H","J","K","L","M"};
const char* encRow3[] = {"W","X","C","V","B","N"};

bool encSearchWiki();
bool encLoadArticleByPageId(int pageid, String fallbackTitle);

void encDrawSearch();
void encDrawResults();
void encDrawArticle();
void encDoSearch();
void encOpenResult(int index);

// =====================================================
// PAGES UTF-8 SAFE
// =====================================================

String encSafeUtf8Page(String s, int page, int charsPerPage) {
  int start = page * charsPerPage;
  if (start >= s.length()) return "";

  int end = start + charsPerPage;
  if (end > s.length()) end = s.length();

  while (start > 0 && start < s.length() && (((uint8_t)s[start] & 0xC0) == 0x80)) {
    start++;
  }

  while (end > start && end < s.length() && (((uint8_t)s[end] & 0xC0) == 0x80)) {
    end--;
  }

  return s.substring(start, end);
}

int encSafePageCount(String s, int charsPerPage) {
  int n = s.length() / charsPerPage;
  if (s.length() % charsPerPage) n++;
  if (n < 1) n = 1;
  return n;
}

// =====================================================
// CLAVIER SIMPLE
// =====================================================

void encAddKey(int x, int y, int w, int h, String label) {
  if (encKeyCount >= 34) return;

  encKeys[encKeyCount].x = x;
  encKeys[encKeyCount].y = y;
  encKeys[encKeyCount].w = w;
  encKeys[encKeyCount].h = h;
  encKeys[encKeyCount].label = label;
  encKeyCount++;
}

bool encInKey(EncKey k, int x, int y) {
  return x >= k.x && x <= k.x + k.w &&
         y >= k.y && y <= k.y + k.h;
}

// =====================================================
// UI STANDARD
// =====================================================

uint16_t encBg(int y) {
  return tft.color565(
    constrain(8 + y / 20, 0, 35),
    constrain(10 + y / 18, 0, 50),
    constrain(35 + y / 4, 45, 120)
  );
}

void encUseStandardFont() {
  teSetFont(ENC_FONT);
}

void encBackground() {
  for (int y = 0; y < 240; y += 4) {
    tft.fillRect(0, y, 320, 4, encBg(y));
  }
}

void encHeader(String title) {
  encUseStandardFont();

  tft.fillRect(0, 0, 320, 32, TFT_BLACK);
  tft.fillTriangle(11, 16, 31, 5, 31, 27, TFT_WHITE);

  teDrawString(title, 160, 16, TFT_WHITE, TFT_BLACK, MC_DATUM, ENC_FONT);
}

void encButton(int x, int y, int w, int h, String label, uint16_t bg, uint16_t fg = TFT_WHITE) {
  encUseStandardFont();

  tft.fillRoundRect(x, y, w, h, 7, bg);
  tft.drawRoundRect(x, y, w, h, 7, TFT_DARKGREY);

  teDrawString(label, x + w / 2, y + h / 2, fg, bg, MC_DATUM, ENC_FONT);
}

void encExitToMenu() {
  teUnloadFont();
  drawSubMenu(currentMenu);
}

void encLoadingScreen(String msg) {
  encUseStandardFont();

  tft.fillScreen(TFT_BLACK);
  tft.fillTriangle(11, 16, 31, 5, 31, 27, TFT_WHITE);

  teDrawString(msg, 160, 104, TFT_CYAN, TFT_BLACK, MC_DATUM, ENC_FONT);
  teDrawString("Veuillez patienter", 160, 132, TFT_WHITE, TFT_BLACK, MC_DATUM, ENC_FONT);
}

// =====================================================
// RECHERCHE
// =====================================================

void encDrawSearch() {
  encUseStandardFont();

  encState = ENC_SEARCH;
  encKeyCount = 0;

  encBackground();
  encHeader("Encyclopédie");

  tft.fillRoundRect(10, 38, 300, 34, 8, TFT_BLACK);
  tft.drawRoundRect(10, 38, 300, 34, 8, TFT_CYAN);

  String shown = encQuery;
  if (shown.length() > 24) shown = shown.substring(shown.length() - 24);

  teDrawString(shown, 160, 55, TFT_YELLOW, TFT_BLACK, MC_DATUM, ENC_FONT);

  int keyW = 28;
  int keyH = 24;
  int gap = 3;
  int y1 = 82;

  for (int i = 0; i < 10; i++) encAddKey(5 + i * (keyW + gap), y1, keyW, keyH, encRow1[i]);
  for (int i = 0; i < 10; i++) encAddKey(15 + i * (keyW + gap), y1 + 30, keyW, keyH, encRow2[i]);

  encAddKey(5, y1 + 60, 45, keyH, "MAJ");

  for (int i = 0; i < 6; i++) encAddKey(55 + i * (keyW + gap), y1 + 60, keyW, keyH, encRow3[i]);

  encAddKey(245, y1 + 60, 70, keyH, "<-");

  encAddKey(5,   205, 65, 28, "ESPACE");
  encAddKey(75,  205, 65, 28, "CLR");
  encAddKey(145, 205, 80, 28, "OK");
  encAddKey(232, 205, 78, 28, "RETOUR");

  for (int i = 0; i < encKeyCount; i++) {
    uint16_t bg = TFT_WHITE;
    uint16_t fg = TFT_BLACK;

    if (encKeys[i].label == "OK") bg = TFT_GREEN;
    if (encKeys[i].label == "<-") bg = TFT_RED;
    if (encKeys[i].label == "CLR") bg = TFT_ORANGE;
    if (encKeys[i].label == "ESPACE") bg = TFT_CYAN;
    if (encKeys[i].label == "RETOUR") bg = TFT_DARKGREY;
    if (encKeys[i].label == "MAJ") bg = TFT_YELLOW;

    encButton(encKeys[i].x, encKeys[i].y, encKeys[i].w, encKeys[i].h, encKeys[i].label, bg, fg);
  }

  teDrawString("Wikipédia FR", 160, 236, TFT_LIGHTGREY, encBg(236), MC_DATUM, 1);
}

void encDoSearch() {
  if (WiFi.status() != WL_CONNECTED) {
    teUnloadFont();
    launchWifiApp();
    return;
  }

  encQuery.trim();
  encLastQuery = encQuery;

  encLoadingScreen("Recherche...");

  bool ok = encSearchWiki();
  if (!ok) encResultCount = 0;

  encDrawResults();
}

// =====================================================
// RESULTATS
// =====================================================

void encDrawResults() {
  encUseStandardFont();

  encState = ENC_RESULTS;

  encBackground();
  encHeader("Résultats");

  if (encResultCount == 0) {
    teDrawString("Aucun résultat", 160, 80, TFT_RED, encBg(80), MC_DATUM, ENC_FONT);
    teDrawString("Relance la recherche", 160, 108, TFT_WHITE, encBg(108), MC_DATUM, ENC_FONT);

    encButton(42, 150, 108, 34, "ACTUALISER", TFT_GREEN, TFT_BLACK);
    encButton(170, 150, 108, 34, "RETOUR", TFT_DARKGREY);
    return;
  }

  for (int i = 0; i < encResultCount; i++) {
    int y = 42 + i * 34;

    tft.fillRoundRect(10, y, 300, 29, 7, TFT_BLACK);
    tft.drawRoundRect(10, y, 300, 29, 7, TFT_CYAN);

    String name = encResults[i];
    if (name.length() > 30) name = name.substring(0, 30);

    teDrawString(String(i + 1) + ". " + name, 160, y + 15, TFT_WHITE, TFT_BLACK, MC_DATUM, ENC_FONT);
  }

  encButton(12,  212, 82, 24, "Retour", TFT_DARKGREY);
  encButton(102, 212, 96, 24, "Actualiser", TFT_GREEN, TFT_BLACK);
  encButton(206, 212, 96, 24, "Chercher", TFT_BLUE);
}

void encOpenResult(int index) {
  if (index < 0 || index >= encResultCount) return;

  if (WiFi.status() != WL_CONNECTED) {
    teUnloadFont();
    launchWifiApp();
    return;
  }

  encLoadingScreen("Chargement...");

  bool ok = encLoadArticleByPageId(encResultPageIds[index], encResults[index]);

  if (!ok) {
    delay(350);
    ok = encLoadArticleByPageId(encResultPageIds[index], encResults[index]);
  }

  if (!ok) {
    encArticleTitle = encResults[index];
    encArticleText = "Erreur de chargement. Retourne aux résultats puis appuie sur Actualiser.";
  }

  encPage = 0;
  encDrawArticle();
}

// =====================================================
// ARTICLE AVEC ROMAN14.VLW
// =====================================================

int encPageCount() {
  return encSafePageCount(encArticleText, ENC_PAGE_CHARS);
}

String encPageText() {
  return encSafeUtf8Page(encArticleText, encPage, ENC_PAGE_CHARS);
}

void encDrawArticleFooter(uint16_t bar) {
  encUseStandardFont();

  tft.fillRect(0, 215, 320, 25, bar);

  encButton(8,   217, 56, 20, "<", TFT_DARKGREY);
  encButton(72,  217, 56, 20, ">", TFT_DARKGREY);
  encButton(136, 217, 72, 20, "Result", TFT_BLUE);
  encButton(216, 217, 96, 20, String(encPage + 1) + "/" + String(encPageCount()), bar);
}

void encDrawArticle() {
  encState = ENC_ARTICLE;

  uint16_t bg       = tft.color565(238, 232, 214);
  uint16_t panel    = tft.color565(255, 250, 235);
  uint16_t textCol  = tft.color565(24, 20, 16);
  uint16_t bar      = tft.color565(58, 42, 30);
  uint16_t frame    = tft.color565(180, 155, 115);

  tft.fillScreen(bg);

  // Barre haute
  tft.fillRect(0, 0, 320, 30, bar);
  tft.fillTriangle(9, 15, 28, 5, 28, 25, TFT_WHITE);

  encUseStandardFont();

  String title = encArticleTitle;
  if (title.length() > 30) title = title.substring(0, 30);

  teDrawString(title, 168, 15, TFT_WHITE, bar, MC_DATUM, ENC_FONT);

  // Page article
  tft.fillRoundRect(5, 34, 310, 176, 8, panel);
  tft.drawRoundRect(5, 34, 310, 176, 8, frame);

  // Police .vlw uniquement pour l'article
  if (!teLoadFont(ENC_ARTICLE_FONT)) {
    teSetFont(ENC_FONT);
  }

  teDrawWrapped(
    encPageText(),
    12,
    43,
    296,
    200,
    19,
    textCol,
    panel,
    ENC_FONT
  );

  // Important : on libère la SD pour le tactile après dessin Roman14
  teFinishDraw();

  // Footer en police standard
  encDrawArticleFooter(bar);
}

// =====================================================
// LANCEMENT
// =====================================================

void launchEncyclopedie() {
  if (WiFi.status() != WL_CONNECTED) {
    launchWifiApp();
    return;
  }

  appMode = MODE_ENCYCLOPEDIE;

  stopSDStartTouch();
  encUseStandardFont();

  encQuery = "";
  encLastQuery = "";
  encResultCount = 0;
  encArticleText = "";
  encArticleTitle = "";
  encPage = 0;

  for (int i = 0; i < ENC_MAX_RESULTS; i++) {
    encResultPageIds[i] = 0;
  }

  encDrawSearch();

  Serial.println("ENCYCLOPEDIE PRETE - ARTICLES EN ROMAN14");
}

// =====================================================
// UPDATE
// =====================================================

void updateEncyclopedie() {
  int x, y;

  if (!getTouch(x, y)) return;
  if (millis() - encLastTouch < 180) return;

  encLastTouch = millis();

  if (y < 32 && x < 45) {
    encExitToMenu();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (encState == ENC_SEARCH) {
    for (int i = 0; i < encKeyCount; i++) {
      if (!encInKey(encKeys[i], x, y)) continue;

      String k = encKeys[i].label;

      if (k == "OK") {
        encDoSearch();
      }
      else if (k == "RETOUR") {
        encExitToMenu();
      }
      else if (k == "<-") {
        if (encQuery.length() > 0) encQuery.remove(encQuery.length() - 1);
        encDrawSearch();
      }
      else if (k == "CLR") {
        encQuery = "";
        encDrawSearch();
      }
      else if (k == "ESPACE") {
        encQuery += " ";
        encDrawSearch();
      }
      else if (k == "MAJ") {
        // simple pour cette version
      }
      else {
        encQuery += k;
        encDrawSearch();
      }

      break;
    }
  }

  else if (encState == ENC_RESULTS) {
    if (encResultCount == 0) {
      if (x >= 42 && x <= 150 && y >= 150 && y <= 184) {
        encQuery = encLastQuery;
        encDoSearch();
      }
      else if (x >= 170 && x <= 278 && y >= 150 && y <= 184) {
        encDrawSearch();
      }
    }

    for (int i = 0; i < encResultCount; i++) {
      int by = 42 + i * 34;

      if (x >= 10 && x <= 310 && y >= by && y <= by + 29) {
        encOpenResult(i);
        break;
      }
    }

    if (x >= 12 && x <= 94 && y >= 212 && y <= 236) {
      encDrawSearch();
    }

    else if (x >= 102 && x <= 198 && y >= 212 && y <= 236) {
      encQuery = encLastQuery;
      encDoSearch();
    }

    else if (x >= 206 && x <= 302 && y >= 212 && y <= 236) {
      encDrawSearch();
    }
  }

  else if (encState == ENC_ARTICLE) {
    if (x >= 8 && x <= 64 && y >= 217 && y <= 237) {
      encPage--;
      if (encPage < 0) encPage = 0;
      encDrawArticle();
    }

    else if (x >= 72 && x <= 128 && y >= 217 && y <= 237) {
      encPage++;
      if (encPage >= encPageCount()) encPage = encPageCount() - 1;
      encDrawArticle();
    }

    else if (x >= 136 && x <= 208 && y >= 217 && y <= 237) {
      encDrawResults();
    }
  }

  while (ts.touched()) delay(5);
  delay(80);
}