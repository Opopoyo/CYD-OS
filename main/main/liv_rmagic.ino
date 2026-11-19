#include "liv_rmagic.h"
#include "menu_core.h"
#include "text_engine.h"

#define RM_FONT "Papernotes.vlw"

#define RM_PAGE_CHARS 520
#define RM_TEXT_X 10
#define RM_TEXT_Y 42
#define RM_TEXT_W 300
#define RM_TEXT_MAX_Y 184

String rmTitle = "R magique";
String rmStory = "";
int rmPage = 0;
unsigned long rmLastTouch = 0;

enum RMagicScene {
  RM_SCENE_INTRO,
  RM_SCENE_THE,
  RM_SCENE_PORTE,
  RM_SCENE_CHAT,
  RM_SCENE_FINAL
};

RMagicScene rmScene = RM_SCENE_INTRO;

// =====================================================
// OUTILS
// =====================================================

void rmLoadFont() {
  if (!teLoadFont(RM_FONT)) {
    teSetFont(2);
  }
}

void rmExit() {
  teUnloadFont();
  drawSubMenu(currentMenu);
}

int rmPageCount() {
  return tePageCount(rmStory, RM_PAGE_CHARS);
}

String rmPageText() {
  return teGetPage(rmStory, rmPage, RM_PAGE_CHARS);
}

void rmSetStory(String title, String text) {
  rmTitle = title;
  rmStory = teClean(text);
  rmPage = 0;
}

// =====================================================
// HISTOIRE
// =====================================================

void rmBuildScene() {
  if (rmScene == RM_SCENE_INTRO) {
    rmSetStory(
      "Le hurlement du terrier",
      "Éléonore marchait sous une pluie violette, avec un parapluie qui criait : "
      "\"Réveille-toi, petite étincelle !\".\n\n"
      "Au pied d'un vieux chêne, un lapin noir surgit. Il portait une veste rouge, "
      "des lunettes rondes, et une guitare électrique minuscule.\n\n"
      "\"Je suis en retard pour le concert du Roi-Corbeau !\" hurla-t-il, avant de "
      "sauter dans un terrier brillant comme une étoile fêlée.\n\n"
      "Éléonore vit alors une inscription gravée dans la terre :\n"
      "À toi de choisir, ô rêveuse : suis la note ou bois le thé."
    );
  }

  else if (rmScene == RM_SCENE_THE) {
    rmSetStory(
      "Le thé du Chapelier de fer",
      "Éléonore entra dans une clairière où une table tournait sur elle-même. "
      "Autour, des tasses chantaient en chœur : é, è, ê, ë, à, ç, œ !\n\n"
      "Le Chapelier de fer frappa sa théière avec une cuillère en argent.\n"
      "\"Ici, les idées bouillent, les horloges mordent, et les rêves font du bruit !\"\n\n"
      "Il versa un thé noir comme la nuit. Une vapeur forma des mots dans l'air :\n"
      "CHOISIS TA PORTE, SINON LA PORTE TE CHOISIRA.\n\n"
      "Au loin, une porte bleue vibrait comme un ampli de métal."
    );
  }

  else if (rmScene == RM_SCENE_PORTE) {
    rmSetStory(
      "La porte qui chantait faux",
      "La porte bleue n'avait pas de poignée. Elle avait une bouche.\n\n"
      "\"Mot de passe ?\" demanda-t-elle.\n\n"
      "Éléonore répondit : \"Fromage électrique.\"\n"
      "La porte ouvrit un œil, très vexée.\n\n"
      "\"Correct. Mais seulement parce que j'adore le fromage.\"\n\n"
      "Derrière la porte, un escalier descendait vers une scène géante. Des champignons "
      "lumineux formaient un public surexcité. Un chat rayé dirigeait l'orchestre avec "
      "sa queue, et chaque coup produisait un éclair rose."
    );
  }

  else if (rmScene == RM_SCENE_CHAT) {
    rmSetStory(
      "Le chat distordu",
      "Le chat sourit si largement que son sourire resta suspendu quand son corps disparut.\n\n"
      "\"Bienvenue, Éléonore. Tu es tombée dans le Livre qui mord.\"\n\n"
      "Une basse énorme gronda : BROMM. Les pages se mirent à tourner seules. "
      "Chaque paragraphe devenait un chemin, chaque accent une étincelle, chaque apostrophe "
      "une petite griffe noire.\n\n"
      "\"Tu veux sortir ? Alors joue la note impossible : celle qui fait rire les monstres "
      "et pleurer les horloges.\""
    );
  }

  else {
    rmSetStory(
      "Final : la note impossible",
      "Éléonore prit la guitare du lapin noir. Elle ne savait pas jouer. "
      "Alors elle pensa à toutes les choses impossibles : les théières jalouses, "
      "les portes bavardes, les chats sans corps, les étoiles qui tombent en faisant BOUM.\n\n"
      "Puis elle gratta une seule corde.\n\n"
      "La note impossible jaillit : ÀÉÈÇÙŒ !\n\n"
      "Le monde entier trembla, puis éclata de rire. Le Roi-Corbeau applaudit avec ses ailes.\n"
      "\"Parfait !\" cria-t-il. \"Le chaos est juste une chanson qui n'a pas encore trouvé son refrain.\"\n\n"
      "Éléonore se réveilla sous son vieux chêne. Dans sa main, il y avait une minuscule "
      "clé noire, chaude comme un souvenir."
    );
  }
}

// =====================================================
// DESSIN
// =====================================================

void rmButton(int x, int y, int w, int h, String label, uint16_t bg, uint16_t fg = TFT_WHITE) {
  tft.fillRoundRect(x, y, w, h, 7, bg);
  tft.drawRoundRect(x, y, w, h, 7, TFT_DARKGREY);
  teDrawString(label, x + w / 2, y + h / 2, fg, bg, MC_DATUM, 1);
}

