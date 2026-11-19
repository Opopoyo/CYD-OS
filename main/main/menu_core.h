#ifndef MENU_CORE_H
#define MENU_CORE_H

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <TJpg_Decoder.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>

#define SCREEN_W 320
#define SCREEN_H 240

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

#define SD_CS   5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

#define TS_MIN_X 280
#define TS_MAX_X 3850
#define TS_MIN_Y 280
#define TS_MAX_Y 3780

#define ICON_LIVRE    "/systeme/iconeLivre-43px.jpg"
#define ICON_JEU      "/systeme/iconeJeu-43px.jpg"
#define ICON_IMAGE    "/systeme/iconeImage-43px.jpg"
#define ICON_MUSIQUE  "/systeme/iconeMusique-43px.jpg"
#define ICON_TEXTE    "/systeme/iconeTexte-43px.jpg"
#define ICON_OUTIL    "/systeme/iconeOutil-43px.jpg"

#define ICON_GALERIE       "/systeme/iconeGalerie-43px.jpg"
#define ICON_ECRITURE      "/systeme/iconeEcriture-43px.jpg"
#define ICON_SD            "/systeme/iconeSD-43px.jpg"
#define ICON_PIXEL_ART     "/systeme/iconePixel-art-43px.jpg"
#define ICON_INSTRUMENT    "/systeme/iconeInstrument-43px.jpg"
#define ICON_IDEE          "/systeme/iconeIdée-43px.jpg"
#define ICON_DAME          "/systeme/iconeDame-43px.jpg"
#define ICON_CASSE_BRIQUE  "/systeme/iconeCasseBrique-43px.jpg"
#define ICON_PONG          "/systeme/iconePong43px.jpg"
#define ICON_CHAT          "/systeme/iconeChat-43px.jpg"
#define ICON_WIFI          "/systeme/iconeWifi-43px.jpg"
#define ICON_HORLOGE       "/systeme/iconeHorloge-43px.jpg"
#define ICON_PEINTURE      "/systeme/iconePeinture-43px.jpg"
#define ICON_RMAGIC        "/systeme/iconeRmagique-43px.jpg"
#define ICON_SNAKE         "/systeme/iconeSnake-43px.jpg"
#define ICON_DISC          "/systeme/iconeWalkman-43px.jpg"
#define ICON_CARTE_SD      "/systeme/iconeSD-43px.jpg"
#define ICON_TETRIS        "/systeme/iconeTetris-43px.jpg"
#define ICON_COMPO         "/systeme/iconeCompo-43px.jpg"
#define ICON_METEO         "/systeme/iconeMeteo-43px.jpg"
#define ICON_RADIO         "/systeme/iconeRadio-43px.jpg"
#define ICON_VIDEO         "/systeme/iconeVideo-43px.jpg"
#define ICON_SAMPLER       "/systeme/iconeSampler-43px.jpg"
#define ICON_ENCYCLOPEDIE  "/systeme/iconeDicsionaire-43px.jpg"
#define ICON_METAL_HURLANT "/systeme/iconeLivre-43px.jpg"

enum AppMode {
  MODE_MENU,
  MODE_KEYBOARD,
  MODE_WRITING,
  MODE_ARCHIVE,
  MODE_GALERIE,
  MODE_PAINT,
  MODE_RMAGIC,
  MODE_JEU_CHAT,
  MODE_JEU_PONG,
  MODE_WIFI,
  MODE_HORLOGE,
  MODE_INSTRUMENT,
  MODE_DISC,
  MODE_IDEE,
  MODE_DAME,
  MODE_PIXEL_ART,
  MODE_CASSE_BRIQUE,
  MODE_SNAKE,
  MODE_CARTE_SD,
  MODE_TETRIS,
  MODE_SAMPLER,
  MODE_COMPO,
  MODE_METEO,
  MODE_RADIO,
  MODE_VIDEO,
  MODE_ENCYCLOPEDIE,
  MODE_METAL_HURLANT,
  MODE_SEIGNEUR_ETOILES
};

struct Button {
  int x;
  int y;
  int w;
  int h;
  String label;
  int index;
};

extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;
extern SPIClass touchscreenSPI;
extern SPIClass sdSPI;
extern AppMode appMode;

extern int currentMenu;

bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

bool startSD();
void stopSDStartTouch();

void drawBoldText(
  String txt,
  int x,
  int y,
  uint16_t fg,
  uint16_t bg,
  uint8_t datum,
  uint8_t font
);

void drawJpgFromSD(const char* filename, int x, int y);

bool getTouch(int &x, int &y);

void drawMainMenu();
void drawSubMenu(int menuIndex);
void updateMenuTouch();

void drawWifiIndicator();

#endif