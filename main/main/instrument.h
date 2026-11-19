#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <Arduino.h>

struct InstButton
{
  int x;
  int y;
  int w;
  int h;

  String label;
  int index;
};

void launchInstrument();
void updateInstrument();

#endif