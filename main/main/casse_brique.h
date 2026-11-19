#ifndef CASSE_BRIQUE_H
#define CASSE_BRIQUE_H

#include <Arduino.h>

#define CB_ROWS 6
#define CB_COLS 10

#define CB_MAX_BALLS 4
#define CB_MAX_BONUS 8
#define CB_MAX_SHOTS 6

struct CBBall {
  float x;
  float y;
  float vx;
  float vy;
  int r;
  uint16_t color;
  bool active;
};

struct CBBrick {
  bool alive;
  bool alien;
  int hp;
};

struct CBBonus {
  float x;
  float y;
  int type;
  bool active;
};

struct CBShot {
  float x;
  float y;
  bool active;
};

void launchCasseBrique();
void updateCasseBrique();

#endif