#ifndef ENCYCLOPEDIE_H
#define ENCYCLOPEDIE_H

#include <Arduino.h>

struct EncKey {
  int x;
  int y;
  int w;
  int h;
  String label;
};

void launchEncyclopedie();
void updateEncyclopedie();

#endif