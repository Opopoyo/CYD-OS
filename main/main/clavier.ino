#include "clavier.h"

bool kbMajuscule = false;
bool kbModeSpecial = false;
String kbTexte = "";

const char* kbRow1Lower[] = {"a","z","e","r","t","y","u","i","o","p"};
const char* kbRow2Lower[] = {"q","s","d","f","g","h","j","k","l","m"};
const char* kbRow3Lower[] = {"w","x","c","v","b","n"};

const char* kbRow1Upper[] = {"A","Z","E","R","T","Y","U","I","O","P"};
const char* kbRow2Upper[] = {"Q","S","D","F","G","H","J","K","L","M"};
const char* kbRow3Upper[] = {"W","X","C","V","B","N"};

const char* kbSpecial1[] = {"1","2","3","4","5","6","7","8","9","0"};
const char* kbSpecial2[] = {"@","#","&","(",")","-","_","=","+","/"};
const char* kbSpecial3[] = {".",",","?","!",":",";"};

KbButton kbButtons[60];
int kbButtonCount = 0;

void kbAddButton(int x, int y, int w, int h, String label) {
  kbButtons[kbButtonCount].x = x;
  kbButtons[kbButtonCount].y = y;
  kbButtons[kbButtonCount].w = w;
  kbButtons[kbButtonCount].h = h;
  kbButtons[kbButtonCount].label = label;
  kbButtonCount++;
}

bool kbInButton(KbButton b, int x, int y) {
  return x >= b.x && x <= b.x + b.w &&
         y >= b.y && y <= b.y + b.h;
}

void kbDrawButton(KbButton b, bool pressed = false) {
  uint16_t bgColor = TFT_WHITE;
  uint16_t textColor = TFT_BLACK;

  if (b.label == "MAJ") bgColor = TFT_CYAN;
  if (b.label == "123") bgColor = TFT_YELLOW;
  if (b.label == "ABC") bgColor = TFT_YELLOW;
  if (b.label == "ENTER") bgColor = TFT_GREEN;
  if (b.label == "<-") bgColor = TFT_RED;
  if (pressed) bgColor = TFT_LIGHTGREY;

  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, bgColor);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_DARKGREY);

  tft.setTextColor(textColor, bgColor);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(b.label, b.x + b.w / 2, b.y + b.h / 2, 2);
}

void kbDrawTextBox() {
  tft.fillRoundRect(45, 5, 270, 42, 8, TFT_BLACK);
  tft.drawRoundRect(45, 5, 270, 42, 8, TFT_WHITE);

  tft.fillTriangle(11, 25, 31, 14, 31, 36, TFT_WHITE);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  String affichage = kbTexte;
  if (affichage.length() > 24) {
    affichage = affichage.substring(affichage.length() - 24);
  }

  tft.drawString(affichage, 52, 18, 2);
}

void kbBuildKeyboard() {
  kbButtonCount = 0;
  tft.fillScreen(TFT_BLACK);

  kbAddButton(0, 0, 45, 50, "BACK");
  kbDrawTextBox();

  int keyW = 28;
  int keyH = 34;
  int gap = 3;
  int startY = 62;

  for (int i = 0; i < 10; i++) {
    String label = kbModeSpecial ? kbSpecial1[i] : (kbMajuscule ? kbRow1Upper[i] : kbRow1Lower[i]);
    kbAddButton(5 + i * (keyW + gap), startY, keyW, keyH, label);
  }

  for (int i = 0; i < 10; i++) {
    String label = kbModeSpecial ? kbSpecial2[i] : (kbMajuscule ? kbRow2Upper[i] : kbRow2Lower[i]);
    kbAddButton(15 + i * (keyW + gap), startY + 40, keyW, keyH, label);
  }

  kbAddButton(5, startY + 80, 45, keyH, kbModeSpecial ? "ABC" : "MAJ");

  for (int i = 0; i < 6; i++) {
    String label = kbModeSpecial ? kbSpecial3[i] : (kbMajuscule ? kbRow3Upper[i] : kbRow3Lower[i]);
    kbAddButton(55 + i * (keyW + gap), startY + 80, keyW, keyH, label);
  }

  kbAddButton(245, startY + 80, 70, keyH, "<-");
  kbAddButton(5, startY + 120, 45, 38, kbModeSpecial ? "ABC" : "123");
  kbAddButton(55, startY + 120, 140, 38, "ESPACE");
  kbAddButton(205, startY + 120, 105, 38, "ENTER");

  for (int i = 0; i < kbButtonCount; i++) {
    if (kbButtons[i].label != "BACK") {
      kbDrawButton(kbButtons[i]);
    }
  }
}

void kbProcessButton(String label) {
  if (label == "BACK") {
    drawSubMenu(currentMenu);
    return;
  }

  if (label == "MAJ") {
    kbMajuscule = !kbMajuscule;
    kbBuildKeyboard();
    return;
  }

  if (label == "123") {
    kbModeSpecial = true;
    kbBuildKeyboard();
    return;
  }

  if (label == "ABC") {
    kbModeSpecial = false;
    kbBuildKeyboard();
    return;
  }

  if (label == "<-") {
    if (kbTexte.length() > 0) kbTexte.remove(kbTexte.length() - 1);
    kbDrawTextBox();
    return;
  }

  if (label == "ESPACE") {
    kbTexte += " ";
    kbDrawTextBox();
    return;
  }

  if (label == "ENTER") {
    Serial.println("==============");
    Serial.println(kbTexte);
    Serial.println("==============");
    kbTexte = "";
    kbDrawTextBox();
    return;
  }

  kbTexte += label;
  kbDrawTextBox();
}

void launchKeyboard() {
  appMode = MODE_KEYBOARD;
  kbMajuscule = false;
  kbModeSpecial = false;
  kbTexte = "";

  stopSDStartTouch();
  kbBuildKeyboard();

  Serial.println("CLAVIER PRET");
}

void updateKeyboard() {
  int x, y;

  if (getTouch(x, y)) {
    for (int i = 0; i < kbButtonCount; i++) {
      if (kbInButton(kbButtons[i], x, y)) {
        if (kbButtons[i].label != "BACK") {
          kbDrawButton(kbButtons[i], true);
          delay(60);
          kbDrawButton(kbButtons[i], false);
        }

        kbProcessButton(kbButtons[i].label);

        while (ts.touched()) delay(5);
        delay(20);
        break;
      }
    }
  }
}