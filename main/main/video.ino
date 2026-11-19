#include "video.h"
#include "menu_core.h"
#include "audio_system.h"
#include "audio_wav_player.h"

#define VIDEO_DIR "/video/demo"
#define VIDEO_AUDIO_PATH "/video/demo/audio.wav"

#define VIDEO_FPS 6
#define VIDEO_FRAME_MS (1000 / VIDEO_FPS)

#define VIDEO_SD_CS   5
#define VIDEO_SD_SCK  18
#define VIDEO_SD_MISO 19
#define VIDEO_SD_MOSI 23

SPIClass videoSdSPI = SPIClass(HSPI);

bool videoPlaying = false;
bool videoPaused = false;

int videoFrame = 0;

unsigned long videoLastFrame = 0;
unsigned long videoLastTouch = 0;

WavPlayer videoWav;

// =====================================================
// SD
// =====================================================

bool videoStartSD() {
  SD.end();
  delay(20);

  videoSdSPI.end();
  delay(20);

  videoSdSPI.begin(VIDEO_SD_SCK, VIDEO_SD_MISO, VIDEO_SD_MOSI, VIDEO_SD_CS);

  if (!SD.begin(VIDEO_SD_CS, videoSdSPI, 10000000)) {
    Serial.println("VIDEO SD ECHEC");
    return false;
  }

  Serial.println("VIDEO SD OK");
  return true;
}

// =====================================================
// VIDEO JPG
// =====================================================

String videoFramePath(int n) {
  char buf[60];
  sprintf(buf, "%s/%04d.jpg", VIDEO_DIR, n);
  return String(buf);
}

bool videoFrameExists(int n) {
  return SD.exists(videoFramePath(n));
}

void videoDrawFrame() {
  String path = videoFramePath(videoFrame);

  if (!SD.exists(path)) {
    videoPlaying = false;
    videoPaused = false;
    wavPlayerStop(videoWav);

    tft.fillRoundRect(52, 90, 216, 64, 10, TFT_BLACK);
    tft.drawRoundRect(52, 90, 216, 64, 10, TFT_GREEN);

    drawBoldText("FIN VIDEO", 160, 112, TFT_GREEN, TFT_BLACK, MC_DATUM, 2);
    drawBoldText("PLAY pour relancer", 160, 138, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);

    return;
  }

  tft.fillRect(40, 34, 240, 180, TFT_BLACK);
  TJpgDec.drawSdJpg(40, 34, path.c_str());

  videoFrame++;
}

// =====================================================
// UI
// =====================================================

void videoHeader() {
  tft.fillRect(0, 0, 320, 34, TFT_BLACK);
  tft.fillTriangle(11, 17, 31, 6, 31, 28, TFT_WHITE);

  drawBoldText("Video", 160, 17, TFT_WHITE, TFT_BLACK, MC_DATUM, 2);
}

void videoButton(int x, int y, int w, int h, String txt, uint16_t bg) {
  tft.fillRoundRect(x, y, w, h, 8, bg);
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);

  drawBoldText(
    txt,
    x + w / 2,
    y + h / 2,
    TFT_BLACK,
    bg,
    MC_DATUM,
    1
  );
}

void videoDrawUI() {
  tft.fillScreen(TFT_BLACK);
  videoHeader();

  tft.drawRoundRect(38, 32, 244, 184, 8, TFT_CYAN);

  String playTxt = "PLAY";
  uint16_t playCol = TFT_GREEN;

  if (videoPlaying && !videoPaused) {
    playTxt = "PAUSE";
    playCol = TFT_ORANGE;
  }

  if (videoPaused) {
    playTxt = "REPLAY";
    playCol = TFT_GREEN;
  }

  videoButton(70, 218, 80, 20, playTxt, playCol);
  videoButton(170, 218, 80, 20, "STOP", TFT_RED);

  drawBoldText(
    "JPG + WAV",
    160,
    205,
    TFT_LIGHTGREY,
    TFT_BLACK,
    MC_DATUM,
    1
  );
}

void videoDrawNoFiles() {
  tft.fillScreen(TFT_BLACK);
  videoHeader();

  drawBoldText("Aucune video", 160, 95, TFT_RED, TFT_BLACK, MC_DATUM, 2);
  drawBoldText("/video/demo/0000.jpg", 160, 125, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
  drawBoldText("/video/demo/audio.wav", 160, 145, TFT_WHITE, TFT_BLACK, MC_DATUM, 1);
}

// =====================================================
// ACTIONS
// =====================================================

void videoStop() {
  videoPlaying = false;
  videoPaused = false;
  videoFrame = 0;

  wavPlayerStop(videoWav);

  videoDrawUI();
}

void videoPlay() {
  if (!videoStartSD()) {
    videoDrawUI();
    drawBoldText("Erreur SD", 160, 120, TFT_RED, TFT_BLACK, MC_DATUM, 2);
    stopSDStartTouch();
    return;
  }

  if (!videoFrameExists(0)) {
    videoDrawNoFiles();
    stopSDStartTouch();
    return;
  }

  videoFrame = 0;
  videoPlaying = true;
  videoPaused = false;
  videoLastFrame = millis();

  videoDrawUI();

  if (SD.exists(VIDEO_AUDIO_PATH)) {
    wavPlayerOpen(videoWav, VIDEO_AUDIO_PATH, 80);
  } else {
    Serial.println("VIDEO : audio.wav introuvable");
  }

  videoDrawFrame();
}

void videoPause() {
  videoPlaying = false;
  videoPaused = true;

  wavPlayerStop(videoWav);

  videoDrawUI();

  if (videoFrame > 0) {
    String lastFrame = videoFramePath(videoFrame - 1);
    if (SD.exists(lastFrame)) {
      TJpgDec.drawSdJpg(40, 34, lastFrame.c_str());
    }
  }
}

void videoResume() {
  videoPlay();
}

void videoExit() {
  videoPlaying = false;
  videoPaused = false;

  wavPlayerStop(videoWav);

  SD.end();
  delay(20);

  stopSDStartTouch();
  drawSubMenu(currentMenu);
}

// =====================================================
// LANCEMENT
// =====================================================

void launchVideo() {
  appMode = MODE_VIDEO;

  stopSDStartTouch();
  audioSystemStop();

  wavPlayerInit(videoWav);

  videoPlaying = false;
  videoPaused = false;
  videoFrame = 0;

  videoStartSD();
  videoDrawUI();

  Serial.println("VIDEO PLAYER COMMUN PRET");
}

// =====================================================
// UPDATE
// =====================================================

void updateVideo() {
  if (videoPlaying) {
    wavPlayerUpdate(videoWav);

    if (millis() - videoLastFrame >= VIDEO_FRAME_MS) {
      videoLastFrame = millis();
      videoDrawFrame();
    }
  }

  int x, y;

  if (!getTouch(x, y)) return;

  if (millis() - videoLastTouch < 180) return;
  videoLastTouch = millis();

  if (y < 34 && x < 45) {
    videoExit();

    while (ts.touched()) delay(5);
    delay(80);
    return;
  }

  // PLAY / PAUSE / REPLAY
  if (x >= 70 && x <= 150 && y >= 218 && y <= 238) {
    if (videoPlaying) {
      videoPause();
    } else {
      videoPlay();
    }
  }

  // STOP
  else if (x >= 170 && x <= 250 && y >= 218 && y <= 238) {
    videoStop();
  }

  while (ts.touched()) delay(5);
  delay(80);
}