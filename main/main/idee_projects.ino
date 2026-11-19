#include "idee.h"
#include "menu_core.h"

String ideeProjectPath(int id) {
  return String(IDEE_DIR) + "/p" + String(id) + ".txt";
}

void ideeSaveProjects() {
  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir(IDEE_DIR);
  SD.remove(IDEE_PROJECT_FILE);

  File f = SD.open(IDEE_PROJECT_FILE, FILE_WRITE);

  if (f) {
    for (int i = 0; i < ideeProjectCount; i++) {
      f.println(ideeClean(ideeProjects[i]));
    }
    f.close();
  }

  stopSDStartTouch();
}

void ideeLoadProjects() {
  ideeProjectCount = 0;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir(IDEE_DIR);

  File f = SD.open(IDEE_PROJECT_FILE);

  if (!f) {
    stopSDStartTouch();
    return;
  }

  while (f.available() && ideeProjectCount < IDEE_MAX_PROJECTS) {
    String line = f.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      ideeProjects[ideeProjectCount] = line;
      ideeProjectCount++;
    }
  }

  f.close();
  stopSDStartTouch();
}

void ideeDeleteProject(int id) {
  if (id < 0 || id >= ideeProjectCount) return;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.remove(ideeProjectPath(id));

  for (int i = id; i < ideeProjectCount - 1; i++) {
    ideeProjects[i] = ideeProjects[i + 1];

    String oldPath = ideeProjectPath(i + 1);
    String newPath = ideeProjectPath(i);

    if (SD.exists(oldPath)) {
      SD.remove(newPath);
      SD.rename(oldPath, newPath);
    }
  }

  ideeProjectCount--;

  stopSDStartTouch();

  if (ideeCurrentProject == id) ideeCurrentProject = -1;
  if (ideeCurrentProject > id) ideeCurrentProject--;

  ideeSaveProjects();
  ideeDrawProjects();
}

void ideeDrawProjects() {
  ideeState = IDEE_PROJECTS;
  ideeButtonCount = 0;
  ideeCurrentProject = -1;

  tft.fillScreen(TFT_WHITE);

  tft.fillRect(0, 0, 320, 36, TFT_BLACK);
  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);
  drawBoldText("Idees", 160, 18, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  drawBoldText("Galerie de projets", 160, 58, TFT_BLACK, TFT_WHITE, MC_DATUM, 2);

  tft.fillCircle(292, 55, 18, TFT_BLACK);
  drawBoldText("+", 292, 55, TFT_WHITE, TFT_BLACK, MC_DATUM, 4);
  ideeAddButton(270, 36, 45, 40, "+", -10);

  if (ideeProjectCount == 0) {
    drawBoldText("Aucun projet", 160, 112, TFT_DARKGREY, TFT_WHITE, MC_DATUM, 2);
    drawBoldText("Clique + pour commencer", 160, 145, TFT_BLACK, TFT_WHITE, MC_DATUM, 1);
    return;
  }

  int y = 86;

  for (int i = 0; i < ideeProjectCount; i++) {
    tft.fillRoundRect(18, y, 220, 30, 8, TFT_BLACK);
    drawBoldText(ideeShort(ideeProjects[i], 18), 128, y + 15, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
    ideeAddButton(18, y, 220, 30, ideeProjects[i], i);

    tft.fillRoundRect(248, y, 48, 30, 8, TFT_RED);
    drawBoldText("SUP", 272, y + 15, TFT_WHITE, TFT_RED, MC_DATUM, 1);
    ideeAddButton(248, y, 48, 30, "SUP", -100 - i);

    y += 38;
  }
}

void ideeSaveMap() {
  if (ideeCurrentProject < 0) return;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  SD.mkdir(IDEE_DIR);

  String path = ideeProjectPath(ideeCurrentProject);
  SD.remove(path);

  File f = SD.open(path, FILE_WRITE);

  if (f) {
    for (int i = 0; i < ideeCount; i++) {
      f.print(ideeClean(idees[i].title));
      f.print("|");
      f.print(ideeClean(idees[i].text));
      f.print("|");
      f.print(idees[i].parent);
      f.print("|");
      f.println(idees[i].colorIndex);
    }

    f.close();
  }

  stopSDStartTouch();
}

void ideeLoadMap(int projectId) {
  ideeCurrentProject = projectId;
  ideeCount = 0;
  ideeOffsetX = 0;
  ideeOffsetY = 0;

  if (!startSD()) {
    stopSDStartTouch();
    return;
  }

  File f = SD.open(ideeProjectPath(projectId));

  if (!f) {
    stopSDStartTouch();
    return;
  }

  while (f.available() && ideeCount < IDEE_MAX_NODES) {
    String line = f.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;

    int p1 = line.indexOf('|');
    int p2 = line.indexOf('|', p1 + 1);
    int p3 = line.indexOf('|', p2 + 1);

    if (p1 < 0 || p2 < 0 || p3 < 0) continue;

    idees[ideeCount].title = line.substring(0, p1);
    idees[ideeCount].text = line.substring(p1 + 1, p2);
    idees[ideeCount].parent = line.substring(p2 + 1, p3).toInt();
    idees[ideeCount].colorIndex = line.substring(p3 + 1).toInt();

    if (idees[ideeCount].parent >= ideeCount) idees[ideeCount].parent = -1;
    if (idees[ideeCount].parent < -1) idees[ideeCount].parent = -1;

    idees[ideeCount].colorIndex = constrain(idees[ideeCount].colorIndex, 0, 7);

    ideeCount++;
  }

  f.close();
  stopSDStartTouch();
}