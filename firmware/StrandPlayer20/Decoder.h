#pragma once
#include "Media.h"
#define MINIMP3_NO_STDIO
#define MINIMP3_ONLY_MP3
#include "vendor/minimp3_ex.h"
class Decoder {
public:
  File file;
  mp3dec_ex_t* mp3=nullptr;
  mp3dec_io_t io={};
  uint32_t rate=0;uint8_t channels=0,bits=0;
  uint64_t frames=0,position=0;
  bool open(const String&path);
  size_t read(int32_t*stereo,size_t count);
  bool seek(uint64_t frame);
  ~Decoder();
};
