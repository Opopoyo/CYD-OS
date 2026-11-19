#include "wifi_app.h"
#include "menu_core.h"

#include <WiFi.h>
#include <Preferences.h>

Preferences wifiPrefs;

enum WifiScreen {
  WIFI_SCAN,
  WIFI_LIST,
  WIFI_PASSWORD,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED
};

WifiScreen wifiScreen = WIFI_SCAN;

String wifiSSID[20];
int wifiRSSI[20];
int wifiCount = 0;
int wifiPage = 0;
int selectedWifi = -1;

String wifiPassword = "";
bool wifiMaj = false;
bool wifiNum = false;

WifiBtn wifiBtns[50];
int wifiBtnCount = 0;

void wifiAddBtn(int x, int y, int w, int h, String label, int index) {
  wifiBtns[wifiBtnCount].x = x;
  wifiBtns[wifiBtnCount].y = y;
  wifiBtns[wifiBtnCount].w = w;
  wifiBtns[wifiBtnCount].h = h;
  wifiBtns[wifiBtnCount].label = label;
  wifiBtns[wifiBtnCount].index = index;
  wifiBtnCount++;
}

bool wifiInBtn(WifiBtn b, int x, int y) {
  return (
    x >= b.x &&
    x <= b.x + b.w &&
    y >= b.y &&
    y <= b.y + b.h
  );
}

void wifiHeader(const char* title) {
  tft.fillRect(0, 0, 320, 36, TFT_BLACK);

  tft.fillTriangle(11, 18, 31, 7, 31, 29, TFT_WHITE);

  wifiAddBtn(0, 0, 45, 36, "BACK", -1);

  drawBoldText(
    title,
    165,
    18,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );
}

void wifiDrawButton(WifiBtn b, uint16_t bg = TFT_WHITE, uint16_t fg = TFT_BLACK) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 7, bg);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 7, TFT_BLACK);

  drawBoldText(
    b.label,
    b.x + b.w / 2,
    b.y + b.h / 2,
    fg,
    bg,
    MC_DATUM,
    1
  );
}

void wifiAutoConnect() {
  wifiPrefs.begin("wifi", true);

  String savedSSID = wifiPrefs.getString("ssid", "");
  String savedPASS = wifiPrefs.getString("pass", "");

  wifiPrefs.end();

  if (savedSSID == "") {
    Serial.println("Aucun WiFi sauvegarde");
    return;
  }

  Serial.println("Connexion auto WiFi");
  Serial.print("SSID sauvegarde: ");
  Serial.println(savedSSID);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  delay(300);

  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long start = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 8000
  ) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi auto connecte");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Echec WiFi auto");
  }
}

