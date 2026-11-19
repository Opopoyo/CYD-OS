#ifndef WIFI_APP_H
#define WIFI_APP_H


#include <Arduino.h>

struct WifiBtn {
  int x;
  int y;
  int w;
  int h;
  String label;
  int index;
};

void launchWifiApp();
void updateWifiApp();
void wifiAutoConnect();

#endif