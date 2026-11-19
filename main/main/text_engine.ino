#include "text_engine.h"
#include "menu_core.h"

// =====================================================
// ETAT GLOBAL
// =====================================================

bool teKeyboardChanged = false;
String teKeyboardLastInput = "";

TEFontMode teFontMode = TE_FONT_INTERNAL;
uint8_t teCurrentFont = 2;
bool teSdFontLoaded = false;
String teLoadedFontName = "";

String teClipboard = "";

String teKeyboardValue = "";
bool teKeyboardActive = false;
bool teKeyboardDone = false;
bool teKeyboardCancel = false;

// =====================================================
// INIT
// =====================================================

void teBegin() {
  teKeyboardChanged = false;
  teKeyboardLastInput = "";

  teFontMode = TE_FONT_INTERNAL;
  teCurrentFont = 2;
  teSdFontLoaded = false;
  teLoadedFontName = "";

  teClipboard = "";

  teKeyboardValue = "";
  teKeyboardActive = false;
  teKeyboardDone = false;
  teKeyboardCancel = false;

  tft.setTextWrap(false, false);

  Serial.println("TEXT_ENGINE PRET");
}

// =====================================================
// NETTOYAGE TEXTE
// =====================================================

String teClean(String s) {
  s.replace("\r", "");
  s.replace("\\n", "\n");
  s.replace("\\\"", "\"");
  s.replace("\\/", "/");

  // On garde les accents UTF-8.
  // On simplifie seulement quelques ponctuations parfois absentes des polices.
  s.replace("’", "'");
  s.replace("‘", "'");
  s.replace("“", "\"");
  s.replace("”", "\"");
  s.replace("–", "-");
  s.replace("—", "-");
  s.replace("…", "...");

  while (s.indexOf("\n\n\n") >= 0) {
    s.replace("\n\n\n", "\n\n");
  }

  return s;
}

// =====================================================
// CONVERSION UNIQUEMENT POUR POLICE INTERNE
// Avec .vlw : on garde UTF-8 intact.
// =====================================================

String teCodepointToDisplay(uint16_t cp) {
  String out = "";

  if (cp < 128) {
    out += (char)cp;
    return out;
  }

  if (cp == 0x00E9) out += (char)130;      // é
  else if (cp == 0x00E2) out += (char)131; // â
  else if (cp == 0x00E4) out += (char)132; // ä
  else if (cp == 0x00E0) out += (char)133; // à
  else if (cp == 0x00E7) out += (char)135; // ç
  else if (cp == 0x00EA) out += (char)136; // ê
  else if (cp == 0x00EB) out += (char)137; // ë
  else if (cp == 0x00E8) out += (char)138; // è
  else if (cp == 0x00EF) out += (char)139; // ï
  else if (cp == 0x00EE) out += (char)140; // î
  else if (cp == 0x00C9) out += (char)144; // É
  else if (cp == 0x00F4) out += (char)147; // ô
  else if (cp == 0x00F6) out += (char)148; // ö
  else if (cp == 0x00FB) out += (char)150; // û
  else if (cp == 0x00F9) out += (char)151; // ù
  else if (cp == 0x00FC) out += (char)129; // ü

  else if (cp == 0x00C0) out += "A";
  else if (cp == 0x00C7) out += "C";
  else if (cp == 0x00C8) out += "E";
  else if (cp == 0x00CA) out += "E";
  else if (cp == 0x00CB) out += "E";
  else if (cp == 0x00CE) out += "I";
  else if (cp == 0x00CF) out += "I";
  else if (cp == 0x00D4) out += "O";
  else if (cp == 0x00D9) out += "U";
  else if (cp == 0x00DB) out += "U";

  else if (cp == 0x0153) out += "oe";
  else if (cp == 0x0152) out += "OE";
  else if (cp == 0x20AC) out += "EUR";
  else if (cp == 0x00B0) out += " deg";
  else out += "?";

  return out;
}

String teToDisplay(String s) {
#if TE_USE_SD_FONTS
  if (teFontMode == TE_FONT_SD && teSdFontLoaded) {
    return s; // IMPORTANT : .vlw reçoit le vrai UTF-8
  }
#endif

  String out = "";

  for (int i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];

    if (c < 128) {
      out += (char)c;
    }

    else if ((c & 0xE0) == 0xC0 && i + 1 < s.length()) {
      uint16_t cp =
        ((c & 0x1F) << 6) |
        ((uint8_t)s[i + 1] & 0x3F);

      out += teCodepointToDisplay(cp);
      i++;
    }

    else if ((c & 0xF0) == 0xE0 && i + 2 < s.length()) {
      uint16_t cp =
        ((c & 0x0F) << 12) |
        (((uint8_t)s[i + 1] & 0x3F) << 6) |
        ((uint8_t)s[i + 2] & 0x3F);

      out += teCodepointToDisplay(cp);
      i += 2;
    }

    else {
      out += "?";
    }
  }

  return out;
}

// =====================================================
// POLICES
// =====================================================

void teSetFont(uint8_t font) {
  teUnloadFont();

  teFontMode = TE_FONT_INTERNAL;
  teCurrentFont = font;

  Serial.print("TE : police interne ");
  Serial.println(font);
}

bool teLoadFont(String fontName) {
  // Exemple :
  // teLoadFont("Papernotes.vlw")
  // devient :
  // systeme/fonts/Papernotes

  fontName.trim();

  fontName.replace("\\", "/");

  if (fontName.startsWith("/")) {
    fontName.remove(0, 1);
  }

  if (fontName.endsWith(".vlw")) {
    fontName.remove(fontName.length() - 4);
  }

  if (fontName.indexOf("/") < 0) {
    fontName = "systeme/fonts/" + fontName;
  }

  return teLoadFontPath(fontName);
}

