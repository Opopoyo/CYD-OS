#include "menu_registry.h"
#include "menu_core.h"

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

// =====================================================
// RUBRIQUES PRINCIPALES
// =====================================================

const char* menuNames[] = {
  "Livre",
  "Jeu",
  "Image",
  "Musique",
  "Texte",
  "Outil"
};

const char* menuIcons[] = {
  ICON_LIVRE,
  ICON_JEU,
  ICON_IMAGE,
  ICON_MUSIQUE,
  ICON_TEXTE,
  ICON_OUTIL
};

const int menuCount = 6;

// =====================================================
// CATEGORIES
// =====================================================

#define CAT_LIVRE   0
#define CAT_JEU     1
#define CAT_IMAGE   2
#define CAT_MUSIQUE 3
#define CAT_TEXTE   4
#define CAT_OUTIL   5

// =====================================================
// REGISTRE UNIQUE DES APPLICATIONS
// =====================================================
//
// Pour ajouter une application :
// 1 ligne ici seulement.
//
// Format :
// {"Nom affiché", ICON_XXX, CATEGORIE, launchFonction}
//

const AppEntry appRegistry[] = {
  // LIVRE
  {"R magique",          ICON_RMAGIC,        CAT_LIVRE,   launchRMagic},
  {"Encyclopedie",       ICON_ENCYCLOPEDIE,  CAT_LIVRE,   launchEncyclopedie},
  {"Metal Urgen",        ICON_LIVRE,         CAT_LIVRE,   launchMetalHurlant},
  {"Seigneur Etoiles",   ICON_LIVRE,         CAT_LIVRE,   launchSeigneurEtoiles},
  {"Livre2",             ICON_LIVRE,         CAT_LIVRE,   nullptr},
  {"Livre3",             ICON_LIVRE,         CAT_LIVRE,   nullptr},

  // JEU
  {"Chat",               ICON_CHAT,          CAT_JEU,     launchJeuChat},
  {"Pong",               ICON_PONG,          CAT_JEU,     launchJeuPong},
  {"Dame",               ICON_DAME,          CAT_JEU,     launchDame},
  {"Casse Brique",       ICON_CASSE_BRIQUE,  CAT_JEU,     launchCasseBrique},
  {"Snake",              ICON_SNAKE,         CAT_JEU,     launchSnake},
  {"Tetris",             ICON_TETRIS,        CAT_JEU,     launchTetris},

  // IMAGE
  {"Galerie",            ICON_GALERIE,       CAT_IMAGE,   launchGalerie},
  {"Peinture",           ICON_PEINTURE,      CAT_IMAGE,   launchPeinture},
  {"Pixel-Art",          ICON_PIXEL_ART,     CAT_IMAGE,   launchPixelArt},

  // MUSIQUE
  {"Instrument",         ICON_INSTRUMENT,    CAT_MUSIQUE, launchInstrument},
  {"Disc",               ICON_DISC,          CAT_MUSIQUE, launchDisc},
  {"Radio",              ICON_RADIO,         CAT_MUSIQUE, launchRadio},
  {"Sampler",            ICON_SAMPLER,       CAT_MUSIQUE, launchSampler},
  {"Compo",              ICON_COMPO,         CAT_MUSIQUE, launchCompo},
  {"Zic3",               ICON_MUSIQUE,       CAT_MUSIQUE, nullptr},

  // TEXTE
  {"Clavier",            ICON_TEXTE,         CAT_TEXTE,   launchKeyboard},
  {"Ecriture",           ICON_ECRITURE,      CAT_TEXTE,   launchWriting},
  {"Archive",            ICON_SD,            CAT_TEXTE,   launchArchive},

  // OUTIL
  {"Wifi",               ICON_WIFI,          CAT_OUTIL,   launchWifiApp},
  {"Horloge",            ICON_HORLOGE,       CAT_OUTIL,   launchHorlogeApp},
  {"Idee",               ICON_IDEE,          CAT_OUTIL,   launchIdee},
  {"Carte SD",           ICON_CARTE_SD,      CAT_OUTIL,   launchCarteSD},
  {"Meteo",              ICON_METEO,         CAT_OUTIL,   launchMeteo},
  {"Video",              ICON_VIDEO,         CAT_OUTIL,   launchVideo}
};

const int appRegistryCount = sizeof(appRegistry) / sizeof(appRegistry[0]);

// =====================================================
// OUTILS REGISTRE
// =====================================================

int getAppCount(int menuIndex) {
  int count = 0;

  for (int i = 0; i < appRegistryCount; i++) {
    if (appRegistry[i].category == menuIndex) {
      count++;
    }
  }

  return count;
}

const AppEntry* getAppEntry(int menuIndex, int appIndex) {
  int visibleIndex = 0;

  for (int i = 0; i < appRegistryCount; i++) {
    if (appRegistry[i].category != menuIndex) continue;

    if (visibleIndex == appIndex) {
      return &appRegistry[i];
    }

    visibleIndex++;
  }

  return nullptr;
}

String getAppName(int menuIndex, int appIndex) {
  const AppEntry* app = getAppEntry(menuIndex, appIndex);
  if (!app) return "";
  return String(app->label);
}

const char* getAppIcon(int menuIndex, int appIndex) {
  const AppEntry* app = getAppEntry(menuIndex, appIndex);
  if (!app) return ICON_SD;
  return app->icon;
}

bool launchAppByLabel(String label) {
  for (int i = 0; i < appRegistryCount; i++) {
    if (label == String(appRegistry[i].label)) {
      if (appRegistry[i].launch != nullptr) {
        appRegistry[i].launch();
        return true;
      }
      return false;
    }
  }

  return false;
}