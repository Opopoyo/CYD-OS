#include "idee.h"
#include "menu_core.h"

String ideeProjects[IDEE_MAX_PROJECTS];
int ideeProjectCount = 0;
int ideeCurrentProject = -1;

IdeeNode idees[IDEE_MAX_NODES];
int ideeCount = 0;

IdeeState ideeState = IDEE_PROJECTS;

IdeeButton ideeButtons[80];
int ideeButtonCount = 0;

int ideeSelected = -1;
int ideeNewParent = -1;
int ideeEditTarget = -1;

String ideeInput = "";
int ideeInputMode = 0;

unsigned long ideeLastTouch = 0;

int ideeOffsetX = 0;
int ideeOffsetY = 0;

int ideeZoom = 100;
int ideeZoomMin = 45;
int ideeZoomMax = 100;

bool ideeDragging = false;
int ideeDragStartX = 0;
int ideeDragStartY = 0;
int ideeDragBaseX = 0;
int ideeDragBaseY = 0;

uint16_t ideeColors[] = {
  TFT_RED, TFT_BLUE, TFT_GREEN, TFT_ORANGE,
  TFT_PURPLE, TFT_CYAN, TFT_YELLOW, TFT_PINK
};

// =====================================================
// OUTILS
// =====================================================

void ideeAddButton(int x, int y, int w, int h, String label, int index) {
  if (ideeButtonCount >= 80) return;

  ideeButtons[ideeButtonCount].x = x;
  ideeButtons[ideeButtonCount].y = y;
  ideeButtons[ideeButtonCount].w = w;
  ideeButtons[ideeButtonCount].h = h;
  ideeButtons[ideeButtonCount].label = label;
  ideeButtons[ideeButtonCount].index = index;

  ideeButtonCount++;
}

bool ideeInButton(IdeeButton b, int x, int y) {
  return x >= b.x && x <= b.x + b.w &&
         y >= b.y && y <= b.y + b.h;
}

String ideeClean(String s) {
  s.replace("|", "/");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

String ideeShort(String s, int n) {
  if (s.length() > n) return s.substring(0, n);
  return s;
}

void ideeDrawBackWhite() {
  tft.fillRect(0, 0, 42, 36, TFT_WHITE);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_BLACK);
}

