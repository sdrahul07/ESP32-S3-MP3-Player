#include "Themes.h"
#include <math.h>
#include <algorithm>
#include "StrandSmall.inc"
#include "StrandBody.inc"
#include "StrandTitle.inc"
#include "StrandClock.inc"
#include "ThemeArt.h"
#include "PixelFont.h"
const char* const playerThemeNames[PLAYER_THEME_COUNT]={"Neon Fishbone","Sakura Tape","Sunvair Flight","NASA Worm","Card Battle","Lunar Courier","Tidal Station","Amber Workshop","Iron Circuit","Melody Machine","Pocket OS","Dungeon Deck","Amber Navigator","Signal Report / Black","Tape Session","Signal Report / Light","Arc Transport","Chrono Deck","Phosphor Deck","Retro Desktop"};
static uint16_t rgb(unsigned r,unsigned g,unsigned b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
ThemePalette playerPalette(uint8_t t){static const ThemePalette palettes[PLAYER_THEME_COUNT]={
{0x053e,0x1149,0xf0f0,0x19ab,0x665d},
{0x45ac,0x3104,0xd12d,0x4987,0x2c4a},
{0xf799,0x31a4,0x43f8,0x63f2,0xcebb},
{0xf799,0x31a4,0xd987,0x8b6d,0xe659},
{0xf799,0x31a4,0x24ab,0x634a,0xce95},
{0xf799,0x31a4,0x9b57,0x83b1,0xd67c},
{0xf799,0x31a4,0x1472,0x53ef,0xbeda},
{0xf799,0x31a4,0xcb85,0x8b89,0xe654},
{0x0881,0xdef9,0xc1e6,0xa572,0x3206},
{0x5a4c,0xff9a,0xfdea,0xd5bb,0x9c18},
{0x3bd9,0xff9a,0x86d6,0xd73d,0x2ad3},
{0x10a3,0xf77b,0xf77b,0xc637,0x31a7},
{0x1061,0xf441,0xf441,0xcc85,0x3922},
{0x0000,0xef9d,0x66bc,0x94d3,0x1905},
{0x0000,0xef9d,0xec07,0x94d3,0x1905},
{0xef7e,0x19a8,0x0bf2,0x5b8f,0xd71c},
{0x0000,0xef9d,0xedc5,0x94d3,0x1905},
{0x0000,0xef9d,0xfb02,0x94d3,0x1905},
{0x0000,0xeffe,0x57f5,0x9553,0x1123},
{0xef19,0x2986,0x2986,0x5ac9,0xd676},
};return palettes[t<PLAYER_THEME_COUNT?t:0];}
static uint16_t blend(uint16_t fg,uint16_t bg,unsigned a){unsigned z=15-a;return uint16_t(((((fg>>11)*a+(bg>>11)*z+7)/15)<<11)|(((((fg>>5)&63)*a+((bg>>5)&63)*z+7)/15)<<5)|(((fg&31)*a+(bg&31)*z+7)/15));}
static uint16_t nextCode(const char*&s){uint8_t a=uint8_t(*s++);if(a<128)return a;if(a>=0xC2&&a<=0xDF&&(*s&0xC0)==0x80){uint16_t cp=((a&31)<<6)|(uint8_t(*s++)&63);return cp<256?cp:'?';}if(a>=0xE0&&a<=0xEF&&s[0]&&s[1]&&(s[0]&0xC0)==0x80&&(s[1]&0xC0)==0x80){uint16_t cp=((a&15)<<12)|((uint8_t(s[0])&63)<<6)|(uint8_t(s[1])&63);s+=2;if(cp==0x2018||cp==0x2019)return '\'';if(cp==0x2013||cp==0x2014)return '-';return '?';}while((*s&0xC0)==0x80)++s;return '?';}
static StrandGlyph glyph(const StrandFont&f,uint16_t cp){StrandGlyph g;cp=cp>=32&&cp<256?cp:'?';memcpy_P(&g,f.glyphs+cp-32,sizeof(g));return g;}
static int measure(const String&s,const StrandFont&f){const char*p=s.c_str();int w=0;while(*p)w+=glyph(f,nextCode(p)).advance;return w;}
static void rasterText(GFXcanvas16&canvas,int x,int baseline,const String&s,int clipLeft,int clipRight,uint16_t fg,uint16_t bg,const StrandFont&font){
  uint16_t shades[16];for(unsigned a=0;a<16;a++)shades[a]=blend(fg,bg,a);
  uint16_t*pixels=canvas.getBuffer();int W=canvas.width(),H=canvas.height();const char*cursor=s.c_str();
  while(*cursor){StrandGlyph g=glyph(font,nextCode(cursor));if(x+g.x>=clipRight)break;
    if(x+g.x+g.width>clipLeft)for(unsigned j=0;j<g.height;j++){int yy=baseline+g.y+j;if(yy<0||yy>=H)continue;for(unsigned i=0;i<g.width;i++){int xx=x+g.x+i;if(xx<clipLeft||xx>=clipRight||xx<0||xx>=W)continue;unsigned n=j*g.width+i;uint8_t bits=pgm_read_byte(font.bitmap+g.offset+n/2);unsigned a=n%2?bits&15:bits>>4;if(a)pixels[yy*W+xx]=shades[a];}}
    x+=g.advance;
  }
}
void strandText(GFXcanvas16&canvas,int x,int baseline,const String&s,int width,uint16_t fg,uint16_t bg,const StrandFont&font){
  if(width<=0)return;bool cut=measure(s,font)>width;if(!cut){rasterText(canvas,x,baseline,s,x,x+width,fg,bg,font);return;}
  int end=x+width-measure("...",font),right=x;const char*cursor=s.c_str();while(*cursor){const char*start=cursor;StrandGlyph g=glyph(font,nextCode(cursor));if(right+g.advance>end){cursor=start;break;}right+=g.advance;}
  String clipped;const char*start=s.c_str();while(start<cursor)clipped+=*start++;
  rasterText(canvas,x,baseline,clipped,x,end,fg,bg,font);rasterText(canvas,right,baseline,"...",right,x+width,fg,bg,font);
}
namespace {
GFXcanvas16*g;ThemePalette p;const ThemeFrame*f;uint32_t scrollStarted=0;
int lastTrack=-999,lastTheme=-1;uint32_t lastSample=0,sampleAt=0,seekAt=0;float dialSeconds=0,seekFrom=0,seekTo=0;bool seeking=false;
String clockText(uint32_t s){char b[24];snprintf(b,sizeof(b),"%lu:%02lu",(unsigned long)(s/60),(unsigned long)(s%60));return b;}
void text(int x,int y,const String&s,int width,uint16_t fg,const StrandFont&font=StrandSmall,uint16_t bg=0){
 int overflow=measure(s,font)-width;if(f&&(s==f->title||s==f->artist||s==f->album)&&overflow>0){
   const float speed=23.f;float travel=overflow*1000.f/speed;float phase=fmodf(float(millis()-scrollStarted),2*travel+2400.f),offset=0;
   if(phase>1200.f&&phase<1200.f+travel)offset=(phase-1200.f)*speed/1000.f;
   else if(phase>=1200.f+travel&&phase<2400.f+travel)offset=overflow;
   else if(phase>=2400.f+travel)offset=overflow-(phase-2400.f-travel)*speed/1000.f;
   rasterText(*g,x-int(offset),y,s,x,x+width,fg,bg,font);
 }else strandText(*g,x,y,s,width,fg,bg,font);
}
void line(int x,int y,int w,uint16_t c){g->drawFastHLine(x,y,w,c);}
String position(){return String(f->queuePos<0?0:f->queuePos+1)+" / "+String(f->queueCount);}
String format(){return f->rate?String("MP3 / ")+String(f->rate/1000.0f,1)+"kHz":"MP3 / SD CARD";}
void bar(int x,int y,int w,int h,uint16_t col){g->fillRect(x,y,w,h,p.panel);if(f->duration)g->fillRect(x,y,std::min<uint64_t>(w,uint64_t(f->elapsed)*w/f->duration),h,col);}
void title(int x,int baseline,int w){text(x,baseline,f->title,w,p.fg,StrandTitle);}
void cornerPanel(int x,int y,int w,int h,uint16_t col){g->drawLine(x,y+4,x+4,y,col);line(x+4,y,w-5,col);g->drawFastVLine(x+w-1,y,h-5,col);g->drawLine(x+w-1,y+h-5,x+w-5,y+h-1,col);line(x,y+h-1,w-5,col);g->drawFastVLine(x,y+4,h-4,col);}
void scope(int,int,int,int,int,const int8_t*,const int8_t*,uint16_t,bool vertical=false);
#include "SelectedThemes.inc"
// Actual signed PCM min/max bins, captured before playback volume scaling.
void scope(int x,int y,int count,int stride,int height,const int8_t*low,const int8_t*high,uint16_t col,bool vertical){
 int prevX=0,prevY=0;uint16_t envelope=blend(col,p.panel,7);
 for(int i=0;i<count;i++){float at=i+historyOffset();int a=int(roundf(sampleBin(low,count,at)*height/128)),b=int(roundf(sampleBin(high,count,at)*height/128)),mid=(a+b)/2;
  int xx=vertical?x+mid:x+i*stride,yy=vertical?y+i*stride:y+mid;
  if(vertical)g->drawFastHLine(x+a,y+i*stride,std::max(1,b-a+1),envelope);
  else g->drawFastVLine(x+i*stride,y+a,std::max(1,b-a+1),envelope);
  if(i)g->drawLine(prevX,prevY,xx,yy,col);prevX=xx;prevY=yy;
 }
}
void updateMotion(uint8_t theme){uint32_t now=millis();if(lastTrack!=f->trackId||lastTheme!=theme){scrollStarted=now;lastTrack=f->trackId;lastTheme=theme;dialSeconds=f->elapsed;lastSample=f->elapsed;sampleAt=now;seeking=false;}
 if(f->elapsed!=lastSample){if(llabs(int64_t(f->elapsed)-lastSample)>2){seekFrom=dialSeconds;seekTo=f->elapsed;seekAt=now;seeking=true;}lastSample=f->elapsed;sampleAt=now;}
 if(seeking){float u=std::min(1.f,(now-seekAt)/550.f);dialSeconds=seekFrom+(seekTo-seekFrom)*(1-powf(1-u,3));if(u>=1)seeking=false;}
 else if(f->playing)dialSeconds=f->elapsed+std::min(.95f,(now-sampleAt)/1000.f);
}
void report(){uint16_t yellow=rgb(225,234,60);g->fillRect(8,7,89,18,yellow);text(13,21,"STRAND / 01",83,0,StrandSmall,yellow);text(109,19,"LOCAL AUDIO",130,p.fg);text(252,19,position(),63,p.dim);line(12,31,296,rgb(82,100,110));text(13,46,"NOW PLAYING",220,p.dim);title(12,79,296);text(13,99,f->artist,295,p.accent,StrandBody);text(13,113,f->album,295,p.dim);g->fillRect(13,123,107,13,rgb(29,48,56));text(18,132,"PCM / L+R",99,p.fg,StrandSmall,rgb(29,48,56));cornerPanel(12,136,296,40,rgb(113,138,151));for(int x=27;x<298;x+=17)g->drawFastVLine(x,140,30,rgb(20,34,41));text(16,150,"L",9,p.dim);text(16,168,"R",9,p.dim);scope(28,148,92,3,7,f->scope.lowL,f->scope.highL,p.accent);scope(28,165,92,3,7,f->scope.lowR,f->scope.highR,rgb(186,142,240));text(12,199,clockText(f->elapsed),100,p.fg,StrandBody);text(89,198,"/ "+(f->duration?clockText(f->duration):"--:--"),100,p.dim);text(224,197,f->rate?String(f->rate/1000.0f,1)+"kHz":"SD",85,p.dim);bar(12,207,296,2,p.accent);g->fillRect(0,218,320,22,rgb(10,20,27));g->fillRect(12,223,61,13,yellow);text(18,232,f->playing?"PLAYING":"PAUSED",55,0,StrandSmall,yellow);text(96,232,"VOL / "+String(f->volume),115,p.fg);text(233,232,"HOLD:MENU",83,p.dim);}
void tape(){text(12,23,"STRAND / SESSION",234,p.fg,StrandBody);text(268,25,String(f->queuePos+1),42,p.accent,StrandBody);line(12,33,296,p.dim);text(12,50,"NOW PLAYING",196,p.dim);title(11,86,203);text(13,109,f->artist,200,p.fg,StrandBody);text(13,126,f->album,200,p.dim);text(13,148,format(),200,p.dim);text(13,171,"VOL "+String(f->volume),200,p.fg);g->fillRect(222,43,86,137,rgb(7,14,18));for(int y=52;y<175;y+=12)line(226,y,78,rgb(28,43,51));g->drawFastVLine(246,53,115,rgb(65,83,92));g->drawFastVLine(283,53,115,rgb(65,83,92));scope(246,57,57,2,13,f->scope.lowL+35,f->scope.highL+35,p.fg,true);scope(283,57,57,2,13,f->scope.lowR+35,f->scope.highR+35,p.accent,true);text(241,51,"L",10,p.fg);text(279,51,"R",10,p.accent);line(12,185,296,p.dim);text(11,215,clockText(f->elapsed),120,p.fg,StrandTitle);text(98,212,"/ "+(f->duration?clockText(f->duration):"--:--"),104,p.dim);g->fillRect(218,197,90,21,p.accent);text(233,211,f->playing?"PLAYING":"PAUSED",72,0,StrandBody,p.accent);bar(12,228,296,3,p.accent);}
void arc(){text(14,20,"LOCAL / MP3",200,p.dim);text(254,20,position(),64,p.dim);const float cx=-106,cy=127,base=185;int first=std::max(0,int((dialSeconds-51)/2));int last=int((dialSeconds+51)/2);for(int j=first;j<=last;j++){float v=j*2,a=(v-dialSeconds)*.0165f;if(a<-.83f||a>.83f)continue;bool major=j%5==0,near=fabsf(v-dialSeconds)<3;float rr=base-(major?22:10);uint16_t col=near?rgb(212,221,218):rgb(37,44,46);g->drawLine(cx+cosf(a)*rr,cy+sinf(a)*rr,cx+cosf(a)*base,cy+sinf(a)*base,col);if(major&&a>-.48f&&a<.48f)text(cx+cosf(a)*(rr-29)-8,cy+sinf(a)*(rr-29)+3,clockText(uint32_t(v)),32,near?p.fg:p.dim);}
 line(58,126,31,p.accent);line(95,126,4,p.accent);text(111,70,f->title,196,p.fg,StrandBody);text(112,89,f->artist,194,p.dim);g->fillRoundRect(105,104,202,61,30,rgb(23,27,29));text(119,153,clockText(f->elapsed),180,p.fg,StrandClock,rgb(23,27,29));text(112,182,"ELAPSED",110,p.dim);text(257,182,"/ "+(f->duration?clockText(f->duration):"--:--"),60,p.dim);text(111,213,f->playing?"PLAYING":"PAUSED",130,p.accent,StrandBody);text(259,213,"VOL "+String(f->volume),59,p.dim);}
void chrono(){text(15,27,f->title,292,p.fg,StrandBody);text(16,46,f->artist,290,p.dim);text(12,124,clockText(f->elapsed),220,p.fg,StrandClock);text(236,111,"/ "+(f->duration?clockText(f->duration):"--:--"),80,p.dim,StrandBody);text(16,140,"PLAYBACK",200,p.dim);g->fillRoundRect(14,150,292,48,10,p.accent);text(25,165,"0:00",90,0,StrandSmall,p.accent);String total=f->duration?clockText(f->duration):"--:--";text(269,165,total,34,0,StrandSmall,p.accent);for(int j=0;j<=64;j++)g->drawFastVLine(25+j*4.2f,175,j%8==0?9:4,rgb(132,51,11));int progress=f->duration?std::min<uint64_t>(269,uint64_t(f->elapsed)*269/f->duration):0;g->fillRect(25,177,progress,3,0);g->fillRect(24+progress,172,2,13,0);text(16,225,f->playing?"PLAYING":"PAUSED",125,p.fg);text(150,225,"VOL "+String(f->volume),95,p.dim);text(257,225,position(),61,p.dim);}
void redline(){uint16_t red=p.accent,yellow=rgb(237,204,53),dark=rgb(8,8,9);text(13,20,"LOCAL AUDIO / MP3",240,p.dim);text(260,20,position(),57,p.dim);g->fillRect(13,35,18,2,red);text(43,39,"PLAYBACK SYSTEM",220,red);g->fillRect(289,35,18,2,red);
 // All adjacent red pieces have straight edges and exactly 4px gutters.
 g->fillRect(13,56,224,46,red);text(25,90,f->title,200,dark,StrandTitle,red);g->fillRect(241,56,66,46,red);text(250,85,"A"+String(f->queuePos<0?0:f->queuePos+1),51,dark,StrandBody,red);
 uint16_t bgs[3]={red,rgb(204,55,72),rgb(168,46,62)};String values[3]={f->artist,f->album,format()};for(int i=0;i<3;i++){int y=106+i*26;g->fillRect(13,y,224,22,bgs[i]);text(21,y+16,values[i],207,i==2?p.fg:dark,i==2?StrandSmall:StrandBody,bgs[i]);}
 g->fillRect(241,106,66,74,red);g->fillTriangle(274,111,245,166,303,166,dark);g->fillTriangle(274,121,254,161,294,161,yellow);bool show=!f->playing||millis()%1400<700;if(show){g->fillRect(272,133,4,15,dark);g->fillRect(272,153,4,4,dark);}text(246,175,f->playing?"ACTIVE":"PAUSED",58,dark,StrandSmall,red);
 text(13,204,clockText(f->elapsed),90,p.fg,StrandBody);text(89,204,"/ "+(f->duration?clockText(f->duration):"--:--"),100,p.dim);text(244,204,"VOL "+String(f->volume),64,p.dim);bar(13,211,294,2,red);text(13,230,f->playing?"PLAYING":"PAUSED",150,red);text(238,230,"HOLD:MENU",80,p.dim);
}
#include "ThemeMenus.inc"
}
void drawPlayerTheme(GFXcanvas16&canvas,uint8_t theme,const ThemeFrame&frame){g=&canvas;f=&frame;theme=theme<PLAYER_THEME_COUNT?theme:0;p=playerPalette(theme);g->fillScreen(p.bg);if(menuTheme!=255){scrollStarted=millis();menuTheme=255;}updateMotion(theme);signalMotion();if(theme<9){artTheme(theme);return;}switch(theme){case 9:melody();break;case 10:pocketOs();break;case 11:dungeon();break;case 12:navigator();break;case 13:report();break;case 14:tape();break;case 15:lightReport();break;case 16:arc();break;case 17:chrono();break;case 18:phosphor();break;case 19:desktop();break;}}

int themeMenuRows(uint8_t theme){return theme==11?3:theme<10||theme==12||theme==16||theme==17?4:5;}
void drawThemeMenu(GFXcanvas16&canvas,uint8_t theme,const ThemeFrame&now,const ThemeMenuFrame&menu){
 theme=theme<PLAYER_THEME_COUNT?theme:0;g=&canvas;f=&now;p=playerPalette(theme);updateMotion(theme);signalMotion();menuScroll(menu,theme);g->fillScreen(p.bg);
 if(theme<9)artworkMenu(theme,menu);
 else switch(theme){case 9:melodyMenu(menu);break;case 10:pocketMenu(menu);break;case 11:dungeonMenu(menu);break;case 12:navigatorMenu(menu);break;case 19:retroMenu(menu);break;default:technicalMenu(theme,menu);break;}
}
void drawThemeBrightness(GFXcanvas16&canvas,uint8_t theme,const ThemeFrame&now,int value,bool available){
 theme=theme<PLAYER_THEME_COUNT?theme:0;g=&canvas;f=&now;p=playerPalette(theme);g->fillScreen(p.bg);menuTheme=theme;menuSelected=-2;menuBrightness(theme,std::max(5,std::min(100,value)),available);
}
void drawThemeToast(GFXcanvas16&canvas,uint8_t theme,const String&message){
 g=&canvas;p=playerPalette(theme);g->fillRect(9,187,302,29,p.panel);box(9,187,302,29,p.dim);
 if(theme<=12||theme==19)pixelText(18,195,message,p.fg,2,284);
 else strandText(canvas,18,207,message,284,p.fg,p.panel,StrandBody);
}
