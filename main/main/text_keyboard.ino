#include "text_engine.h"
#include "menu_core.h"

#define TE_KB_MAX_BTNS 64

TEButton teKbBtns[TE_KB_MAX_BTNS];
int teKbCount = 0;

TEKeyboardMode teKbMode = TE_KB_LETTERS;
String teKbTitle = "";

unsigned long teKbLastTouch = 0;

const char* teLow1[] = {"a","z","e","r","t","y","u","i","o","p"};
const char* teLow2[] = {"q","s","d","f","g","h","j","k","l","m"};
const char* teLow3[] = {"w","x","c","v","b","n"};

const char* teMaj1[] = {"A","Z","E","R","T","Y","U","I","O","P"};
const char* teMaj2[] = {"Q","S","D","F","G","H","J","K","L","M"};
const char* teMaj3[] = {"W","X","C","V","B","N"};

const char* teNum1[] = {"1","2","3","4","5","6","7","8","9","0"};
const char* teNum2[] = {".",",","?","!","'","\"","-","_","(",")"};
const char* teNum3[] = {":",";","@","#","&","/"};

const char* teAcc1[] = {"é","è","ê","ë","à","â","ù","û","î","ï"};
const char* teAcc2[] = {"ô","ö","ç","É","È","À","Ç","æ","œ","'"};
const char* teAcc3[] = {"^","¨","`","´","~","°"};

const char* teCode1[] = {"<",">","{","}","[","]","=","+","*","%"};
const char* teCode2[] = {"\\","|","~","^","$","`","€","§","µ","@"};
const char* teCode3[] = {".",",",";",":","!","?"};

void teKbAdd(int x, int y, int w, int h, String label, int id) {
  if (teKbCount >= TE_KB_MAX_BTNS) return;

  teKbBtns[teKbCount].x = x;
  teKbBtns[teKbCount].y = y;
  teKbBtns[teKbCount].w = w;
  teKbBtns[teKbCount].h = h;
  teKbBtns[teKbCount].label = label;
  teKbBtns[teKbCount].id = id;
  teKbCount++;
}

bool teKbIn(TEButton b, int x, int y) {
  return x >= b.x && x <= b.x + b.w &&
         y >= b.y && y <= b.y + b.h;
}

void teKbButton(TEButton b, bool pressed = false) {
  uint16_t bg = TFT_WHITE;
  uint16_t fg = TFT_BLACK;

  if (b.label == "OK") bg = TFT_GREEN;
  if (b.label == "ANN") bg = TFT_RED;
  if (b.label == "<-") bg = TFT_ORANGE;
  if (b.label == "MAJ") bg = TFT_CYAN;
  if (b.label == "abc") bg = TFT_CYAN;
  if (b.label == "123") bg = TFT_YELLOW;
  if (b.label == "ABC") bg = TFT_YELLOW;
  if (b.label == "ACC") bg = TFT_PURPLE;
  if (b.label == "CODE") bg = TFT_BLUE;
  if (b.label == "ESPACE") bg = TFT_CYAN;
  if (b.label == "ENTER") bg = TFT_PURPLE;

  if (pressed) bg = TFT_LIGHTGREY;

  tft.fillRoundRect(b.x, b.y, b.w, b.h, 5, bg);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 5, TFT_DARKGREY);
  teDrawString(b.label, b.x + b.w / 2, b.y + b.h / 2, fg, bg, MC_DATUM, 1);
}

void teKbInputBox() {
  tft.fillRoundRect(5, 31, 310, 36, 7, TFT_BLACK);
  tft.drawRoundRect(5, 31, 310, 36, 7, TFT_CYAN);

  String shown = teKeyboardValue;
  if (shown.length() > 58) shown = shown.substring(shown.length() - 58);

  teDrawString(shown, 12, 49, TFT_WHITE, TFT_BLACK, ML_DATUM, 1);
}

String teKbGetLabelRow1(int i) {
  if (teKbMode == TE_KB_LETTERS) return teLow1[i];
  if (teKbMode == TE_KB_MAJ)     return teMaj1[i];
  if (teKbMode == TE_KB_NUM)     return teNum1[i];
  if (teKbMode == TE_KB_ACCENTS) return teAcc1[i];
  return teCode1[i];
}

String teKbGetLabelRow2(int i) {
  if (teKbMode == TE_KB_LETTERS) return teLow2[i];
  if (teKbMode == TE_KB_MAJ)     return teMaj2[i];
  if (teKbMode == TE_KB_NUM)     return teNum2[i];
  if (teKbMode == TE_KB_ACCENTS) return teAcc2[i];
  return teCode2[i];
}

String teKbGetLabelRow3(int i) {
  if (teKbMode == TE_KB_LETTERS) return teLow3[i];
  if (teKbMode == TE_KB_MAJ)     return teMaj3[i];
  if (teKbMode == TE_KB_NUM)     return teNum3[i];
  if (teKbMode == TE_KB_ACCENTS) return teAcc3[i];
  return teCode3[i];
}

