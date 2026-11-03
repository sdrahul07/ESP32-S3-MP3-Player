#pragma once
#include <Arduino.h>
namespace Pins {
constexpr int TFT_CS=10,TFT_DC=9,TFT_RST=14,BL=21;
constexpr int SD_CS=15,SCK=12,MOSI=11,MISO=13;
constexpr int BCK=4,WS=5,DOUT=6; // PCM5102A; GPIO7/8/16 no longer used.
constexpr int ENC_A=17,ENC_B=18;
constexpr int CENTER=1,DOWN=2,RIGHT=38,UP=39,LEFT=40;
}
constexpr uint32_t SD_SPEED=16000000; // Reduce to 8000000 if breadboard reads fail.
constexpr uint32_t TFT_SPEED=16000000;
constexpr size_t MAX_TRACKS=4096,MAX_ART_BYTES=1024*1024;
constexpr int AUDIO_FRAMES=1024;

