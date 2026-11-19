#ifndef IDEE_H
#define IDEE_H

#include <Arduino.h>

#define IDEE_MAX_PROJECTS 8
#define IDEE_MAX_NODES    30
#define IDEE_DIR          "/Idees"
#define IDEE_PROJECT_FILE "/Idees/projets.txt"

enum IdeeState {
  IDEE_PROJECTS,
  IDEE_INPUT_PROJECT,
  IDEE_MAP,
  IDEE_DETAIL,
  IDEE_INPUT_TITLE,
  IDEE_INPUT_TEXT
};

struct IdeeButton {
  int x, y, w, h;
  String label;
  int index;
};

struct IdeeNode {
  String title;
  String text;
  int parent;
  int colorIndex;
};

extern String ideeProjects[IDEE_MAX_PROJECTS];
extern int ideeProjectCount;
extern int ideeCurrentProject;

extern IdeeNode idees[IDEE_MAX_NODES];
extern int ideeCount;

extern IdeeState ideeState;
extern IdeeButton ideeButtons[80];
extern int ideeButtonCount;

extern int ideeSelected;
extern int ideeNewParent;
extern int ideeEditTarget;

extern String ideeInput;
extern int ideeInputMode;

extern int ideeOffsetX;
extern int ideeOffsetY;

void launchIdee();
void updateIdee();

void ideeDrawProjects();
void ideeLoadProjects();
void ideeSaveProjects();
void ideeDeleteProject(int id);

void ideeLoadMap(int projectId);
void ideeSaveMap();

void ideeDrawMap();
void ideeDrawDetail(int id);
void ideeDrawInput(String title);

void ideeAddButton(int x, int y, int w, int h, String label, int index);
bool ideeInButton(IdeeButton b, int x, int y);

String ideeClean(String s);
String ideeShort(String s, int n);
String ideeProjectPath(int id);

#endif