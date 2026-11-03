// Included inside the renderer namespace. UI-only; no audio/SD/heap operations.
static uint32_t signalSerial=0,signalReceived=0,signalAt=0;
static int signalTrack=-999;
static float rmsLeft=0,rmsRight=0;
void signalMotion(){
 uint32_t now=millis();
 if(signalTrack!=f->trackId||f->scope.serial<signalSerial){signalTrack=f->trackId;signalSerial=f->scope.serial;signalReceived=now;rmsLeft=rmsRight=0;signalAt=now;}
 if(signalSerial!=f->scope.serial){signalSerial=f->scope.serial;signalReceived=now;}
 float dt=std::min<uint32_t>(250,now-signalAt);signalAt=now;
 float l=f->playing&&now-signalReceived<250?f->scope.rmsL:0;
 float r=f->playing&&now-signalReceived<250?f->scope.rmsR:0;
 rmsLeft+=(l-rmsLeft)*(1-expf(-dt/(l>rmsLeft?35.f:180.f)));
 rmsRight+=(r-rmsRight)*(1-expf(-dt/(r>rmsRight?35.f:180.f)));
}
// A 30ms presentation lag permits scrolling between known, real 10ms bins.
// Interpolation moves the trace, never generates waveform content.
float historyOffset(){return f->playing?-3.f+std::min(2.f,(millis()-signalReceived)/10.f):0.f;}
float sampleBin(const int8_t*data,int count,float index){index=std::max(0.f,std::min(float(count-1),index));int a=int(index),b=std::min(count-1,a+1);return data[a]+(data[b]-data[a])*(index-a);}
float energyBin(const uint8_t*data,float index){index=std::max(0.f,std::min(91.f,index));int a=int(index),b=std::min(91,a+1);return data[a]+(data[b]-data[a])*(index-a);}
uint16_t heat(float v){
 v=std::max(0.f,std::min(1.f,v));
 const uint16_t colors[]={rgb(33,30,63),rgb(110,58,203),rgb(16,159,225),rgb(0,221,171),rgb(225,225,46),rgb(255,75,124)};
 float at=v*5;int a=std::min(4,int(at));return blend(colors[a+1],colors[a],unsigned((at-a)*15));
}
uint16_t pixelCode(uint16_t cp){
 if(cp>='a'&&cp<='z')return cp-32;
 switch(cp){case 0xC0:case 0xC1:case 0xC2:case 0xC3:case 0xC4:case 0xC5:case 0xE0:case 0xE1:case 0xE2:case 0xE3:case 0xE4:case 0xE5:return 'A';case 0xC7:case 0xE7:return 'C';case 0xC8:case 0xC9:case 0xCA:case 0xCB:case 0xE8:case 0xE9:case 0xEA:case 0xEB:return 'E';case 0xCC:case 0xCD:case 0xCE:case 0xCF:case 0xEC:case 0xED:case 0xEE:case 0xEF:return 'I';case 0xD1:case 0xF1:return 'N';case 0xD2:case 0xD3:case 0xD4:case 0xD5:case 0xD6:case 0xF2:case 0xF3:case 0xF4:case 0xF5:case 0xF6:return 'O';case 0xD9:case 0xDA:case 0xDB:case 0xDC:case 0xF9:case 0xFA:case 0xFB:case 0xFC:return 'U';}
 return cp>=32&&cp<128?cp:'?';
}
PixelGlyph pglyph(uint16_t cp){PixelGlyph out;memcpy_P(&out,PixelGlyphs+pixelCode(cp)-32,sizeof(out));return out;}
int pixelWidth(const String&s,int scale){const char*q=s.c_str();int w=0;while(*q)w+=(pglyph(nextCode(q)).width+1)*scale;return std::max(0,w-scale);}
int scrollOffset(int overflow){if(overflow<=0)return 0;float travel=overflow*1000.f/23.f;float phase=fmodf(float(millis()-scrollStarted),2*travel+2400.f);if(phase<1200)return 0;if(phase<1200+travel)return int((phase-1200)*.023f);if(phase<2400+travel)return overflow;return std::max(0,overflow-int((phase-2400-travel)*.023f));}
void pixelText(int x,int y,const String&s,uint16_t color,int scale=1,int maxWidth=296,bool scroll=false){
 int left=x,right=std::min(320,x+maxWidth);if(scroll)x-=scrollOffset(pixelWidth(s,scale)-maxWidth);
 const char*q=s.c_str();while(*q){PixelGlyph gl=pglyph(nextCode(q));if(x>=right)break;
  for(int yy=0;yy<7;yy++)for(int xx=0;xx<gl.width;xx++)if(gl.rows[yy]&(1<<(gl.width-1-xx))){int px=x+xx*scale;int l=std::max(left,px),r=std::min(right,px+scale);if(r>l)g->fillRect(l,y+yy*scale,r-l,scale,color);}
  x+=(gl.width+1)*scale;
 }
}
void centeredPixel(int y,const String&s,uint16_t color,int scale=1){pixelText(std::max(12,(320-pixelWidth(s,scale))/2),y,s,color,scale,296,true);}
void box(int x,int y,int w,int h,uint16_t color){line(x,y,w,color);line(x,y+h-1,w,color);g->drawFastVLine(x,y,h,color);g->drawFastVLine(x+w-1,y,h,color);}
void checker(int x,int y,int w,int h,uint16_t color){for(int yy=0;yy<h;yy+=2)for(int xx=0;xx<w;xx+=2)if((xx+yy)%4==0)g->fillRect(x+xx,y+yy,2,2,color);}
void disk(int x,int y,int r,uint16_t color){for(int yy=-r;yy<=r;yy++){int w=int(sqrtf(float(r*r-yy*yy)));line(x-w,y+yy,w*2+1,color);}}
void miniReel(int x,int y,int radius,uint16_t fg,uint16_t bg){disk(x,y,radius,fg);disk(x,y,std::max(2,radius-3),bg);float a=dialSeconds*1.9f;g->fillRect(x+int(cosf(a)*(radius-5))-1,y+int(sinf(a)*(radius-5))-1,3,3,fg);}
void pixelFooter(){g->fillRect(0,220,320,20,p.panel);pixelText(12,227,f->playing?"PLAYING":"PAUSED",p.fg);pixelText(138,227,"VOL "+String(f->volume),p.fg);pixelText(252,227,"HOLD:MENU",p.fg);}
void artTheme(uint8_t t){
 for(int y=0;y<138;y++)memcpy_P(g->getBuffer()+(15+y)*320+3,ThemeArt[t]+y*314,314*2);
 line(0,0,320,p.dim);line(0,14,320,p.dim);line(0,154,320,p.dim);line(0,239,320,p.dim);
 static const char*const labels[]={"NEON / FISHBONE","SAKURA / TAPE","SUNVAIR / AUDIO","NASA / AUDIO","POCKET RADIO","LUNAR RADIO","TIDAL RADIO","AMBER RADIO","IRON CIRCUIT"};
 pixelText(9,5,labels[t],p.fg);pixelText(258,5,position(),p.fg,1,60);
 centeredPixel(160,f->title,p.fg,2);
 centeredPixel(179,f->artist,p.dim,2);
 // Quantities remain legible; decorative format labels were removed from this area.
 for(int n=0;n<31;n++)g->fillRect(13+int(n*9.5f),197,8,3,f->duration&&n*float(f->duration)/31<f->elapsed?p.accent:p.panel);
 int progress=f->duration?std::min<uint64_t>(290,uint64_t(f->elapsed)*290/f->duration):0;
 g->fillRect(12+progress,195,4,7,p.fg);
 pixelText(13,205,clockText(f->elapsed),p.fg,1,100);String end=f->duration?clockText(f->duration):"--:--";pixelText(307-pixelWidth(end,1),205,end,p.fg);
 uint16_t foot=t==0?rgb(255,219,39):t==1?rgb(243,223,198):p.bg;g->fillRect(4,217,312,21,foot);line(4,216,312,p.dim);
 pixelText(13,225,f->playing?"PLAYING":"PAUSED",p.fg);pixelText(140,225,"VOL "+String(f->volume),p.fg);pixelText(250,225,"HOLD:MENU",p.fg);
}
void melody(){
 uint16_t ink=rgb(52,38,62),lavender=rgb(155,130,196),gold=p.accent;
 g->fillRect(0,0,320,18,ink);pixelText(9,6,"MUSIC / LOCAL",p.fg);pixelText(253,6,position(),rgb(131,217,176),1,64);
 g->fillRect(9,25,302,38,lavender);box(9,25,302,38,ink);pixelText(16,31,"TRACK NAME",ink);g->fillRect(15,43,290,16,ink);pixelText(20,47,f->title,p.fg,1,280,true);
 g->fillRect(9,69,302,35,gold);pixelText(17,77,f->artist,ink,2,286,true);pixelText(17,94,format(),ink);
 g->fillRect(9,110,96,70,lavender);box(9,110,96,70,gold);g->fillRect(11,112,92,12,ink);pixelText(22,115,"VOLUME",gold);
 disk(57,147,14,ink);disk(57,145,13,p.fg);disk(57,145,10,gold);float a=-2.4f+f->volume*.048f;g->fillRect(56+int(sinf(a)*7),144-int(cosf(a)*7),3,3,ink);pixelText(30,168,String(f->volume)+" / 100",ink);
 g->fillRect(110,110,201,70,lavender);box(110,110,201,70,gold);g->fillRect(112,112,197,12,ink);pixelText(122,115,"TRANSPORT",gold);
 pixelText(123,135,clockText(f->elapsed),p.fg,3,120);pixelText(244,153,"/ "+(f->duration?clockText(f->duration):"--:--"),ink,1,60);pixelText(124,169,f->playing?"PLAYING":"PAUSED",ink);
 g->fillRect(9,188,302,22,ink);for(int n=0;n<49;n++)g->fillRect(14+n*6,194,3,10,f->duration&&n*float(f->duration)/49<f->elapsed?gold:lavender);pixelFooter();
}
void windowPanel(int x,int y,int w,int h,const String&caption){uint16_t ink=rgb(52,38,62),cream=rgb(255,241,210);g->fillRect(x+3,y+3,w,h,ink);g->fillRect(x,y,w,h,cream);box(x,y,w,h,ink);g->fillRect(x+3,y+3,w-6,13,rgb(182,213,233));pixelText(x+8,y+6,caption,ink);line(x+4,y+17,w-8,ink);box(x+w-15,y+5,9,9,ink);}
void pocketOs(){
 uint16_t ink=rgb(52,38,62),cream=rgb(255,241,210),mint=rgb(131,217,176);
 for(int y=18;y<240;y+=4)for(int x=(y%8?0:2);x<320;x+=4)g->drawPixel(x,y,rgb(102,152,211));
 g->fillRect(0,0,320,18,cream);pixelText(8,6,"POCKET OS",ink);pixelText(112,6,"MUSIC",ink);pixelText(270,6,"SD 120",ink,1,48);
 windowPanel(12,29,266,157,"PLAYER / "+position());g->fillRect(22,55,71,81,rgb(181,213,195));box(22,55,71,81,ink);miniReel(57,90,25,ink,mint);disk(57,90,6,cream);pixelText(39,123,"SIDE A",ink);
 pixelText(105,62,f->title,ink,2,161,true);pixelText(105,105,f->artist,ink,1,160,true);pixelText(105,124,format(),rgb(95,97,115),1,158);
 g->fillRect(22,145,246,5,rgb(181,184,186));int w=f->duration?std::min<uint64_t>(246,uint64_t(f->elapsed)*246/f->duration):0;g->fillRect(22,145,w,5,rgb(66,127,134));g->fillRect(21+w,142,4,11,ink);
 pixelText(22,161,clockText(f->elapsed),ink);pixelText(115,161,f->playing?"PLAYING":"PAUSED",ink);pixelText(241,161,f->duration?clockText(f->duration):"--:--",ink,1,35);
 box(290,37,20,23,ink);g->fillRect(294,40,12,10,rgb(232,141,120));pixelText(291,66,"SD",cream,1,20);
 windowPanel(129,193,176,35,"VOLUME");line(141,219,115,ink);g->fillRect(142+f->volume,213,8,12,mint);box(142+f->volume,213,8,12,ink);pixelText(273,215,String(f->volume),ink,1,26);pixelText(13,226,"HOLD:MENU",cream,1,105);
}
void ornate(int x,int y,int w,int h){box(x,y,w,h,p.fg);line(x+3,y+3,w-6,p.fg);line(x+3,y+h-4,w-6,p.fg);for(int i=0;i<4;i++){int a=i%2?x+w-8:x,b=i/2?y+h-8:y;g->fillRect(a,b,8,8,p.fg);g->fillRect(a+2,b+2,4,4,p.bg);}}
void dungeon(){
 ornate(7,7,306,39);pixelText(19,19,"VOL "+String(f->volume),p.fg,2,155);pixelText(193,19,position(),p.fg,2,115);
 for(int x=8;x<312;x+=16){box(x,54,14,10,p.fg);g->fillRect(x+3,57,3,2,p.fg);}ornate(8,69,304,109);
 pixelText(22,83,f->title,p.fg,2,276,true);pixelText(23,106,f->artist,p.fg,1,270,true);
 box(22,124,57,40,p.fg);box(28,129,45,19,p.fg);miniReel(39,139,6,p.fg,p.bg);miniReel(62,139,6,p.fg,p.bg);g->fillRect(36,155,27,5,p.fg);
 pixelText(99,130,clockText(f->elapsed),p.fg,3,122);pixelText(218,149,"/ "+(f->duration?clockText(f->duration):"--:--"),p.fg,1,83);pixelText(100,158,f->playing?"PLAYING":"PAUSED",p.fg);
 ornate(8,185,304,27);for(int n=0;n<36;n++)g->fillRect(16+n*8,194,5,9,f->duration&&n*float(f->duration)/36<f->elapsed?p.fg:p.panel);pixelFooter();
}
void navigator(){
 box(4,4,312,232,p.fg);g->fillRect(9,9,302,25,p.fg);pixelText(16,18,"DECK / LOCAL AUDIO",p.bg);pixelText(282,18,String(f->queuePos+1),p.bg,1,25);
 box(10,41,300,65,p.fg);pixelText(19,53,f->title,p.fg,2,282,true);pixelText(20,78,f->artist,p.fg,1,278,true);pixelText(279,89,"MP3",p.fg,1,28);
 g->fillRect(10,113,184,83,p.fg);pixelText(23,128,clockText(f->elapsed),p.bg,4,164);pixelText(26,169,"/ "+(f->duration?clockText(f->duration):"--:--"),p.bg,2,160);
 box(201,113,109,83,p.fg);g->fillRect(245,125,19,4,p.fg);g->fillRect(242,129,27,4,p.fg);g->fillRect(237,133,37,5,p.fg);g->fillRect(242,138,27,3,p.fg);g->fillRect(247,141,17,4,p.fg);g->fillRect(249,130,4,3,p.bg);g->fillRect(259,130,4,3,p.bg);
 pixelText(220,153,f->playing?"PLAYING":"PAUSED",p.fg,1,86);pixelText(220,175,"VOL "+String(f->volume),p.fg,2,86);
 for(int n=0;n<49;n++)g->fillRect(14+n*6,202,4,7,f->duration&&n*float(f->duration)/49<f->elapsed?p.fg:p.panel);g->fillRect(10,218,300,12,p.fg);pixelText(16,221,"HOLD:MENU",p.bg);pixelText(236,221,position(),p.bg,1,69);
}
void lightReport(){
 g->fillRect(8,7,89,18,rgb(225,234,60));text(13,21,"STRAND / 01",83,p.fg,StrandSmall,rgb(225,234,60));text(109,19,"LOCAL AUDIO",130,p.fg,StrandSmall,p.bg);text(252,19,position(),63,p.dim,StrandSmall,p.bg);line(12,31,296,p.dim);
 text(13,48,"NOW PLAYING",250,p.dim,StrandSmall,p.bg);text(12,80,f->title,296,p.fg,StrandTitle,p.bg);text(13,101,f->artist,295,p.accent,StrandBody,p.bg);text(13,119,f->album,295,p.dim,StrandSmall,p.bg);
 g->fillRect(12,132,296,45,p.panel);box(12,132,296,45,p.dim);for(int x=28;x<300;x+=17)g->drawFastVLine(x,135,39,blend(p.dim,p.panel,3));
 text(16,149,"L",10,p.fg,StrandSmall,p.panel);text(16,168,"R",10,p.fg,StrandSmall,p.panel);scope(28,146,92,3,9,f->scope.lowL,f->scope.highL,p.accent);scope(28,165,92,3,9,f->scope.lowR,f->scope.highR,rgb(132,72,159));
 text(12,200,clockText(f->elapsed),95,p.fg,StrandBody,p.bg);text(94,200,"/ "+(f->duration?clockText(f->duration):"--:--"),105,p.dim,StrandSmall,p.bg);text(235,200,"VOL "+String(f->volume),77,p.fg,StrandSmall,p.bg);bar(12,208,296,3,p.accent);text(12,234,f->playing?"PLAYING":"PAUSED",150,p.accent,StrandSmall,p.bg);text(230,234,"HOLD:MENU",85,p.dim,StrandSmall,p.bg);
}
void hexCell(int x,int y,int r,uint16_t color){g->fillRect(x-r+1,y-r/2,2*r-1,r+1,color);g->drawFastHLine(x-r+2,y-r/2-1,2*r-3,color);g->drawFastHLine(x-r+2,y+r/2+1,2*r-3,color);}
void phosphor(){
 text(10,19,"NOW PLAYING",180,p.dim);text(255,19,"SD/LOCAL",62,p.fg);line(10,25,300,p.panel);text(10,43,format(),230,p.accent);title(10,77,300);text(10,98,f->artist,300,p.accent,StrandBody);line(10,106,300,p.panel);
 text(10,121,"LEVEL HISTORY",160,p.dim);text(196,121,"STEREO RMS",113,p.dim);
 for(int row=0;row<6;row++)for(int col=0;col<20;col++){
  float index=col*91.f/19+historyOffset();float v=energyBin(row<3?f->scope.energyL:f->scope.energyR,index)/100.f;
  // Rows are amplitude thresholds; each column is an actual past 10ms RMS bin.
  float threshold=(row%3)*.14f;float strength=std::max(0.f,v-threshold)*2.f;
  hexCell(15+col*8+(row%2)*3,132+row*7,3,strength>.005f?heat(std::min(1.f,strength)):p.panel);
 }
 text(195,145,"L",12,p.dim);text(195,168,"R",12,p.dim);int levels[2]={int(rmsLeft),int(rmsRight)};
 for(int ch=0;ch<2;ch++)for(int n=0;n<12;n++)g->fillRect(211+n*8,136+ch*23,5,10,n<levels[ch]*12/100?heat(n/11.f):p.panel);
 text(10,201,clockText(f->elapsed),95,p.fg,StrandTitle);text(105,201,"/ "+(f->duration?clockText(f->duration):"--:--"),95,p.dim);text(238,201,"VOL "+String(f->volume),76,p.fg);bar(10,208,300,3,p.accent);text(10,233,f->playing?"PLAYING":"PAUSED",150,p.accent);text(231,233,"HOLD:MENU",85,p.dim);
}
void desktop(){
 box(5,5,310,230,p.fg);checker(8,8,304,7,p.fg);g->fillRect(8,19,304,13,p.fg);pixelText(14,22,"PLAYER.EXE",p.bg);pixelText(292,22,"X",p.bg,1,14);
 box(14,42,292,64,p.fg);g->fillRect(16,44,288,11,p.panel);pixelText(22,46,"TRACK "+position(),p.fg);pixelText(22,63,f->title,p.fg,2,276,true);pixelText(22,87,f->artist,p.fg,1,276,true);
 box(14,114,186,61,p.fg);checker(17,117,180,7,p.fg);pixelText(23,130,"STEREO RMS HISTORY",p.fg);
 for(int n=0;n<27;n++){float v=(energyBin(f->scope.energyL,n*91.f/26+historyOffset())+energyBin(f->scope.energyR,n*91.f/26+historyOffset()))*.5f;int h=int(v*25/100);if(h>0)g->fillRect(23+n*6,167-h,3,h,p.fg);}
 box(208,114,98,61,p.fg);pixelText(222,123,"VOLUME",p.fg);pixelText(222,141,String(f->volume),p.fg,3,78);
 pixelText(15,185,clockText(f->elapsed)+" / "+(f->duration?clockText(f->duration):"--:--"),p.fg);bar(15,201,291,4,p.fg);pixelText(15,220,f->playing?"PLAYING":"PAUSED",p.fg);pixelText(237,220,"HOLD:MENU",p.fg);
}