void rmDrawHeader() {
  tft.fillRect(0, 0, 320, 34, TFT_BLACK);
  tft.fillTriangle(10, 17, 30, 6, 30, 28, TFT_WHITE);

  teDrawString(rmTitle, 165, 17, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);
}

void rmDrawBackground() {
  uint16_t bg1 = tft.color565(8, 4, 16);
  uint16_t bg2 = tft.color565(25, 8, 44);

  for (int y = 0; y < 240; y += 4) {
    uint8_t r = 8 + y / 18;
    uint8_t g = 4 + y / 40;
    uint8_t b = 16 + y / 7;
    tft.fillRect(0, y, 320, 4, tft.color565(r, g, b));
  }

  for (int i = 0; i < 28; i++) {
    int x = random(6, 314);
    int y = random(38, 206);
    uint16_t c = (i % 3 == 0) ? TFT_MAGENTA : TFT_CYAN;
    tft.drawPixel(x, y, c);
    if (i % 5 == 0) tft.drawPixel(x + 1, y, TFT_YELLOW);
  }
}

void rmDrawTextPanel() {
  uint16_t panel = tft.color565(12, 10, 25);
  uint16_t frame = tft.color565(120, 50, 180);
  uint16_t txt = tft.color565(245, 235, 215);

  tft.fillRoundRect(6, 38, 308, 172, 9, panel);
  tft.drawRoundRect(6, 38, 308, 172, 9, frame);

  teDrawWrapped(
    rmPageText(),
    RM_TEXT_X,
    RM_TEXT_Y,
    RM_TEXT_W,
    RM_TEXT_MAX_Y,
    18,
    txt,
    panel,
    2
  );
}

void rmDrawFooter() {
  tft.fillRect(0, 214, 320, 26, TFT_BLACK);

  rmButton(8, 217, 52, 20, "<", TFT_DARKGREY);
  rmButton(66, 217, 52, 20, ">", TFT_DARKGREY);
  rmButton(124, 217, 72, 20, "Choix", TFT_BLUE);
  rmButton(204, 217, 106, 20, String(rmPage + 1) + "/" + String(rmPageCount()), TFT_BLACK);
}

void rmDrawChoiceScreen() {
  rmDrawBackground();
  rmDrawHeader();

  uint16_t panel = tft.color565(12, 10, 25);

  tft.fillRoundRect(20, 48, 280, 142, 10, panel);
  tft.drawRoundRect(20, 48, 280, 142, 10, TFT_MAGENTA);

  teDrawString("Quel chemin prends-tu ?", 160, 70, TFT_YELLOW, panel, MC_DATUM, 1);

  rmButton(42, 96, 236, 28, "Boire le thé hurlant", TFT_PURPLE);
  rmButton(42, 134, 236, 28, "Ouvrir la porte bleue", TFT_BLUE);

  rmButton(92, 210, 136, 24, "Retour texte", TFT_DARKGREY);
}

void rmDrawPage() {
  rmDrawBackground();
  rmDrawHeader();
  rmDrawTextPanel();
  rmDrawFooter();
}

// =====================================================
// LANCEMENT
// =====================================================

void launchRMagic() {
  appMode = MODE_RMAGIC;

  stopSDStartTouch();

  rmLoadFont();

  randomSeed(millis());
  rmScene = RM_SCENE_INTRO;
  rmBuildScene();
  rmDrawPage();

  Serial.println("R MAGIC PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateRMagic() {
  int x, y;

  if (!getTouch(x, y)) return;
  if (millis() - rmLastTouch < 170) return;
  rmLastTouch = millis();

  // Retour menu
  if (y < 34 && x < 45) {
    teUnloadFont();
    drawSubMenu(currentMenu);

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // Page précédente
  if (x >= 8 && x <= 60 && y >= 217 && y <= 237) {
    rmPage--;
    if (rmPage < 0) rmPage = 0;
    rmDrawPage();
  }

  // Page suivante
  else if (x >= 66 && x <= 118 && y >= 217 && y <= 237) {
    rmPage++;
    if (rmPage >= rmPageCount()) rmPage = rmPageCount() - 1;
    rmDrawPage();
  }

  // Choix
  else if (x >= 124 && x <= 196 && y >= 217 && y <= 237) {
    rmDrawChoiceScreen();
  }

  // Choix 1
  else if (x >= 42 && x <= 278 && y >= 96 && y <= 124) {
    if (rmScene == RM_SCENE_INTRO) rmScene = RM_SCENE_THE;
    else if (rmScene == RM_SCENE_THE) rmScene = RM_SCENE_CHAT;
    else if (rmScene == RM_SCENE_PORTE) rmScene = RM_SCENE_CHAT;
    else rmScene = RM_SCENE_FINAL;

    rmBuildScene();
    rmDrawPage();
  }

  // Choix 2
  else if (x >= 42 && x <= 278 && y >= 134 && y <= 162) {
    if (rmScene == RM_SCENE_INTRO) rmScene = RM_SCENE_PORTE;
    else if (rmScene == RM_SCENE_THE) rmScene = RM_SCENE_PORTE;
    else if (rmScene == RM_SCENE_CHAT) rmScene = RM_SCENE_FINAL;
    else rmScene = RM_SCENE_FINAL;

    rmBuildScene();
    rmDrawPage();
  }

  // Retour texte depuis écran choix
  else if (x >= 92 && x <= 228 && y >= 210 && y <= 234) {
    rmDrawPage();
  }

  while (ts.touched()) delay(5);
  delay(80);
}