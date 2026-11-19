#ifndef MENU_REGISTRY_H
#define MENU_REGISTRY_H

#include <Arduino.h>

extern const char* menuNames[];
extern const char* menuIcons[];
extern const int menuCount;

int getAppCount(int menuIndex);
String getAppName(int menuIndex, int appIndex);
const char* getAppIcon(int menuIndex, int appIndex);
const char* getAppIconByLabel(String label);
bool launchAppByLabel(String label);

#endif