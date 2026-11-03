#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "PcmScope.h"
constexpr uint8_t PLAYER_THEME_COUNT=20;
extern const char* const playerThemeNames[PLAYER_THEME_COUNT];
struct ThemePalette {uint16_t bg,fg,accent,dim,panel;};
ThemePalette playerPalette(uint8_t theme);
struct ThemeFrame {
  String title,artist,album;
  int trackId=-1;
  uint32_t elapsed=0,duration=0,rate=0;
  int volume=0,queuePos=0,queueCount=0;
  uint8_t left=0,right=0;
  PcmScope scope;
  bool playing=false;
};
// UI task only. Does not access SD, I2S, playback commands or Preferences.
void drawPlayerTheme(GFXcanvas16&canvas,uint8_t theme,const ThemeFrame&frame);

enum class ThemeMenuKind:uint8_t{Library,List,Settings,Appearance,Themes,Display,Actions,Calibration};
struct ThemeMenuFrame {
 String heading,emptyMessage;
 const String*items=nullptr;
 String secondary[5]; // Optional metadata for visible track rows only.
 int count=0,selected=0,top=0,playingIndex=-1,appliedIndex=-1;
 ThemeMenuKind kind=ThemeMenuKind::Library;
};
int themeMenuRows(uint8_t theme);
void drawThemeMenu(GFXcanvas16&,uint8_t,const ThemeFrame&,const ThemeMenuFrame&);
void drawThemeBrightness(GFXcanvas16&,uint8_t,const ThemeFrame&,int value,bool available);
void drawThemeToast(GFXcanvas16&,uint8_t,const String&);

#include "StrandFonts.h"
void strandText(GFXcanvas16&,int x,int baseline,const String&,int width,uint16_t fg,uint16_t bg,const StrandFont&);
