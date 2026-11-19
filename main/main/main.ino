#include "menu_core.h"

#include "audio_system.h"
#include "audio_wav_player.h"
#include "loader.h"
#include "text_engine.h"

#include "clavier.h"
#include "ecriture.h"
#include "archive.h"
#include "galerie.h"
#include "peinture.h"
#include "liv_rmagic.h"
#include "jeu_chat.h"
#include "jeu_pong.h"
#include "wifi_app.h"
#include "horloge_app.h"
#include "instrument.h"
#include "disc.h"
#include "idee.h"
#include "dame.h"
#include "pixel_art.h"
#include "casse_brique.h"
#include "snake.h"
#include "carte_sd.h"
#include "tetris.h"
#include "sampler.h"
#include "compo.h"
#include "meteo.h"
#include "radio.h"
#include "video.h"
#include "encyclopedie.h"
#include "liv_metal_hurlant.h"
#include "liv_seigneur_etoiles.h"

TFT_eSPI tft = TFT_eSPI();

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

SPIClass sdSPI = SPIClass(VSPI);

AppMode appMode = MODE_MENU;

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(XPT2046_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(XPT2046_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  audioSystemBegin();

  tft.init();
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Important : teBegin après tft.init()
  teBegin();

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tftOutput);

  showLoaderStart();

  loaderProgress(20);
  Serial.println("Audio OK");

  loaderProgress(35);
  Serial.println("TFT OK");

  loaderProgress(45);
  Serial.println("WiFi connexion...");
  wifiAutoConnect();

  loaderProgress(75);
  Serial.println("SD / Touch init...");
  stopSDStartTouch();

  loaderProgress(100);
  delay(250);

  drawMainMenu();

  Serial.println("MENU CYD PRET");
}

void loop() {
  if (appMode == MODE_KEYBOARD) updateKeyboard();
  else if (appMode == MODE_WRITING) updateWriting();
  else if (appMode == MODE_ARCHIVE) updateArchive();
  else if (appMode == MODE_GALERIE) updateGalerie();
  else if (appMode == MODE_PAINT) updatePeinture();
  else if (appMode == MODE_RMAGIC) updateRMagic();
  else if (appMode == MODE_JEU_CHAT) updateJeuChat();
  else if (appMode == MODE_JEU_PONG) updateJeuPong();
  else if (appMode == MODE_WIFI) updateWifiApp();
  else if (appMode == MODE_HORLOGE) updateHorlogeApp();
  else if (appMode == MODE_INSTRUMENT) updateInstrument();
  else if (appMode == MODE_DISC) updateDisc();
  else if (appMode == MODE_IDEE) updateIdee();
  else if (appMode == MODE_DAME) updateDame();
  else if (appMode == MODE_PIXEL_ART) updatePixelArt();
  else if (appMode == MODE_CASSE_BRIQUE) updateCasseBrique();
  else if (appMode == MODE_SNAKE) updateSnake();
  else if (appMode == MODE_CARTE_SD) updateCarteSD();
  else if (appMode == MODE_TETRIS) updateTetris();
  else if (appMode == MODE_SAMPLER) updateSampler();
  else if (appMode == MODE_COMPO) updateCompo();
  else if (appMode == MODE_METEO) updateMeteo();
  else if (appMode == MODE_RADIO) updateRadio();
  else if (appMode == MODE_VIDEO) updateVideo();
  else if (appMode == MODE_ENCYCLOPEDIE) updateEncyclopedie();
  else if (appMode == MODE_METAL_HURLANT) updateMetalHurlant();
  else if (appMode == MODE_SEIGNEUR_ETOILES) updateSeigneurEtoiles();
  else updateMenuTouch();
}