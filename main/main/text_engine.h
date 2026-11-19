#ifndef TEXT_ENGINE_H
#define TEXT_ENGINE_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#define TE_USE_SD_FONTS 1
#define TE_MAX_CLIPBOARD 512

enum TEFontMode {
  TE_FONT_INTERNAL,
  TE_FONT_SD
};

enum TEKeyboardMode {
  TE_KB_LETTERS,
  TE_KB_MAJ,
  TE_KB_NUM,
  TE_KB_ACCENTS,
  TE_KB_CODE
};

struct TEButton {
  int x;
  int y;
  int w;
  int h;
  String label;
  int id;
};

extern String teKeyboardValue;
extern bool teKeyboardActive;
extern bool teKeyboardDone;
extern bool teKeyboardCancel;
extern bool teKeyboardChanged;
extern String teKeyboardLastInput;

void teBegin();

String teClean(String s);
String teToDisplay(String s);

void teSetFont(uint8_t font);
bool teLoadFont(String fontName);
bool teLoadFontPath(String fontPathNoSlashNoExt);
void teUnloadFont();
void teFinishDraw();

void teDrawString(
  String s,
  int x,
  int y,
  uint16_t fg,
  uint16_t bg,
  uint8_t datum,
  uint8_t font = 2
);

void teDrawWrapped(
  String s,
  int x,
  int y,
  int maxW,
  int maxY,
  int lineH,
  uint16_t fg,
  uint16_t bg,
  uint8_t font = 2
);

int tePageCount(String text, int charsPerPage);
String teGetPage(String text, int page, int charsPerPage);

void teCopy(String s);
String tePaste();
void teClearClipboard();

void teKeyboardOpen(String title, String initialText);
void teKeyboardUpdate();
void teKeyboardClose();
void teKeyboardDraw();

#endif