#ifndef GALERIE_H
#define GALERIE_H

#include <Arduino.h>

struct GalButton {
  int x;
  int y;
  int w;
  int h;
  String label;
  int index;
};

void launchGalerie();
void updateGalerie();

#endif