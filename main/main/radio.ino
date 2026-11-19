#include "radio.h"
#include "menu_core.h"
#include "wifi_app.h"
#include "audio_system.h"

#include <WiFi.h>

// =====================================================
// CONFIG PROXY PYTHON SUR TON PC WINDOWS
// =====================================================

#define RADIO_HOST "192.168.1.100"   // <-- IP de ton PC Windows
#define RADIO_PORT 8080

#define RADIO_SAMPLE_RATE 22050
#define RADIO_BUFFER_SIZE 512

WiFiClient radioClient;

String radioNames[] = {
  "France Inter",
  "FIP",
  "1FM Disco"
};

String radioPaths[] = {
  "/franceinter.raw",
  "/fip.raw",
  "/disco.raw"
};

const int radioCount = 3;

int radioIndex = 0;
int radioVolume = 70;

bool radioPlaying = false;
bool radioConnecting = false;
bool radioHeaderOk = false;

unsigned long radioLastTouch = 0;
unsigned long radioLastAnim = 0;
unsigned long radioLastSampleMicros = 0;

unsigned long radioSampleInterval = 1000000UL / RADIO_SAMPLE_RATE;

String radioHeader = "";

uint8_t radioBuf[RADIO_BUFFER_SIZE];
int radioBufLen = 0;
int radioBufPos = 0;

// =====================================================
// UI
// =====================================================

uint16_t radioBg(int y) {
  return tft.color565(
    constrain(3 + y / 16, 0, 40),
    constrain(4 + y / 20, 0, 45),
    constrain(25 + y / 3, 30, 150)
  );
}

void radioBackground() {
  for (int y = 0; y < 240; y += 4) {
    tft.fillRect(0, y, 320, 4, radioBg(y));
  }

  for (int i = 0; i < 35; i++) {
    int x = random(0, 320);
    int y = random(38, 225);

    uint16_t c = TFT_CYAN;
    if (i % 3 == 1) c = TFT_MAGENTA;
    if (i % 3 == 2) c = TFT_YELLOW;

    tft.drawPixel(x, y, c);
  }
}