void teKeyboardDraw() {
  teKbCount = 0;

  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, 320, 28, TFT_BLACK);
  tft.fillTriangle(11, 15, 30, 5, 30, 25, TFT_WHITE);

  teDrawString(teKbTitle, 160, 14, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  teKbAdd(270, 3, 45, 23, "OK", 900);
  teKbButton(teKbBtns[teKbCount - 1]);

  teKbInputBox();

  int keyW = 28;
  int keyH = 24;
  int gap = 3;
  int y1 = 74;

  for (int i = 0; i < 10; i++) {
    teKbAdd(5 + i * (keyW + gap), y1, keyW, keyH, teKbGetLabelRow1(i), i);
  }

  for (int i = 0; i < 10; i++) {
    teKbAdd(15 + i * (keyW + gap), y1 + 30, keyW, keyH, teKbGetLabelRow2(i), 20 + i);
  }

  String leftMode = "MAJ";
  if (teKbMode == TE_KB_MAJ) leftMode = "abc";
  else if (teKbMode == TE_KB_NUM) leftMode = "ACC";
  else if (teKbMode == TE_KB_ACCENTS) leftMode = "CODE";
  else if (teKbMode == TE_KB_CODE) leftMode = "123";

  teKbAdd(5, y1 + 60, 45, keyH, leftMode, 801);

  for (int i = 0; i < 6; i++) {
    teKbAdd(55 + i * (keyW + gap), y1 + 60, keyW, keyH, teKbGetLabelRow3(i), 40 + i);
  }

  teKbAdd(245, y1 + 60, 70, keyH, "<-", 802);

  teKbAdd(5,   196, 45, 32, teKbMode == TE_KB_LETTERS || teKbMode == TE_KB_MAJ ? "123" : "ABC", 803);
  teKbAdd(55,  196, 90, 32, "ESPACE", 804);
  teKbAdd(150, 196, 70, 32, "ENTER", 805);
  teKbAdd(225, 196, 45, 32, "ANN", 806);
  teKbAdd(275, 196, 40, 32, "OK", 807);

  for (int i = 0; i < teKbCount; i++) {
    if (i == 0) continue;
    teKbButton(teKbBtns[i]);
  }
}

void teKeyboardOpen(String title, String initialText) {
  teKeyboardChanged = false;
  teKeyboardLastInput = "";
  teKbTitle = title;

  teKeyboardValue = initialText;
  teKeyboardActive = true;
  teKeyboardDone = false;
  teKeyboardCancel = false;

  teKbMode = TE_KB_LETTERS;
  teKbLastTouch = millis();

  teKeyboardDraw();
}

void teKeyboardClose() {
  teKeyboardActive = false;
}

void teKbAddText(String s) {
  if (s == "ESPACE") s = " ";
  if (s == "ENTER") s = "\n";

  teKeyboardValue += s;

  teKeyboardLastInput = s;
  teKeyboardChanged = true;

  teKbInputBox();
}

void teKbBackspace() {
  if (teKeyboardValue.length() <= 0) return;

  int last = teKeyboardValue.length() - 1;

  while (last > 0 && ((uint8_t)teKeyboardValue[last] & 0xC0) == 0x80) {
    last--;
  }

  teKeyboardValue.remove(last);
  teKbInputBox();
}

void teKbProcess(String label) {
  if (label == "OK") {
    teKeyboardDone = true;
    teKeyboardActive = false;
    return;
  }

  if (label == "ANN") {
    teKeyboardCancel = true;
    teKeyboardActive = false;
    return;
  }

  if (label == "<-") {
    teKbBackspace();
    return;
  }

  if (label == "MAJ") {
    teKbMode = TE_KB_MAJ;
    teKeyboardDraw();
    return;
  }

  if (label == "abc") {
    teKbMode = TE_KB_LETTERS;
    teKeyboardDraw();
    return;
  }

  if (label == "123") {
    teKbMode = TE_KB_NUM;
    teKeyboardDraw();
    return;
  }

  if (label == "ABC") {
    teKbMode = TE_KB_LETTERS;
    teKeyboardDraw();
    return;
  }

  if (label == "ACC") {
    teKbMode = TE_KB_ACCENTS;
    teKeyboardDraw();
    return;
  }

  if (label == "CODE") {
    teKbMode = TE_KB_CODE;
    teKeyboardDraw();
    return;
  }

  teKbAddText(label);
}

void teKeyboardUpdate() {
  if (!teKeyboardActive) return;

  int x, y;
  if (!getTouch(x, y)) return;

  if (millis() - teKbLastTouch < 110) return;
  teKbLastTouch = millis();

  if (x < 42 && y < 30) {
    teKeyboardCancel = true;
    teKeyboardActive = false;
    while (ts.touched()) delay(5);
    delay(50);
    return;
  }

  for (int i = 0; i < teKbCount; i++) {
    if (!teKbIn(teKbBtns[i], x, y)) continue;

    teKbButton(teKbBtns[i], true);
    delay(35);

    teKbProcess(teKbBtns[i].label);

    while (ts.touched()) delay(5);
    delay(45);

    return;
  }
}