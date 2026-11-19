#include "liv_seigneur_etoiles.h"
#include "menu_core.h"
#include "text_engine.h"

#define SE_FONT_COUNT 12
#define SE_MAX_VARIANTS 8
#define SE_STYLE_COUNT 5

const char* seFamilyNames[SE_FONT_COUNT] = {
  "Ananda",
  "Dune",
  "FreeSans",
  "Gotham",
  "GothamB",
  "Modum",
  "Papernotes",
  "Pixelmania",
  "Plank",
  "Roman",
  "alex",
  "Base"
};

const char* seFonts[SE_FONT_COUNT][SE_MAX_VARIANTS] = {
  {"Ananda22.vlw", "Ananda26.vlw", "", "", "", "", "", ""},
  {"Dune8.vlw", "Dune10.vlw", "Dune12.vlw", "Dune14.vlw", "Dune16.vlw", "Dune18.vlw", "", ""},
  {"FreeSans.vlw", "", "", "", "", "", "", ""},
  {"Gotham12.vlw", "Gotham14.vlw", "Gotham16.vlw", "Gotham18.vlw", "Gotham20.vlw", "Gotham22.vlw", "Gotham24.vlw", "Gotham26.vlw"},
  {"GothamB12.vlw", "GothamB14.vlw", "GothamB16.vlw", "GothamB18.vlw", "GothamB20.vlw", "GothamB22.vlw", "GothamB24.vlw", "GothamB26.vlw"},
  {"Modum12.vlw", "Modum14.vlw", "", "", "", "", "", ""},
  {"Papernotes.vlw", "Papernotes12.vlw", "Papernotes14.vlw", "Papernotes16.vlw", "Papernotes18.vlw", "Papernotes20.vlw", "", ""},
  {"Pixelmania10.vlw", "Pixelmania12.vlw", "Pixelmania14.vlw", "", "", "", "", ""},
  {"Plank14.vlw", "Plank16.vlw", "Plank18.vlw", "Plank20.vlw", "", "", "", ""},
  {"Roman14.vlw", "Roman18.vlw", "Roman22.vlw", "Roman24.vlw", "", "", "", ""},
  {"alex.vlw", "", "", "", "", "", "", ""},
  {"Papernotes.vlw", "FreeSans.vlw", "Gotham20.vlw", "Roman22.vlw", "", "", "", ""}
};

const char* seStyleNames[SE_STYLE_COUNT] = {
  "Livre",
  "Article clair",
  "Article nuit",
  "Titre choix",
  "Accents"
};

int seFamily = 0;
int seVariant = 0;
int seStyle = 0;

unsigned long seLastTouch = 0;

uint16_t seBg, sePanel, seText, seBar, seAccent;

String seSample =
"Le Seigneur des Étoiles\n\n"
"Théodore observa l'étrange lumière au-dessus des collines. "
"Les étoiles bougeaient lentement, puis rapides comme des oiseaux de feu.\n\n"
"À l'horizon, un vaisseau descendait du ciel. "
"Sa coque argentée reflétait les nuages et la peur des hommes.\n\n"
"Accents : é è ê ë à â ç ù û î ï ô œ É È À Ç Œ.";

String seCurrentFont() {
  return String(seFonts[seFamily][seVariant]);
}

int seVariantCount(int fam) {
  int n = 0;
  for (int i = 0; i < SE_MAX_VARIANTS; i++) {
    if (String(seFonts[fam][i]).length() > 0) n++;
  }
  if (n < 1) n = 1;
  return n;
}

void seFixVariant() {
  int n = seVariantCount(seFamily);
  if (seVariant >= n) seVariant = 0;
}

void seApplyColors() {
  if (seStyle == 2) {
    seBg = tft.color565(8, 10, 18);
    sePanel = tft.color565(18, 22, 34);
    seText = tft.color565(235, 230, 210);
    seBar = TFT_BLACK;
    seAccent = TFT_CYAN;
  } else {
    seBg = tft.color565(225, 216, 190);
    sePanel = tft.color565(255, 248, 225);
    seText = tft.color565(25, 20, 15);
    seBar = tft.color565(64, 42, 26);
    seAccent = tft.color565(120, 55, 35);
  }
}

void seBackToMenu() {
  teUnloadFont();
  drawSubMenu(currentMenu);
}

bool seLoadFont() {
  String f = seCurrentFont();
  if (f.length() == 0) return false;
  return teLoadFont(f);
}

void seButton(int x, int y, int w, int h, String label) {
  tft.fillRoundRect(x, y, w, h, 5, seBar);
  tft.drawRoundRect(x, y, w, h, 5, TFT_WHITE);
  teDrawString(label, x + w / 2, y + h / 2, TFT_WHITE, seBar, MC_DATUM, 2);
}

void seDrawHeader() {
  tft.fillRect(0, 0, 320, 32, seBar);
  tft.fillTriangle(10, 16, 31, 6, 31, 27, TFT_WHITE);
  teDrawString("Test Polices SD", 168, 16, TFT_WHITE, seBar, MC_DATUM, 2);
}

void seDrawFooter() {
  tft.fillRect(0, 214, 320, 26, seBar);

  seButton(4, 217, 60, 20, "Famille");
  seButton(68, 217, 58, 20, "Taille");
  seButton(130, 217, 56, 20, "Style");
  seButton(248, 217, 66, 20, "Menu");
}