void radioHeaderUI() {
  tft.fillRect(0, 0, 320, 34, TFT_BLACK);
  tft.fillTriangle(11, 17, 31, 6, 31, 28, TFT_WHITE);

  drawBoldText("Radio", 160, 17, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

void radioButton(int x, int y, int w, int h, String txt, uint16_t bg, uint16_t fg) {
  tft.fillRoundRect(x, y, w, h, 8, bg);
  tft.drawRoundRect(x, y, w, h, 8, TFT_DARKGREY);
  drawBoldText(txt, x + w / 2, y + h / 2, fg, bg, MC_DATUM, 1);
}

void radioDrawSpeaker() {
  int cx = 160;
  int cy = 104;

  tft.fillCircle(cx, cy, 52, TFT_BLACK);
  tft.drawCircle(cx, cy, 52, TFT_CYAN);
  tft.drawCircle(cx, cy, 40, TFT_MAGENTA);
  tft.drawCircle(cx, cy, 28, TFT_BLUE);

  uint16_t core = radioPlaying ? TFT_GREEN : TFT_DARKGREY;

  tft.fillCircle(cx, cy, 18, core);
  tft.drawCircle(cx, cy, 18, TFT_WHITE);

  if (radioPlaying && millis() - radioLastAnim > 160) {
    radioLastAnim = millis();

    int r = 58 + random(0, 10);
    tft.drawCircle(cx, cy, r, TFT_YELLOW);
    tft.drawCircle(cx, cy, r + 8, TFT_CYAN);
  }
}

void radioDrawUI() {
  radioBackground();
  radioHeaderUI();

  drawBoldText(
    radioNames[radioIndex],
    160,
    50,
    TFT_YELLOW,
    radioBg(50),
    MC_DATUM,
    2
  );

  String status = "STOP";
  uint16_t col = TFT_LIGHTGREY;

  if (radioConnecting) {
    status = "CONNEXION...";
    col = TFT_ORANGE;
  }
  else if (radioPlaying) {
    status = "EN DIRECT";
    col = TFT_GREEN;
  }

  drawBoldText(status, 160, 72, col, radioBg(72), MC_DATUM, 1);

  radioDrawSpeaker();

  radioButton(18, 91, 55, 38, "<<", TFT_BLACK, TFT_WHITE);
  radioButton(247, 91, 55, 38, ">>", TFT_BLACK, TFT_WHITE);

  radioButton(
    103,
    162,
    114,
    40,
    radioPlaying ? "STOP" : "PLAY",
    radioPlaying ? TFT_RED : TFT_GREEN,
    TFT_BLACK
  );

  radioButton(22, 212, 58, 22, "V-", TFT_DARKGREY, TFT_WHITE);
  radioButton(240, 212, 58, 22, "V+", TFT_DARKGREY, TFT_WHITE);

  drawBoldText(
    "Vol " + String(radioVolume) + "%",
    160,
    224,
    TFT_WHITE,
    radioBg(224),
    MC_DATUM,
    1
  );
}

void radioMessage(String msg, uint16_t col) {
  tft.fillRoundRect(35, 82, 250, 70, 10, TFT_BLACK);
  tft.drawRoundRect(35, 82, 250, 70, 10, col);
  drawBoldText(msg, 160, 112, col, TFT_BLACK, MC_DATUM, 2);
}

// =====================================================
// STREAM PCM
// =====================================================

void radioStop() {
  radioPlaying = false;
  radioConnecting = false;
  radioHeaderOk = false;

  radioHeader = "";

  radioClient.stop();

  radioBufLen = 0;
  radioBufPos = 0;

  audioSystemStop();

  radioDrawUI();
}

bool radioReadHeader() {
  while (radioClient.available() && !radioHeaderOk) {
    char c = radioClient.read();
    radioHeader += c;

    if (radioHeader.endsWith("\r\n\r\n")) {
      radioHeaderOk = true;

      Serial.println("HEADER RADIO:");
      Serial.println(radioHeader);

      return true;
    }

    if (radioHeader.length() > 1000) {
      radioHeaderOk = true;
      return true;
    }
  }

  return radioHeaderOk;
}

bool radioFillBuffer() {
  if (!radioClient.connected() && !radioClient.available()) {
    return false;
  }

  if (!radioHeaderOk) {
    radioReadHeader();
    return true;
  }

  if (radioClient.available() <= 0) {
    return true;
  }

  radioBufLen = radioClient.read(radioBuf, RADIO_BUFFER_SIZE);
  radioBufPos = 0;

  return radioBufLen > 0;
}

void radioPlaySample() {
  if (!radioPlaying) return;

  if (!radioHeaderOk || radioBufPos + 1 >= radioBufLen) {
    if (!radioFillBuffer()) {
      Serial.println("Radio : fin ou coupure flux");
      radioStop();
      return;
    }
  }

  if (!radioHeaderOk) return;
  if (radioBufPos + 1 >= radioBufLen) return;

  uint8_t lo = radioBuf[radioBufPos++];
  uint8_t hi = radioBuf[radioBufPos++];

  int16_t s = (int16_t)((hi << 8) | lo);

  int sample = (s >> 8) + 128;
  sample = constrain(sample, 0, 255);

  int centered = sample - 128;
  int scaled = 128 + ((centered * radioVolume) / 100);
  scaled = constrain(scaled, 0, 255);

  audioSystemWrite((uint8_t)scaled);
}

void radioAudioUpdate() {
  if (!radioPlaying) return;

  int guard = 0;

  while (
    radioPlaying &&
    micros() - radioLastSampleMicros >= radioSampleInterval &&
    guard < 80
  ) {
    radioLastSampleMicros += radioSampleInterval;
    radioPlaySample();
    guard++;
  }
}

// =====================================================
// CONNEXION
// =====================================================

bool radioConnect() {
  radioStop();

  if (WiFi.status() != WL_CONNECTED) {
    launchWifiApp();
    return false;
  }

  radioConnecting = true;
  radioDrawUI();

  Serial.println("Connexion proxy radio...");
  Serial.print("Host : ");
  Serial.println(RADIO_HOST);
  Serial.print("Path : ");
  Serial.println(radioPaths[radioIndex]);

  if (!radioClient.connect(RADIO_HOST, RADIO_PORT)) {
    Serial.println("Connexion proxy ECHEC");

    radioConnecting = false;
    radioPlaying = false;

    radioDrawUI();
    radioMessage("Proxy introuvable", TFT_RED);

    delay(1000);
    radioDrawUI();

    return false;
  }

  radioClient.print("GET ");
  radioClient.print(radioPaths[radioIndex]);
  radioClient.println(" HTTP/1.0");

  radioClient.print("Host: ");
  radioClient.println(RADIO_HOST);

  radioClient.println("Connection: close");
  radioClient.println();

  radioHeaderOk = false;
  radioHeader = "";

  radioBufLen = 0;
  radioBufPos = 0;

  audioSystemEnable();

  radioLastSampleMicros = micros();

  radioConnecting = false;
  radioPlaying = true;

  radioDrawUI();

  Serial.println("Radio proxy connectee");

  return true;
}

// =====================================================
// ACTIONS
// =====================================================

void radioToggle() {
  if (radioPlaying || radioConnecting) {
    radioStop();
  } else {
    radioConnect();
  }
}

void radioNext() {
  radioStop();

  radioIndex++;
  if (radioIndex >= radioCount) radioIndex = 0;

  radioDrawUI();
}

void radioPrev() {
  radioStop();

  radioIndex--;
  if (radioIndex < 0) radioIndex = radioCount - 1;

  radioDrawUI();
}

void radioVolPlus() {
  radioVolume += 10;
  if (radioVolume > 100) radioVolume = 100;

  radioDrawUI();
}

void radioVolMinus() {
  radioVolume -= 10;
  if (radioVolume < 0) radioVolume = 0;

  radioDrawUI();
}

void radioExit() {
  radioPlaying = false;
  radioConnecting = false;

  radioClient.stop();
  audioSystemStop();

  drawSubMenu(currentMenu);
}

// =====================================================
// LANCEMENT
// =====================================================

void launchRadio() {
  if (WiFi.status() != WL_CONNECTED) {
    launchWifiApp();
    return;
  }

  appMode = MODE_RADIO;

  stopSDStartTouch();
  audioSystemStop();

  radioIndex = 0;
  radioVolume = 70;

  radioPlaying = false;
  radioConnecting = false;
  radioHeaderOk = false;

  radioBufLen = 0;
  radioBufPos = 0;

  radioDrawUI();

  Serial.println("RADIO PCM PROXY PRETE");
}

// =====================================================
// UPDATE
// =====================================================

void updateRadio() {
  radioAudioUpdate();

  if (radioPlaying) {
    radioDrawSpeaker();
  }

  int x, y;

  if (!getTouch(x, y)) return;

  if (millis() - radioLastTouch < 220) return;
  radioLastTouch = millis();

  if (y < 34 && x < 45) {
    radioExit();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (x >= 103 && x <= 217 && y >= 162 && y <= 202) {
    radioToggle();
  }

  else if (x >= 18 && x <= 73 && y >= 91 && y <= 129) {
    radioPrev();
  }

  else if (x >= 247 && x <= 302 && y >= 91 && y <= 129) {
    radioNext();
  }

  else if (x >= 22 && x <= 80 && y >= 212 && y <= 234) {
    radioVolMinus();
  }

  else if (x >= 240 && x <= 298 && y >= 212 && y <= 234) {
    radioVolPlus();
  }

  while (ts.touched()) delay(5);
  delay(80);
}