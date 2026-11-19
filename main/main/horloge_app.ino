#include "horloge_app.h"
#include "menu_core.h"
#include "wifi_app.h"

#include <WiFi.h>
#include <time.h>

unsigned long lastClockUpdate = 0;

void drawClockScreen() {

  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, 320, 36, TFT_BLACK);

  // bouton retour
  tft.fillTriangle(
    11, 18,
    31, 7,
    31, 29,
    TFT_WHITE
  );

  drawBoldText(
    "Horloge",
    160,
    18,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );
}

void drawClockTime() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {

    tft.fillRect(
      0,
      40,
      320,
      200,
      TFT_BLACK
    );

    drawBoldText(
      "Heure indisponible",
      160,
      120,
      TFT_RED,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    return;
  }

  char timeStr[16];

  strftime(
    timeStr,
    sizeof(timeStr),
    "%H:%M:%S",
    &timeinfo
  );

  // nettoyage zone heure
  tft.fillRect(
    0,
    60,
    320,
    90,
    TFT_BLACK
  );

  drawBoldText(
    String(timeStr),
    160,
    110,
    TFT_BLUE,
    TFT_BLACK,
    MC_DATUM,
    7
  );

  char dateStr[20];

  strftime(
    dateStr,
    sizeof(dateStr),
    "%d/%m/%Y",
    &timeinfo
  );

  tft.fillRect(
    0,
    160,
    320,
    40,
    TFT_BLACK
  );

  drawBoldText(
    String(dateStr),
    160,
    180,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );
}

void launchHorlogeApp() {
  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);

    drawBoldText(
      "Wifi requis",
      160,
      95,
      TFT_RED,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    drawBoldText(
      "Ouverture Wifi...",
      160,
      130,
      TFT_WHITE,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    delay(900);

    launchWifiApp();
    return;
  }

  appMode = MODE_HORLOGE;

  configTzTime(
    "CET-1CEST,M3.5.0/2,M10.5.0/3",
    "pool.ntp.org",
    "time.nist.gov"
  );

  drawClockScreen();
  drawClockTime();

  lastClockUpdate = millis();

  Serial.println("HORLOGE PRETE");
}

void updateHorlogeApp() {
  int x, y;

  if (getTouch(x, y)) {
    if (y < 36 && x < 45) {
      drawSubMenu(currentMenu);

      while (ts.touched()) delay(5);
      delay(80);
      return;
    }
  }

  if (millis() - lastClockUpdate > 1000) {
    lastClockUpdate = millis();
    drawClockTime();
  }
}