void seDrawBackground() {
  tft.fillScreen(seBg);

  if (seStyle != 2) {
    for (int i = 0; i < 90; i++) {
      int x = random(0, 320);
      int y = random(34, 212);
      uint16_t c = tft.color565(
        205 + random(0, 30),
        195 + random(0, 25),
        165 + random(0, 25)
      );
      tft.drawPixel(x, y, c);
    }
  }
}

int seLineH() {
  String f = seCurrentFont();

  if (f.indexOf("8") >= 0) return 14;
  if (f.indexOf("10") >= 0) return 16;
  if (f.indexOf("12") >= 0) return 18;
  if (f.indexOf("14") >= 0) return 20;
  if (f.indexOf("16") >= 0) return 22;
  if (f.indexOf("18") >= 0) return 24;
  if (f.indexOf("20") >= 0) return 26;
  if (f.indexOf("22") >= 0) return 28;
  if (f.indexOf("24") >= 0) return 30;
  if (f.indexOf("26") >= 0) return 32;

  return 24;
}

int seStartY() {
  int lh = seLineH();
  if (lh >= 30) return 83;
  if (lh >= 26) return 87;
  return 91;
}

void seDrawInfo() {
  teDrawString(
    "Famille : " + String(seFamilyNames[seFamily]),
    16,
    42,
    seAccent,
    sePanel,
    TL_DATUM,
    2
  );

  teDrawString(
    "Fichier : " + seCurrentFont(),
    16,
    61,
    seAccent,
    sePanel,
    TL_DATUM,
    2
  );

  teDrawString(
    "Style : " + String(seStyleNames[seStyle]),
    16,
    80,
    seAccent,
    sePanel,
    TL_DATUM,
    2
  );
}

void seDrawPage() {
  seFixVariant();
  seApplyColors();

  seDrawBackground();
  seDrawHeader();

  tft.fillRoundRect(6, 38, 308, 172, 10, sePanel);
  tft.drawRoundRect(6, 38, 308, 172, 10, seAccent);

  bool ok = seLoadFont();

  if (!ok) {
    teSetFont(2);
    teDrawString("Police introuvable", 16, 58, TFT_RED, sePanel, TL_DATUM, 2);
    teDrawString(seCurrentFont(), 16, 90, seText, sePanel, TL_DATUM, 2);
  } else {
    seDrawInfo();

    if (seStyle == 0) {
      teDrawWrapped(seSample, 16, seStartY(), 288, 198, seLineH(), seText, sePanel, 2);
    }

    else if (seStyle == 1) {
      teDrawWrapped(
        "Article clair : idéal pour Encyclopédie si la police reste nette. "
        "Exemple avec accents : l'été, l'étoile, château, cœur, œuvre.",
        16,
        seStartY(),
        288,
        198,
        seLineH(),
        seText,
        sePanel,
        2
      );
    }

    else if (seStyle == 2) {
      teDrawWrapped(
        "Article nuit : lecture longue. "
        "Le texte doit rester doux, lisible, sans fatigue visuelle.",
        16,
        seStartY(),
        288,
        198,
        seLineH(),
        seText,
        sePanel,
        2
      );
    }

    else if (seStyle == 3) {
      teDrawString("TITRE DE CHAPITRE", 160, 70, seAccent, sePanel, MC_DATUM, 2);
      teDrawString("Choix : suivre Théodore", 24, 112, seText, sePanel, TL_DATUM, 2);
      teDrawString("Choix : observer les étoiles", 24, 150, seText, sePanel, TL_DATUM, 2);
      teDrawString("Choix : entrer dans le vaisseau", 24, 186, seText, sePanel, TL_DATUM, 2);
    }

    else if (seStyle == 4) {
      teDrawString("ACCENTS", 160, 60, seAccent, sePanel, MC_DATUM, 2);
      teDrawString("é è ê ë à â ç", 18, 94, seText, sePanel, TL_DATUM, 2);
      teDrawString("ù û î ï ô œ", 18, 126, seText, sePanel, TL_DATUM, 2);
      teDrawString("É È À Ç Œ", 18, 158, seText, sePanel, TL_DATUM, 2);
      teDrawString("{ } [ ] < > # & /", 18, 190, seText, sePanel, TL_DATUM, 2);
    }
  }

  seDrawFooter();
  teFinishDraw();
}

void launchSeigneurEtoiles() {
  appMode = MODE_SEIGNEUR_ETOILES;

  stopSDStartTouch();
  randomSeed(millis());

  seFamily = 0;
  seVariant = 0;
  seStyle = 0;

  seDrawPage();

  Serial.println("TEST TOUTES POLICES SD PRET");
}

void updateSeigneurEtoiles() {
  int x, y;

  if (!getTouch(x, y)) return;
  if (millis() - seLastTouch < 170) return;

  seLastTouch = millis();

  if (y < 34 && x < 45) {
    seBackToMenu();
    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (x >= 4 && x <= 64 && y >= 217 && y <= 237) {
    seFamily++;
    if (seFamily >= SE_FONT_COUNT) seFamily = 0;
    seVariant = 0;
    seDrawPage();
  }

  else if (x >= 68 && x <= 126 && y >= 217 && y <= 237) {
    seVariant++;
    if (seVariant >= seVariantCount(seFamily)) seVariant = 0;
    seDrawPage();
  }

  else if (x >= 130 && x <= 186 && y >= 217 && y <= 237) {
    seStyle++;
    if (seStyle >= SE_STYLE_COUNT) seStyle = 0;
    seDrawPage();
  }

  else if (x >= 248 && x <= 314 && y >= 217 && y <= 237) {
    seBackToMenu();
  }

  while (ts.touched()) delay(5);
  delay(80);
}