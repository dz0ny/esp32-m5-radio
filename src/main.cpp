#include <M5Stack.h>

#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorAAC.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioOutputI2SNoDAC.h>

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <WiFi.h>

typedef enum Codec {
  MP3,
  AAC,
} Codec;

typedef struct Station {
  String url;
  Codec codec;
  String name;
  int32_t fontColor;
  int32_t bgColor;
} Station;

bool isPlaying = false;
int activeStation = 0;

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server, &dns);

AudioGeneratorAAC *aac;
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

Station stations[] = {
    Station{"http://mp3.rtvslo.si/val202", Codec::AAC, "Val202", TFT_WHITE,
            TFT_RED},
    Station{"http://mp3.rtvslo.si/prvi", Codec::AAC, "Prvi", TFT_WHITE,
            TFT_BLUE},
    Station{"http://sl32.hnux.com/stream?type=http&nocache=1257", Codec::MP3, "Lounge",
            TFT_WHITE, TFT_GREEN},
    Station{"http://188.165.212.154:8478/stream", Codec::MP3, "Romatic",
            TFT_WHITE, TFT_PINK},
    Station{"http://live.radioterminal.si/live", Codec::MP3, "Terminal",
            TFT_BLACK, TFT_YELLOW},
};
#define numStations (sizeof(stations) / sizeof(stations[0])) // array size

void update() {
  M5.Lcd.setBrightness(1);
  M5.Lcd.setTextSize(6);
  M5.Lcd.setTextColor(stations[activeStation].fontColor);
  M5.Lcd.fillScreen(stations[activeStation].bgColor);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.print(stations[activeStation].name.c_str());
}

void play() {

  file = new AudioFileSourceHTTPStream(stations[activeStation].url.c_str());
  buff = new AudioFileSourceBuffer(file, 2048);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);

  switch (stations[activeStation].codec) {
  case Codec::AAC:
    aac = new AudioGeneratorAAC();
    aac->begin(buff, out);
    break;
  case Codec::MP3:
    mp3 = new AudioGeneratorMP3();
    mp3->begin(buff, out);
    break;
  }
}

void stop() {

  if (aac) {
    aac->stop();
    delete aac;
    aac = NULL;
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = NULL;
  }
  if (file) {
    file->close();
    delete file;
    file = NULL;
  }
}

void CriticalLoop() {
  M5.update();
  if (aac && aac->isRunning() && !aac->loop()) {
    stop();
  }
  if (mp3 && mp3->isRunning() && !mp3->loop()) {
    stop();
  }
}

void setup() {
  M5.begin();
  wifiManager.autoConnect("Radio");
  update();
}

void loop() {
  CriticalLoop();
  if (M5.BtnA.wasPressed()) {
    int prev = activeStation - 1;
    if (prev == -1) {
      activeStation = numStations-1;
    } else {
      activeStation = prev;
    }
    update();
  }

  if (M5.BtnC.wasPressed()) {
    int next = activeStation + 1;
    if (next >= numStations) {
      activeStation = 0;
    } else {
      activeStation = next;
    }
    update();
  }

  if (M5.BtnB.wasPressed()) {
    if ((aac && aac->isRunning()) || (mp3 && mp3->isRunning())) {
      stop();
    } else {
      play();
    }
  }
}
