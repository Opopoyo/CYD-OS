#include "meteo.h"
#include "menu_core.h"
#include "wifi_app.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define METEO_LAT "48.8566"
#define METEO_LON "2.3522"

float metTemp = 0;
float metHum = 0;
float metPress = 0;
float metWind = 0;

int metCode = 0;
int metAqi = -1;
float metPM10 = 0;
float metPM25 = 0;

bool meteoOk = false;
bool airOk = false;

unsigned long metLastTouch = 0;
unsigned long metLastRefresh = 0;

// =====================================================
// TEXTES
// =====================================================

String meteoTextFromCode(int code) {
  if (code == 0) return "Soleil";
  if (code == 1 || code == 2) return "Peu nuageux";
  if (code == 3) return "Nuageux";
  if (code == 45 || code == 48) return "Brouillard";
  if (code >= 51 && code <= 57) return "Bruine";
  if (code >= 61 && code <= 67) return "Pluie";
  if (code >= 71 && code <= 77) return "Neige";
  if (code >= 80 && code <= 82) return "Averses";
  if (code >= 95) return "Orage";
  return "Inconnu";
}

String airText(int aqi) {
  if (aqi < 0) return "Inconnue";
  if (aqi <= 20) return "Excellente";
  if (aqi <= 40) return "Bonne";
  if (aqi <= 60) return "Moyenne";
  if (aqi <= 80) return "Mauvaise";
  if (aqi <= 100) return "Tres mauvaise";
  return "Extreme";
}

uint16_t metColorTemp(float t) {
  if (t < 5) return TFT_CYAN;
  if (t < 15) return TFT_BLUE;
  if (t < 25) return TFT_GREEN;
  if (t < 32) return TFT_ORANGE;
  return TFT_RED;
}

uint16_t metColorAqi(int aqi) {
  if (aqi < 0) return TFT_DARKGREY;
  if (aqi <= 20) return TFT_GREEN;
  if (aqi <= 40) return TFT_CYAN;
  if (aqi <= 60) return TFT_YELLOW;
  if (aqi <= 80) return TFT_ORANGE;
  return TFT_RED;
}

// =====================================================
// HTTP
// =====================================================

bool meteoHttpGet(String url, String &payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi non connecte");
    return false;
  }

  HTTPClient http;
  http.setTimeout(12000);

  Serial.println("URL METEO:");
  Serial.println(url);

  if (!http.begin(url)) {
    Serial.println("HTTP begin ECHEC");
    return false;
  }

  int code = http.GET();

  Serial.print("HTTP code = ");
  Serial.println(code);

  payload = http.getString();

  Serial.print("Payload taille = ");
  Serial.println(payload.length());

  if (code != 200) {
    Serial.println("Payload erreur :");
    Serial.println(payload);
    http.end();
    return false;
  }

  http.end();
  return true;
}

// =====================================================
// FETCH METEO
// =====================================================

bool meteoFetchWeather() {
  String url =
    "http://api.open-meteo.com/v1/forecast?"
    "latitude=" METEO_LAT
    "&longitude=" METEO_LON
    "&current=temperature_2m,relative_humidity_2m,weather_code,surface_pressure,wind_speed_10m"
    "&timezone=Europe%2FParis";

  String payload;

  if (!meteoHttpGet(url, payload)) {
    Serial.println("meteoFetchWeather ECHEC HTTP");
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("JSON meteo erreur : ");
    Serial.println(err.c_str());
    return false;
  }

  metTemp = doc["current"]["temperature_2m"] | 0.0;
  metHum = doc["current"]["relative_humidity_2m"] | 0.0;
  metCode = doc["current"]["weather_code"] | 0;
  metPress = doc["current"]["surface_pressure"] | 0.0;
  metWind = doc["current"]["wind_speed_10m"] | 0.0;

  Serial.println("METEO OK");
  Serial.print("Temp = "); Serial.println(metTemp);
  Serial.print("Hum = "); Serial.println(metHum);
  Serial.print("Pression = "); Serial.println(metPress);

  return true;
}

bool meteoFetchAir() {
  String url =
    "http://air-quality-api.open-meteo.com/v1/air-quality?"
    "latitude=" METEO_LAT
    "&longitude=" METEO_LON
    "&current=european_aqi,pm10,pm2_5"
    "&timezone=Europe%2FParis";

  String payload;

  if (!meteoHttpGet(url, payload)) {
    Serial.println("meteoFetchAir ECHEC HTTP");
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("JSON air erreur : ");
    Serial.println(err.c_str());
    return false;
  }

  metAqi = doc["current"]["european_aqi"] | -1;
  metPM10 = doc["current"]["pm10"] | 0.0;
  metPM25 = doc["current"]["pm2_5"] | 0.0;

  Serial.println("AIR OK");
  Serial.print("AQI = "); Serial.println(metAqi);
  Serial.print("PM10 = "); Serial.println(metPM10);
  Serial.print("PM2.5 = "); Serial.println(metPM25);

  return true;
}