bool teLoadFontPath(String fontPathNoSlashNoExt) {
#if TE_USE_SD_FONTS
  teUnloadFont();

  fontPathNoSlashNoExt.trim();
  fontPathNoSlashNoExt.replace("\\", "/");

  if (fontPathNoSlashNoExt.startsWith("/")) {
    fontPathNoSlashNoExt.remove(0, 1);
  }

  if (fontPathNoSlashNoExt.endsWith(".vlw")) {
    fontPathNoSlashNoExt.remove(fontPathNoSlashNoExt.length() - 4);
  }

  String realPath = "/" + fontPathNoSlashNoExt + ".vlw";

  if (!startSD()) {
    Serial.println("TE : erreur startSD()");
    return false;
  }

  if (!SD.exists(realPath)) {
    Serial.print("TE : police introuvable : ");
    Serial.println(realPath);

    stopSDStartTouch();
    return false;
  }

  Serial.print("TE : loadFont TFT_eSPI : ");
  Serial.println(fontPathNoSlashNoExt);

  // IMPORTANT TFT_eSPI :
  // pas de / au début
  // pas de .vlw à la fin
  tft.loadFont(fontPathNoSlashNoExt, SD);

  teFontMode = TE_FONT_SD;
  teSdFontLoaded = true;
  teLoadedFontName = fontPathNoSlashNoExt;

  return true;
#else
  Serial.println("TE_USE_SD_FONTS = 0");
  return false;
#endif
}

void teUnloadFont() {
#if TE_USE_SD_FONTS
  if (teSdFontLoaded) {
    tft.unloadFont();
  }
#endif

  teSdFontLoaded = false;
  teFontMode = TE_FONT_INTERNAL;
  teLoadedFontName = "";
}

void teFinishDraw() {
#if TE_USE_SD_FONTS
  if (teSdFontLoaded) {
    // Après avoir fini de dessiner l'écran, on rend le tactile disponible.
    stopSDStartTouch();
  }
#endif
}

// =====================================================
// AFFICHAGE
// =====================================================

void teDrawString(
  String s,
  int x,
  int y,
  uint16_t fg,
  uint16_t bg,
  uint8_t datum,
  uint8_t font
) {
  tft.setTextDatum(datum);
  tft.setTextColor(fg, bg);

  String d = teToDisplay(s);

#if TE_USE_SD_FONTS
  if (teFontMode == TE_FONT_SD && teSdFontLoaded) {
    tft.drawString(d, x, y);
    return;
  }
#endif

  tft.drawString(d, x, y, font);
}

int teTextWidth(String s, uint8_t font) {
  String d = teToDisplay(s);

#if TE_USE_SD_FONTS
  if (teFontMode == TE_FONT_SD && teSdFontLoaded) {
    return tft.textWidth(d);
  }
#endif

  return tft.textWidth(d, font);
}

void teDrawLine(String s, int x, int y, uint8_t font) {
  String d = teToDisplay(s);

#if TE_USE_SD_FONTS
  if (teFontMode == TE_FONT_SD && teSdFontLoaded) {
    tft.drawString(d, x, y);
    return;
  }
#endif

  tft.drawString(d, x, y, font);
}

void teDrawWrapped(
  String s,
  int x,
  int y,
  int maxW,
  int maxY,
  int lineH,
  uint16_t fg,
  uint16_t bg,
  uint8_t font
) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(fg, bg);

  String display = s;
  String word = "";
  String line = "";
  int cursorY = y;

  for (int i = 0; i <= display.length(); i++) {
    char c = (i < display.length()) ? display[i] : ' ';

    if (c == '\n' || c == ' ') {
      String testLine = line;

      if (testLine.length() > 0) testLine += " ";
      testLine += word;

      int tw = teTextWidth(testLine, font);

      if (tw > maxW && line.length() > 0) {
        teDrawLine(line, x, cursorY, font);

        cursorY += lineH;
        if (cursorY > maxY) return;

        line = word;
      }
      else {
        line = testLine;
      }

      word = "";

      if (c == '\n') {
        teDrawLine(line, x, cursorY, font);

        cursorY += lineH;
        line = "";

        if (cursorY > maxY) return;
      }
    }
    else {
      word += c;
    }
  }

  if (line.length() > 0 && cursorY <= maxY) {
    teDrawLine(line, x, cursorY, font);
  }
}

// =====================================================
// PAGINATION UTF-8 SAFE
// =====================================================

int tePageCount(String text, int charsPerPage) {
  if (charsPerPage <= 0) charsPerPage = 300;

  int n = text.length() / charsPerPage;
  if (text.length() % charsPerPage) n++;

  if (n < 1) n = 1;
  return n;
}

String teGetPage(String text, int page, int charsPerPage) {
  if (charsPerPage <= 0) charsPerPage = 300;
  if (page < 0) page = 0;

  int start = page * charsPerPage;
  if (start >= text.length()) return "";

  int end = start + charsPerPage;
  if (end > text.length()) end = text.length();

  while (start > 0 &&
         start < text.length() &&
         (((uint8_t)text[start] & 0xC0) == 0x80)) {
    start++;
  }

  while (end > start &&
         end < text.length() &&
         (((uint8_t)text[end] & 0xC0) == 0x80)) {
    end--;
  }

  return text.substring(start, end);
}

// =====================================================
// COPIER / COLLER
// =====================================================

void teCopy(String s) {
  if (s.length() > TE_MAX_CLIPBOARD) {
    s = s.substring(0, TE_MAX_CLIPBOARD);
  }

  teClipboard = s;
}

String tePaste() {
  return teClipboard;
}

void teClearClipboard() {
  teClipboard = "";
}