void wifiScanNetworks() {
  wifiScreen = WIFI_SCAN;
  wifiBtnCount = 0;

  tft.fillScreen(TFT_BLACK);
  wifiHeader("Wifi");

  drawBoldText(
    "Scan en cours...",
    160,
    120,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  delay(500);

  wifiCount = WiFi.scanNetworks();

  if (wifiCount > 20) {
    wifiCount = 20;
  }

  for (int i = 0; i < wifiCount; i++) {
    wifiSSID[i] = WiFi.SSID(i);
    wifiRSSI[i] = WiFi.RSSI(i);
  }

  wifiPage = 0;
  wifiScreen = WIFI_LIST;
}

void wifiDrawList() {
  wifiScreen = WIFI_LIST;
  wifiBtnCount = 0;

  tft.fillScreen(TFT_BLACK);
  wifiHeader("Wifi");

  wifiAddBtn(235, 4, 78, 28, "SCAN", -2);
  wifiDrawButton(wifiBtns[wifiBtnCount - 1], TFT_GREEN, TFT_BLACK);

  if (wifiCount <= 0) {
    drawBoldText(
      "Aucun reseau",
      160,
      120,
      TFT_WHITE,
      TFT_BLACK,
      MC_DATUM,
      2
    );
    return;
  }

  int start = wifiPage * 5;
  int end = start + 5;

  if (end > wifiCount) {
    end = wifiCount;
  }

  for (int i = start; i < end; i++) {
    int y = 45 + (i - start) * 34;

    String name = wifiSSID[i];

    if (name.length() > 19) {
      name = name.substring(0, 19);
    }

    String label = name + " " + String(wifiRSSI[i]);

    wifiAddBtn(10, y, 300, 29, label, i);
    wifiDrawButton(wifiBtns[wifiBtnCount - 1]);
  }

  int totalPages = (wifiCount + 4) / 5;

  if (totalPages < 1) {
    totalPages = 1;
  }

  tft.fillTriangle(105, 225, 125, 213, 125, 237, TFT_WHITE);
  wifiAddBtn(90, 205, 45, 35, "PREV", -3);

  drawBoldText(
    String(wifiPage + 1) + "/" + String(totalPages),
    160,
    225,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    1
  );

  tft.fillTriangle(215, 225, 195, 213, 195, 237, TFT_WHITE);
  wifiAddBtn(185, 205, 45, 35, "NEXT", -4);
}

void wifiDrawPasswordBox() {
  tft.fillRect(5, 38, 310, 34, TFT_BLACK);
  tft.drawRoundRect(5, 38, 310, 34, 6, TFT_WHITE);

  String affichage = wifiPassword;

  if (affichage.length() > 26) {
    affichage = affichage.substring(affichage.length() - 26);
  }

  drawBoldText(
    affichage,
    12,
    55,
    TFT_WHITE,
    TFT_BLACK,
    ML_DATUM,
    2
  );
}

void wifiBuildKeyboard() {
  wifiScreen = WIFI_PASSWORD;
  wifiBtnCount = 0;

  tft.fillScreen(TFT_BLACK);
  wifiHeader("Code Wifi");

  drawBoldText(
    wifiSSID[selectedWifi],
    160,
    25,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    1
  );

  wifiDrawPasswordBox();

  const char* row1a[] = {"a","z","e","r","t","y","u","i","o","p"};
  const char* row2a[] = {"q","s","d","f","g","h","j","k","l","m"};
  const char* row3a[] = {"w","x","c","v","b","n"};

  const char* row1n[] = {"1","2","3","4","5","6","7","8","9","0"};
  const char* row2n[] = {"@","#","&","-","_","=","+","/","?","!"};
  const char* row3n[] = {".",",",":",";","(",")"};

  int keyW = 28;
  int keyH = 29;
  int gap = 3;
  int startY = 78;

  for (int i = 0; i < 10; i++) {
    String lab = wifiNum ? row1n[i] : row1a[i];

    if (wifiMaj && !wifiNum) {
      lab.toUpperCase();
    }

    wifiAddBtn(
      5 + i * (keyW + gap),
      startY,
      keyW,
      keyH,
      lab,
      100
    );
  }

  for (int i = 0; i < 10; i++) {
    String lab = wifiNum ? row2n[i] : row2a[i];

    if (wifiMaj && !wifiNum) {
      lab.toUpperCase();
    }

    wifiAddBtn(
      15 + i * (keyW + gap),
      startY + 35,
      keyW,
      keyH,
      lab,
      100
    );
  }

  wifiAddBtn(5, startY + 70, 45, keyH, wifiNum ? "ABC" : "MAJ", -5);

  for (int i = 0; i < 6; i++) {
    String lab = wifiNum ? row3n[i] : row3a[i];

    if (wifiMaj && !wifiNum) {
      lab.toUpperCase();
    }

    wifiAddBtn(
      55 + i * (keyW + gap),
      startY + 70,
      keyW,
      keyH,
      lab,
      100
    );
  }

  wifiAddBtn(245, startY + 70, 70, keyH, "<-", -6);
  wifiAddBtn(5, startY + 105, 45, 32, wifiNum ? "ABC" : "123", -7);
  wifiAddBtn(55, startY + 105, 140, 32, "ESPACE", -8);
  wifiAddBtn(205, startY + 105, 105, 32, "OK", -9);

  for (int i = 1; i < wifiBtnCount; i++) {
    if (wifiBtns[i].label == "OK") {
      wifiDrawButton(wifiBtns[i], TFT_GREEN, TFT_BLACK);
    } else if (wifiBtns[i].label == "<-") {
      wifiDrawButton(wifiBtns[i], TFT_RED, TFT_WHITE);
    } else {
      wifiDrawButton(wifiBtns[i]);
    }
  }
}

void wifiConnect() {
  wifiScreen = WIFI_CONNECTING;
  wifiBtnCount = 0;

  tft.fillScreen(TFT_BLACK);
  wifiHeader("Connexion");

  drawBoldText(
    wifiSSID[selectedWifi],
    160,
    80,
    TFT_WHITE,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  drawBoldText(
    wifiPassword,
    160,
    110,
    TFT_LIGHTGREY,
    TFT_BLACK,
    MC_DATUM,
    1
  );

  drawBoldText(
    "Connexion...",
    160,
    145,
    TFT_YELLOW,
    TFT_BLACK,
    MC_DATUM,
    2
  );

  Serial.println("Connexion WiFi");
  Serial.print("SSID: ");
  Serial.println(wifiSSID[selectedWifi]);
  Serial.print("PASS: ");
  Serial.println(wifiPassword);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  delay(500);

  WiFi.begin(
    wifiSSID[selectedWifi].c_str(),
    wifiPassword.c_str()
  );

  unsigned long start = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 20000
  ) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi status: ");
  Serial.println(WiFi.status());

  tft.fillScreen(TFT_BLACK);
  wifiBtnCount = 0;
  wifiHeader("Wifi");

  if (WiFi.status() == WL_CONNECTED) {
    wifiScreen = WIFI_CONNECTED;

    wifiPrefs.begin("wifi", false);
    wifiPrefs.putString("ssid", wifiSSID[selectedWifi]);
    wifiPrefs.putString("pass", wifiPassword);
    wifiPrefs.end();

    Serial.println("WiFi sauvegarde");

    drawBoldText(
      "Connecte !",
      160,
      85,
      TFT_GREEN,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    drawBoldText(
      WiFi.localIP().toString(),
      160,
      120,
      TFT_WHITE,
      TFT_BLACK,
      MC_DATUM,
      2
    );
  } else {
    wifiScreen = WIFI_FAILED;

    drawBoldText(
      "Echec connexion",
      160,
      85,
      TFT_RED,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    String statusMsg = "Code: " + String(WiFi.status());

    drawBoldText(
      statusMsg,
      160,
      115,
      TFT_WHITE,
      TFT_BLACK,
      MC_DATUM,
      2
    );

    drawBoldText(
      "Verifie 2.4GHz/code",
      160,
      145,
      TFT_WHITE,
      TFT_BLACK,
      MC_DATUM,
      1
    );
  }

  wifiAddBtn(55, 180, 210, 42, "RETOUR", -10);
  wifiDrawButton(wifiBtns[wifiBtnCount - 1], TFT_WHITE, TFT_BLACK);
}

void launchWifiApp() {
  appMode = MODE_WIFI;

  stopSDStartTouch();

  wifiScanNetworks();
  wifiDrawList();
}

void updateWifiApp() {
  int x, y;

  if (!getTouch(x, y)) {
    return;
  }

  for (int i = 0; i < wifiBtnCount; i++) {
    if (!wifiInBtn(wifiBtns[i], x, y)) {
      continue;
    }

    int idx = wifiBtns[i].index;
    String lab = wifiBtns[i].label;

    if (idx == -1) {
      drawSubMenu(currentMenu);
    }

    else if (idx == -2) {
      wifiScanNetworks();
      wifiDrawList();
    }

    else if (idx == -3) {
      if (wifiPage > 0) {
        wifiPage--;
      }

      wifiDrawList();
    }

    else if (idx == -4) {
      int totalPages = (wifiCount + 4) / 5;

      if (wifiPage < totalPages - 1) {
        wifiPage++;
      }

      wifiDrawList();
    }

    else if (idx == 100) {
      wifiPassword += lab;
      wifiDrawPasswordBox();
    }

    else if (idx == -5) {
      wifiMaj = !wifiMaj;
      wifiBuildKeyboard();
    }

    else if (idx == -6) {
      if (wifiPassword.length() > 0) {
        wifiPassword.remove(wifiPassword.length() - 1);
      }

      wifiDrawPasswordBox();
    }

    else if (idx == -7) {
      wifiNum = !wifiNum;
      wifiBuildKeyboard();
    }

    else if (idx == -8) {
      wifiPassword += " ";
      wifiDrawPasswordBox();
    }

    else if (idx == -9) {
      wifiConnect();
    }

    else if (idx == -10) {
      wifiDrawList();
    }

    else if (idx >= 0) {
      selectedWifi = idx;
      wifiPassword = "";
      wifiMaj = false;
      wifiNum = false;

      wifiBuildKeyboard();
    }

    while (ts.touched()) {
      delay(5);
    }

    delay(80);
    return;
  }
}