// =====================================================
// UI
// =====================================================

void meteoHeader() {
  tft.fillRect(0, 0, 320, 34, TFT_BLACK);
  tft.fillTriangle(11, 17, 31, 6, 31, 28, TFT_WHITE);

  drawBoldText("Meteo Paris", 150, 17, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);

  tft.fillRoundRect(248, 5, 64, 24, 6, TFT_DARKGREY);
  drawBoldText("MAJ", 280, 17, TFT_WHITE, TFT_DARKGREY, MC_DATUM, 1);
}

void meteoCard(int x, int y, int w, int h, String title, String value, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 10, TFT_BLACK);
  tft.drawRoundRect(x, y, w, h, 10, col);

  drawBoldText(title, x + w / 2, y + 16, TFT_LIGHTGREY, TFT_BLACK, MC_DATUM, 1);
  drawBoldText(value, x + w / 2, y + 42, col, TFT_BLACK, MC_DATUM, 2);
}

void meteoDrawBackground() {
  for (int y = 0; y < 240; y += 4) {
    int r = 4 + y / 12;
    int g = 8 + y / 10;
    int b = 30 + y / 3;

    tft.fillRect(
      0,
      y,
      320,
      4,
      tft.color565(
        constrain(r, 0, 40),
        constrain(g, 0, 60),
        constrain(b, 35, 145)
      )
    );
  }

  for (int i = 0; i < 25; i++) {
    int x = random(5, 315);
    int y = random(40, 220);
    tft.drawPixel(x, y, TFT_CYAN);
  }
}

void meteoDrawUI() {
  meteoDrawBackground();
  meteoHeader();

  if (!meteoOk) {
    drawBoldText("Erreur meteo", 160, 95, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    drawBoldText("Voir moniteur serie", 160, 125, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
    drawBoldText("Bouton MAJ pour reessayer", 160, 150, TFT_YELLOW, TFT_BLACK, MC_DATUM, 1);
    return;
  }

  meteoCard(
    10,
    44,
    145,
    62,
    "Temperature",
    String(metTemp, 1) + " C",
    metColorTemp(metTemp)
  );

  meteoCard(
    165,
    44,
    145,
    62,
    "Temps",
    meteoTextFromCode(metCode),
    TFT_YELLOW
  );

  meteoCard(
    10,
    114,
    145,
    52,
    "Humidite",
    String((int)metHum) + " %",
    TFT_CYAN
  );

  meteoCard(
    165,
    114,
    145,
    52,
    "Barometre",
    String(metPress, 0) + " hPa",
    TFT_GREEN
  );

  meteoCard(
    10,
    174,
    145,
    52,
    "Vent",
    String(metWind, 1) + " km/h",
    TFT_ORANGE
  );

  if (airOk) {
    meteoCard(
      165,
      174,
      145,
      52,
      "Air AQI",
      String(metAqi) + " " + airText(metAqi),
      metColorAqi(metAqi)
    );
  } else {
    meteoCard(
      165,
      174,
      145,
      52,
      "Air AQI",
      "Erreur",
      TFT_RED
    );
  }
}

// =====================================================
// REFRESH
// =====================================================

void meteoRefresh() {
  tft.fillScreen(TFT_BLACK);
  drawBoldText("Connexion meteo...", 160, 100, TFT_CYAN, TFT_BLACK, MC_DATUM, 2);
  drawBoldText("Paris / Ile-de-France", 160, 128, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

  meteoOk = false;
  airOk = false;

  meteoOk = meteoFetchWeather();

  if (meteoOk) {
    airOk = meteoFetchAir();
  }

  metLastRefresh = millis();

  meteoDrawUI();
}

// =====================================================
// LANCEMENT
// =====================================================

void launchMeteo() {
  if (WiFi.status() != WL_CONNECTED) {
    launchWifiApp();
    return;
  }

  appMode = MODE_METEO;

  stopSDStartTouch();

  meteoRefresh();

  Serial.println("METEO PRETE");
}

// =====================================================
// UPDATE
// =====================================================

void updateMeteo() {
  int x, y;

  if (!getTouch(x, y)) {
    return;
  }

  if (millis() - metLastTouch < 180) {
    return;
  }

  metLastTouch = millis();

  if (y < 34 && x < 45) {
    drawSubMenu(currentMenu);

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  if (x >= 248 && x <= 312 && y >= 5 && y <= 29) {
    meteoRefresh();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }
}