void ideeHeaderBlack(String title) {
  tft.fillRect(0, 0, 320, 36, TFT_BLACK);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);
  drawBoldText(title, 160, 18, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

void ideeDrawMapTools() {
  tft.fillRect(170, 0, 150, 42, TFT_WHITE);

  tft.fillRoundRect(174, 5, 28, 28, 6, TFT_BLACK);
  drawBoldText("-", 188, 19, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
  ideeAddButton(170, 2, 36, 38, "Z-", -12);

  tft.fillRoundRect(210, 5, 28, 28, 6, TFT_BLACK);
  drawBoldText("+", 224, 19, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
  ideeAddButton(206, 2, 36, 38, "Z+", -13);

  tft.fillCircle(260, 20, 15, TFT_BLACK);
  tft.fillCircle(260, 20, 5, TFT_WHITE);
  ideeAddButton(242, 2, 36, 38, "CENT", -11);

  tft.fillCircle(300, 20, 15, TFT_BLACK);
  drawBoldText("+", 300, 19, TFT_WHITE, TFT_BLACK, MC_DATUM, 4);
  ideeAddButton(282, 2, 36, 38, "+", -10);
}

// =====================================================
// ARBORESCENCE ANTI-CHEVAUCHEMENT
// =====================================================

float ideeNormalizeAngle(float a) {
  while (a > 3.14159) a -= 6.28318;
  while (a < -3.14159) a += 6.28318;
  return a;
}

int ideeLevel(int id) {
  int level = 0;
  int p = idees[id].parent;

  while (p >= 0 && p < ideeCount && level < 9) {
    level++;
    p = idees[p].parent;
  }

  return level;
}

int ideeCountAtLevel(int level) {
  int c = 0;

  for (int i = 0; i < ideeCount; i++) {
    if (ideeLevel(i) == level) c++;
  }

  return c;
}

int ideeRankAtLevel(int id) {
  int level = ideeLevel(id);
  int r = 0;

  for (int i = 0; i < id; i++) {
    if (ideeLevel(i) == level) r++;
  }

  return r;
}

int ideeChildCount(int parent) {
  int c = 0;

  for (int i = 0; i < ideeCount; i++) {
    if (idees[i].parent == parent) c++;
  }

  return c;
}

int ideeChildRank(int id) {
  int r = 0;
  int p = idees[id].parent;

  for (int i = 0; i < id; i++) {
    if (idees[i].parent == p) r++;
  }

  return r;
}

int ideeFirstChild(int parent) {
  for (int i = 0; i < ideeCount; i++) {
    if (idees[i].parent == parent) return i;
  }

  return -1;
}

int ideeLastChild(int parent) {
  int last = -1;

  for (int i = 0; i < ideeCount; i++) {
    if (idees[i].parent == parent) last = i;
  }

  return last;
}

int ideeFamilyCountAtLevel(int level) {
  if (level == 0) return max(1, ideeCountAtLevel(0));

  int count = 0;

  for (int p = 0; p < ideeCount; p++) {
    if (ideeLevel(p) == level - 1 && ideeChildCount(p) > 0) {
      count++;
    }
  }

  return max(1, count);
}

float ideeFamilyMaxSpread(int parent, int level) {
  int childCount = max(1, ideeChildCount(parent));
  int familyCount = ideeFamilyCountAtLevel(level);

  // Taille maximale du secteur disponible entre familles.
  float sector = 6.28318 / familyCount;

  // Une famille de 2 ne prend jamais plus de 1/8 de ronde.
  float maxSpread = sector * 0.70;

  if (childCount <= 2) {
    maxSpread = min(maxSpread, 0.785f); // 1/8 de cercle
  }
  else if (childCount <= 4) {
    maxSpread = min(maxSpread, 1.15f);
  }
  else {
    maxSpread = min(maxSpread, 1.55f);
  }

  if (maxSpread < 0.18) maxSpread = 0.18;

  return maxSpread;
}

int ideeRadiusBaseForLevel(int level) {
  int nodeDiameter = 40;
  int minGap = 12;
  int minSpacing = nodeDiameter + minGap;

  int countLevel = ideeCountAtLevel(level);

  int radius = 46 + level * 54;

  // Répartition globale : assez de périmètre pour tous les cercles.
  int needGlobal = (countLevel * minSpacing) / 6;
  if (needGlobal > radius) radius = needGlobal;

  // Répartition par famille : une grande famille force sa ronde à grandir.
  if (level > 0) {
    for (int parent = 0; parent < ideeCount; parent++) {
      if (ideeLevel(parent) != level - 1) continue;

      int childCount = ideeChildCount(parent);
      if (childCount <= 1) continue;

      float maxSpread = ideeFamilyMaxSpread(parent, level);

      int needFamily = ((childCount - 1) * minSpacing) / maxSpread;

      if (needFamily > radius) radius = needFamily;
    }
  }

  return radius;
}

int ideeRadiusForLevel(int level) {
  int radius = 46;

  for (int l = 0; l <= level; l++) {
    int wanted = ideeRadiusBaseForLevel(l);

    if (l == 0) {
      radius = wanted;
    } else {
      // Si une ronde s’agrandit, toutes les rondes supérieures suivent.
      radius = max(wanted, radius + 58);
    }
  }

  return radius;
}

int ideeScaledRadius(int logicalRadius) {
  return (logicalRadius * ideeZoom) / 100;
}

float ideeRootAngle(int id) {
  int count = max(1, ideeCountAtLevel(0));
  int rank = ideeRankAtLevel(id);

  return -1.5708 + (6.28318 * rank) / count;
}

float ideeAngle(int id);

float ideeFamilyCenterAngle(int parent) {
  if (parent < 0) return -1.5708;
  return ideeAngle(parent);
}

float ideeAngle(int id) {
  int level = ideeLevel(id);

  // Première ronde : répartition égale.
  if (level == 0) {
    return ideeRootAngle(id);
  }

  int parent = idees[id].parent;
  int count = max(1, ideeChildCount(parent));
  int rank = ideeChildRank(id);

  float center = ideeFamilyCenterAngle(parent);

  int radius = ideeRadiusForLevel(level);

  int nodeDiameter = 40;
  int minGap = 12;
  int minSpacing = nodeDiameter + minGap;

  float spacingAngle = (float)minSpacing / max(60, radius);

  float maxSpread = ideeFamilyMaxSpread(parent, level);

  float naturalSpread = spacingAngle * max(0, count - 1);

  // Si la famille est petite, on ne l’étire pas trop.
  // Si elle est grande, la ronde aura déjà grandi pour garder l’espacement.
  float spread = min(naturalSpread, maxSpread);

  if (count == 1) {
    return center;
  }

  return ideeNormalizeAngle(center - spread / 2.0 + (spread * rank) / (count - 1));
}

void ideeGetPos(int id, int &x, int &y) {
  int cx = 160 + ideeOffsetX;
  int cy = 126 + ideeOffsetY;

  int radius = ideeScaledRadius(ideeRadiusForLevel(ideeLevel(id)));
  float a = ideeAngle(id);

  x = cx + cos(a) * radius;
  y = cy + sin(a) * radius;
}

void ideePointOnCircle(int logicalRadius, float angle, int &x, int &y) {
  int cx = 160 + ideeOffsetX;
  int cy = 126 + ideeOffsetY;

  int radius = ideeScaledRadius(logicalRadius);

  x = cx + cos(angle) * radius;
  y = cy + sin(angle) * radius;
}

float ideeMiddleAngleOfFamily(int parent) {
  int first = ideeFirstChild(parent);
  int last = ideeLastChild(parent);

  if (first < 0) return ideeFamilyCenterAngle(parent);

  float a1 = ideeAngle(first);
  float a2 = ideeAngle(last);

  float diff = ideeNormalizeAngle(a2 - a1);

  return ideeNormalizeAngle(a1 + diff / 2.0);
}

void ideeDrawArcSegment(int cx, int cy, int logicalRadius, float a1, float a2) {
  int radius = ideeScaledRadius(logicalRadius);

  float diff = ideeNormalizeAngle(a2 - a1);

  int steps = 18;

  int lastX = cx + cos(a1) * radius;
  int lastY = cy + sin(a1) * radius;

  for (int i = 1; i <= steps; i++) {
    float a = a1 + (diff * i) / steps;

    int x = cx + cos(a) * radius;
    int y = cy + sin(a) * radius;

    tft.drawLine(lastX, lastY, x, y, TFT_BLACK);
    tft.drawLine(lastX, lastY + 1, x, y + 1, TFT_DARKGREY);

    lastX = x;
    lastY = y;
  }
}

void ideeDrawLinks() {
  int cx = 160 + ideeOffsetX;
  int cy = 126 + ideeOffsetY;

  // 1) Arcs entre les cercles d’une même famille.
  for (int parent = -1; parent < ideeCount; parent++) {
    int count = ideeChildCount(parent);
    if (count < 2) continue;

    int previous = -1;

    for (int i = 0; i < ideeCount; i++) {
      if (idees[i].parent != parent) continue;

      if (previous >= 0) {
        int level = ideeLevel(i);
        int radius = ideeRadiusForLevel(level);

        ideeDrawArcSegment(
          cx,
          cy,
          radius,
          ideeAngle(previous),
          ideeAngle(i)
        );
      }

      previous = i;
    }
  }

  // 2) Jonctions perpendiculaires/radiales depuis le cercle source.
  for (int parent = 0; parent < ideeCount; parent++) {
    int first = ideeFirstChild(parent);
    if (first < 0) continue;

    int parentLevel = ideeLevel(parent);
    int childLevel = parentLevel + 1;

    int rParent = ideeRadiusForLevel(parentLevel);
    int rChild = ideeRadiusForLevel(childLevel);

    float aParent = ideeAngle(parent);
    float aMiddle = ideeMiddleAngleOfFamily(parent);

    int parentX, parentY;
    int hitX, hitY;

    // Départ : cercle source.
    ideeGetPos(parent, parentX, parentY);

    // Arrivée : même axe que le cercle source.
    // C’est donc une jonction perpendiculaire aux rondes.
    ideePointOnCircle(rChild, aParent, hitX, hitY);

    tft.drawLine(parentX, parentY, hitX, hitY, TFT_BLACK);
    tft.drawLine(parentX + 1, parentY, hitX + 1, hitY, TFT_DARKGREY);

    // Le point d’arrivée est ensuite relié au centre de sa famille sur la ronde.
    if (abs(aMiddle - aParent) > 0.03) {
      ideeDrawArcSegment(cx, cy, rChild, aParent, aMiddle);
    }
  }
}

// =====================================================
// CARTE
// =====================================================

void ideeDrawMap() {
  ideeState = IDEE_MAP;
  ideeSelected = -1;
  ideeButtonCount = 0;

  tft.fillScreen(TFT_WHITE);

  ideeDrawBackWhite();
  ideeDrawLinks();

  int nodeRadius = max(10, (17 * ideeZoom) / 100);
  int buttonRadius = max(14, nodeRadius + 4);

  for (int i = 0; i < ideeCount; i++) {
    int x, y;
    ideeGetPos(i, x, y);

    if (x < -45 || x > 365 || y < -10 || y > 285) continue;

    uint16_t col = ideeColors[idees[i].colorIndex];

    tft.fillCircle(x, y, nodeRadius, col);
    tft.drawCircle(x, y, nodeRadius, TFT_BLACK);

    if (ideeZoom >= 65) {
      drawBoldText(ideeShort(idees[i].title, 5), x, y, TFT_BLACK, col, MC_DATUM, 1);
    }

    ideeAddButton(
      x - buttonRadius,
      y - buttonRadius,
      buttonRadius * 2,
      buttonRadius * 2,
      idees[i].title,
      i
    );
  }

  ideeDrawBackWhite();
  ideeDrawMapTools();
}

// =====================================================
// DETAIL
// =====================================================

void ideeDrawDetail(int id) {
  ideeState = IDEE_DETAIL;
  ideeSelected = id;
  ideeButtonCount = 0;

  tft.fillScreen(TFT_WHITE);
  ideeHeaderBlack("Detail");

  uint16_t col = ideeColors[idees[id].colorIndex];

  tft.fillCircle(52, 70, 26, col);
  tft.drawCircle(52, 70, 26, TFT_BLACK);

  drawBoldText(ideeShort(idees[id].title, 14), 180, 58, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);

  String info = "Ronde " + String(ideeLevel(id) + 1);

  if (idees[id].parent >= 0) {
    info += " / parent ";
    info += ideeShort(idees[idees[id].parent].title, 8);
  } else {
    info += " / racine";
  }

  drawBoldText(info, 180, 84, TFT_DARKGREY, TFT_WHITE, MC_DATUM, 1);

  tft.drawRoundRect(12, 105, 296, 70, 8, TFT_BLACK);

  String txt = idees[id].text;
  if (txt.length() == 0) txt = "(aucun texte)";

  int start = 0;
  int y = 117;

  while (start < txt.length() && y < 170) {
    String line = txt.substring(start, start + 35);
    drawBoldText(line, 22, y, TFT_BLACK, TFT_WHITE, ML_DATUM, 1);
    start += 35;
    y += 15;
  }

  ideeAddButton(10, 184, 58, 30, "TXT", -20);
  ideeAddButton(75, 184, 58, 30, "COUL", -21);
  ideeAddButton(140, 184, 58, 30, "RAC", -22);
  ideeAddButton(245, 184, 62, 30, "SUP", -24);

  for (int i = 0; i < ideeButtonCount; i++) {
    uint16_t bg = TFT_BLACK;

    if (ideeButtons[i].index == -22) bg = TFT_GREEN;
    if (ideeButtons[i].index == -24) bg = TFT_RED;

    tft.fillRoundRect(ideeButtons[i].x, ideeButtons[i].y, ideeButtons[i].w, ideeButtons[i].h, 7, bg);

    drawBoldText(
      ideeButtons[i].label,
      ideeButtons[i].x + ideeButtons[i].w / 2,
      ideeButtons[i].y + ideeButtons[i].h / 2,
      TFT_WHITE,
      bg,
      MC_DATUM,
      1
    );
  }

  drawBoldText("RAC = nouvelle idee liee", 160, 229, TFT_DARKGREY, TFT_WHITE, MC_DATUM, 1);
}

// =====================================================
// CLAVIER
// =====================================================

void ideeDrawInput(String title) {
  ideeButtonCount = 0;

  tft.fillScreen(TFT_WHITE);
  ideeHeaderBlack(title);

  tft.drawRoundRect(10, 44, 300, 48, 8, TFT_BLACK);

  String show = ideeInput;
  if (show.length() > 34) show = show.substring(show.length() - 34);

  drawBoldText(show, 20, 68, TFT_BLACK, TFT_WHITE, ML_DATUM, 2);

  String keys1 = "AZERTYUIOP";
  String keys2 = "QSDFGHJKLM";
  String keys3 = "WXCVBN";

  int y1 = 104;
  int keyW = 29;
  int keyH = 28;

  for (int i = 0; i < keys1.length(); i++) {
    String k = keys1.substring(i, i + 1);
    int x = 8 + i * 31;

    tft.fillRoundRect(x, y1, keyW, keyH, 5, TFT_BLACK);
    drawBoldText(k, x + keyW / 2, y1 + keyH / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
    ideeAddButton(x, y1, keyW, keyH, k, k[0]);
  }

  for (int i = 0; i < keys2.length(); i++) {
    String k = keys2.substring(i, i + 1);
    int x = 8 + i * 31;
    int y = y1 + 34;

    tft.fillRoundRect(x, y, keyW, keyH, 5, TFT_BLACK);
    drawBoldText(k, x + keyW / 2, y + keyH / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
    ideeAddButton(x, y, keyW, keyH, k, k[0]);
  }

  for (int i = 0; i < keys3.length(); i++) {
    String k = keys3.substring(i, i + 1);
    int x = 48 + i * 31;
    int y = y1 + 68;

    tft.fillRoundRect(x, y, keyW, keyH, 5, TFT_BLACK);
    drawBoldText(k, x + keyW / 2, y + keyH / 2, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
    ideeAddButton(x, y, keyW, keyH, k, k[0]);
  }

  ideeAddButton(12, 210, 54, 26, "ESP", -30);
  ideeAddButton(74, 210, 54, 26, "EFF", -31);
  ideeAddButton(136, 210, 54, 26, ".", '.');
  ideeAddButton(198, 210, 54, 26, "OK", -32);
  ideeAddButton(260, 210, 48, 26, "ANN", -33);

  for (int i = ideeButtonCount - 5; i < ideeButtonCount; i++) {
    tft.fillRoundRect(ideeButtons[i].x, ideeButtons[i].y, ideeButtons[i].w, ideeButtons[i].h, 6, TFT_DARKGREY);

    drawBoldText(
      ideeButtons[i].label,
      ideeButtons[i].x + ideeButtons[i].w / 2,
      ideeButtons[i].y + ideeButtons[i].h / 2,
      TFT_WHITE,
      TFT_DARKGREY,
      MC_DATUM,
      1
    );
  }
}

// =====================================================
// ACTIONS
// =====================================================

void ideeNewProject() {
  if (ideeProjectCount >= IDEE_MAX_PROJECTS) return;

  ideeInput = "";
  ideeInputMode = 0;
  ideeState = IDEE_INPUT_PROJECT;
  ideeDrawInput("Titre projet");
}

void ideeNewIdea(int parent) {
  if (ideeCount >= IDEE_MAX_NODES) return;

  ideeNewParent = parent;
  ideeInput = "";
  ideeInputMode = 1;
  ideeEditTarget = ideeCount;

  ideeState = IDEE_INPUT_TITLE;
  ideeDrawInput("Titre idee");
}

void ideeNewText() {
  ideeInput = "";
  ideeInputMode = 2;
  ideeState = IDEE_INPUT_TEXT;
  ideeDrawInput("Texte idee");
}

void ideeEditText(int id) {
  ideeInput = idees[id].text;
  ideeInputMode = 3;
  ideeEditTarget = id;

  ideeState = IDEE_INPUT_TEXT;
  ideeDrawInput("Modifier texte");
}

void ideeDelete(int id) {
  if (id < 0 || id >= ideeCount) return;

  for (int i = 0; i < ideeCount; i++) {
    if (idees[i].parent == id) {
      idees[i].parent = idees[id].parent;
    }
  }

  for (int i = id; i < ideeCount - 1; i++) {
    idees[i] = idees[i + 1];
  }

  ideeCount--;

  for (int i = 0; i < ideeCount; i++) {
    if (idees[i].parent > id) idees[i].parent--;
  }

  ideeSaveMap();
  ideeDrawMap();
}

void ideeFinishInput() {
  if (ideeInputMode == 0) {
    if (ideeInput.length() == 0) ideeInput = "Projet";

    ideeProjects[ideeProjectCount] = ideeInput.substring(0, 16);
    ideeCurrentProject = ideeProjectCount;
    ideeProjectCount++;

    ideeCount = 0;
    ideeOffsetX = 0;
    ideeOffsetY = 0;
    ideeZoom = ideeZoomMax;

    ideeSaveProjects();
    ideeSaveMap();
    ideeDrawMap();
    return;
  }

  if (ideeInputMode == 1) {
    if (ideeInput.length() == 0) ideeInput = "Idee";

    idees[ideeCount].title = ideeInput.substring(0, 12);
    idees[ideeCount].text = "";
    idees[ideeCount].parent = ideeNewParent;
    idees[ideeCount].colorIndex = ideeCount % 8;

    ideeNewText();
    return;
  }

  if (ideeInputMode == 2) {
    idees[ideeCount].text = ideeInput.substring(0, 180);
    ideeCount++;

    ideeSaveMap();
    ideeDrawMap();
    return;
  }

  if (ideeInputMode == 3) {
    idees[ideeEditTarget].text = ideeInput.substring(0, 180);

    ideeSaveMap();
    ideeDrawDetail(ideeEditTarget);
    return;
  }
}

// =====================================================
// LANCEMENT
// =====================================================

void launchIdee() {
  appMode = MODE_IDEE;

  stopSDStartTouch();

  ideeZoom = ideeZoomMax;
  ideeOffsetX = 0;
  ideeOffsetY = 0;

  ideeLoadProjects();
  ideeDrawProjects();

  Serial.println("IDEE PROJETS PRETE");
}

// =====================================================
// UPDATE
// =====================================================

void updateIdee() {
  int x, y;

  if (!getTouch(x, y)) {
    ideeDragging = false;
    return;
  }

  if (ideeState == IDEE_MAP) {
    if (!ideeDragging) {
      ideeDragging = true;
      ideeDragStartX = x;
      ideeDragStartY = y;
      ideeDragBaseX = ideeOffsetX;
      ideeDragBaseY = ideeOffsetY;
    }

    int dx = x - ideeDragStartX;
    int dy = y - ideeDragStartY;

    if (abs(dx) > 14 || abs(dy) > 14) {
      ideeOffsetX = constrain(ideeDragBaseX + dx, -850, 850);
      ideeOffsetY = constrain(ideeDragBaseY + dy, -850, 850);

      ideeDrawMap();
      delay(30);
      return;
    }
  }

  if (millis() - ideeLastTouch < 160) return;
  ideeLastTouch = millis();

  if (y < 36 && x < 45) {
    if (ideeState == IDEE_PROJECTS) {
      drawSubMenu(currentMenu);
    }
    else if (ideeState == IDEE_MAP) {
      ideeDrawProjects();
    }
    else {
      if (ideeCurrentProject >= 0) ideeDrawMap();
      else ideeDrawProjects();
    }

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  for (int i = 0; i < ideeButtonCount; i++) {
    if (!ideeInButton(ideeButtons[i], x, y)) continue;

    int idx = ideeButtons[i].index;

    if (ideeState == IDEE_PROJECTS) {
      if (idx == -10) {
        ideeNewProject();
      }
      else if (idx <= -100) {
        int projectId = -100 - idx;
        ideeDeleteProject(projectId);
      }
      else if (idx >= 0) {
        ideeLoadMap(idx);
        ideeZoom = ideeZoomMax;
        ideeOffsetX = 0;
        ideeOffsetY = 0;
        ideeDrawMap();
      }
    }

    else if (ideeState == IDEE_MAP) {
      if (idx == -10) {
        ideeNewIdea(-1);
      }
      else if (idx == -11) {
        ideeOffsetX = 0;
        ideeOffsetY = 0;
        ideeDrawMap();
      }
      else if (idx == -12) {
        ideeZoom -= 10;
        if (ideeZoom < ideeZoomMin) ideeZoom = ideeZoomMin;
        ideeDrawMap();
      }
      else if (idx == -13) {
        ideeZoom += 10;
        if (ideeZoom > ideeZoomMax) ideeZoom = ideeZoomMax;
        ideeDrawMap();
      }
      else if (idx >= 0) {
        ideeDrawDetail(idx);
      }
    }

    else if (ideeState == IDEE_DETAIL) {
      if (idx == -20) {
        ideeEditText(ideeSelected);
      }
      else if (idx == -21) {
        idees[ideeSelected].colorIndex++;
        if (idees[ideeSelected].colorIndex > 7) idees[ideeSelected].colorIndex = 0;

        ideeSaveMap();
        ideeDrawDetail(ideeSelected);
      }
      else if (idx == -22) {
        ideeNewIdea(ideeSelected);
      }
      else if (idx == -24) {
        ideeDelete(ideeSelected);
      }
    }

    else if (
      ideeState == IDEE_INPUT_PROJECT ||
      ideeState == IDEE_INPUT_TITLE ||
      ideeState == IDEE_INPUT_TEXT
    ) {
      if (idx == -30) {
        ideeInput += " ";
      }
      else if (idx == -31) {
        if (ideeInput.length() > 0) ideeInput.remove(ideeInput.length() - 1);
      }
      else if (idx == -32) {
        ideeFinishInput();

        while (ts.touched()) delay(5);
        delay(80);
        return;
      }
      else if (idx == -33) {
        if (ideeCurrentProject >= 0) ideeDrawMap();
        else ideeDrawProjects();

        while (ts.touched()) delay(5);
        delay(80);
        return;
      }
      else {
        int maxLen = 16;

        if (ideeState == IDEE_INPUT_TEXT) maxLen = 180;
        else if (ideeState == IDEE_INPUT_TITLE) maxLen = 12;

        if (ideeInput.length() < maxLen) {
          ideeInput += char(idx);
        }
      }

      if (ideeState == IDEE_INPUT_PROJECT) ideeDrawInput("Titre projet");
      else if (ideeState == IDEE_INPUT_TITLE) ideeDrawInput("Titre idee");
      else ideeDrawInput("Texte idee");
    }

    while (ts.touched()) delay(5);
    ideeDragging = false;
    delay(80);
    return;
  }
}