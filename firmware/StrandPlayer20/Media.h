#pragma once
#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
extern SemaphoreHandle_t spiMutex;
struct SpiLock {
  SpiLock(){xSemaphoreTake(spiMutex,portMAX_DELAY);}
  ~SpiLock(){xSemaphoreGive(spiMutex);}
};
struct Track {
  String path,title,artist,album;
  uint32_t artOffset=0,artBytes=0;
};
extern std::vector<Track> tracks;
extern std::vector<String> playlists;
void scanMusic(); // Startup only, before other tasks use SPI.
bool readPlaylist(const String &path,std::vector<int> &out);
String savePlaylist(const std::vector<int> &items);
