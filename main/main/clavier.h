#ifndef CLAVIER_H
#define CLAVIER_H

#include <Arduino.h>

struct KbButton {
  int x;
  int y;
  int w;
  int h;
  String label;
};

void launchKeyboard();
void updateKeyboard();

#endif