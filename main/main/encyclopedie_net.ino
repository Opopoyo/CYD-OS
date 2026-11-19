#include "encyclopedie.h"
#include "menu_core.h"
#include "wifi_app.h"
#include "text_engine.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define ENC_MAX_RESULTS 5
#define ENC_TEXT_MAX 4200

extern String encQuery;
extern String encResults[ENC_MAX_RESULTS];
extern int encResultPageIds[ENC_MAX_RESULTS];
extern int encResultCount;

extern String encArticleTitle;
extern String encArticleText;
extern int encPage;

String encUrlEncode(String s) {
  String out = "";

  for (int i = 0; i < s.length(); i++) {
    uint8_t c = s[i];

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9')) {
      out += (char)c;
    }
    else if (c == ' ') {
      out += "%20";
    }
    else {
      char buf[4];
      sprintf(buf, "%%%02X", c);
      out += buf;
    }
  }

  return out;
}

bool encHttpGetOnce(String url, String &payload) {
  if (WiFi.status() != WL_CONNECTED) {
    launchWifiApp();
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(15000);
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
    Serial.println("ENC HTTP begin ECHEC");
    return false;
  }

  http.addHeader("User-Agent", "CYD2USB-Encyclopedie/1.0 alex.loison@gmail.com");
  http.addHeader("Api-User-Agent", "CYD2USB-Encyclopedie/1.0 alex.loison@gmail.com");
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  payload = http.getString();

  Serial.print("ENC HTTP code = ");
  Serial.println(code);
  Serial.print("ENC payload taille = ");
  Serial.println(payload.length());

  http.end();

  return code == 200 && payload.length() > 0;
}

bool encHttpGet(String url, String &payload) {
  payload = "";

  for (int i = 0; i < 3; i++) {
    Serial.print("ENC tentative ");
    Serial.println(i + 1);
    Serial.println(url);

    if (encHttpGetOnce(url, payload)) {
      return true;
    }

    delay(350 + i * 450);
  }

  return false;
}

// =====================================================
// RECHERCHE PLUS FIABLE : PREFIXSEARCH puis SEARCH
// On garde le PAGEID pour charger l'article ensuite.
// =====================================================

bool encSearchPrefix() {
  String url =
    "https://fr.wikipedia.org/w/api.php?"
    "action=query"
    "&list=prefixsearch"
    "&format=json"
    "&utf8=1"
    "&pslimit=5"
    "&pssearch=" + encUrlEncode(encQuery);

  String payload;
  if (!encHttpGet(url, payload)) return false;

  DynamicJsonDocument doc(14000);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("ENC prefix JSON erreur : ");
    Serial.println(err.c_str());
    return false;
  }

  JsonArray arr = doc["query"]["prefixsearch"].as<JsonArray>();

  for (JsonObject item : arr) {
    if (encResultCount >= ENC_MAX_RESULTS) break;

    String title = item["title"].as<String>();
    int pageid = item["pageid"].as<int>();

    title.trim();

    if (title.length() > 0 && pageid > 0) {
      encResults[encResultCount] = title;
      encResultPageIds[encResultCount] = pageid;
      encResultCount++;
    }
  }

  return encResultCount > 0;
}

bool encSearchClassic() {
  String url =
    "https://fr.wikipedia.org/w/api.php?"
    "action=query"
    "&list=search"
    "&format=json"
    "&utf8=1"
    "&srlimit=5"
    "&srprop="
    "&srsearch=" + encUrlEncode(encQuery);

  String payload;
  if (!encHttpGet(url, payload)) return false;

  DynamicJsonDocument doc(16000);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("ENC classic JSON erreur : ");
    Serial.println(err.c_str());
    return false;
  }

  JsonArray arr = doc["query"]["search"].as<JsonArray>();

  for (JsonObject item : arr) {
    if (encResultCount >= ENC_MAX_RESULTS) break;

    String title = item["title"].as<String>();
    int pageid = item["pageid"].as<int>();

    title.trim();

    if (title.length() > 0 && pageid > 0) {
      encResults[encResultCount] = title;
      encResultPageIds[encResultCount] = pageid;
      encResultCount++;
    }
  }

  return encResultCount > 0;
}

bool encSearchWiki() {
  encResultCount = 0;
  encQuery.trim();

  for (int i = 0; i < ENC_MAX_RESULTS; i++) {
    encResults[i] = "";
    encResultPageIds[i] = 0;
  }

  if (encQuery.length() == 0) return false;

  bool ok = encSearchPrefix();

  if (!ok || encResultCount == 0) {
    ok = encSearchClassic();
  }

  Serial.print("ENC resultats = ");
  Serial.println(encResultCount);

  return encResultCount > 0;
}

// =====================================================
// ARTICLE PAR PAGEID : beaucoup plus fiable que le titre
// =====================================================

bool encLoadArticleByPageId(int pageid, String fallbackTitle) {
  encArticleTitle = fallbackTitle;
  encArticleText = "";
  encPage = 0;

  if (pageid <= 0) return false;

  String url =
    "https://fr.wikipedia.org/w/api.php?"
    "action=query"
    "&prop=extracts"
    "&explaintext=1"
    "&exsectionformat=plain"
    "&exchars=4200"
    "&format=json"
    "&utf8=1"
    "&pageids=" + String(pageid);

  String payload;
  if (!encHttpGet(url, payload)) return false;

  DynamicJsonDocument doc(24000);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("ENC article JSON erreur : ");
    Serial.println(err.c_str());
    return false;
  }

  JsonObject pages = doc["query"]["pages"].as<JsonObject>();

  for (JsonPair kv : pages) {
    String title = kv.value()["title"].as<String>();
    String extract = kv.value()["extract"].as<String>();

    if (title.length() > 0) encArticleTitle = title;

    if (extract.length() == 0) {
      extract = "Aucun texte disponible pour cet article.";
    }

    if (extract.length() > ENC_TEXT_MAX) {
      extract = extract.substring(0, ENC_TEXT_MAX);
    }

    encArticleText = teClean(extract);
    return true;
  }

  return false;
}

bool encLoadArticle(String title) {
  // Gardé pour compatibilité, mais on préfère encLoadArticleByPageId().
  encArticleTitle = title;
  encArticleText = "";
  encPage = 0;

  String url =
    "https://fr.wikipedia.org/w/api.php?"
    "action=query"
    "&prop=extracts"
    "&explaintext=1"
    "&exsectionformat=plain"
    "&exchars=4200"
    "&format=json"
    "&utf8=1"
    "&redirects=1"
    "&titles=" + encUrlEncode(title);

  String payload;
  if (!encHttpGet(url, payload)) return false;

  DynamicJsonDocument doc(24000);
  DeserializationError err = deserializeJson(doc, payload);

  if (err) return false;

  JsonObject pages = doc["query"]["pages"].as<JsonObject>();

  for (JsonPair kv : pages) {
    String extract = kv.value()["extract"].as<String>();

    if (extract.length() > ENC_TEXT_MAX) {
      extract = extract.substring(0, ENC_TEXT_MAX);
    }

    encArticleText = teClean(extract);
    return encArticleText.length() > 0;
  }

  return false;
}