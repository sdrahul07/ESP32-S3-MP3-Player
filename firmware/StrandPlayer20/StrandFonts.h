#pragma once
#include <Arduino.h>
struct StrandGlyph {uint32_t offset;uint8_t width,height,advance;int8_t x,y;};
struct StrandFont {const uint8_t*bitmap;const StrandGlyph*glyphs;uint8_t lineHeight;};
extern const StrandFont StrandSmall;
extern const StrandFont StrandBody;
extern const StrandFont StrandTitle;
extern const StrandFont StrandClock;
