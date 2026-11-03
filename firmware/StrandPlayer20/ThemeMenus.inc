// Navigation uses each selected theme's geometry and type, not just its palette.
static int menuSelected=-1;
static uint8_t menuTheme=255;
static String menuHeading,menuEntry;
void menuScroll(const ThemeMenuFrame&m,uint8_t theme){
 String entry=m.items&&m.selected>=0&&m.selected<m.count?m.items[m.selected]:"";
 if(menuTheme!=theme||menuSelected!=m.selected||menuHeading!=m.heading||menuEntry!=entry){scrollStarted=millis();menuTheme=theme;menuSelected=m.selected;menuHeading=m.heading;menuEntry=entry;}
}
String menuCounter(const ThemeMenuFrame&m){return m.count?String(m.selected+1)+"/"+String(m.count):"0/0";}
String menuLeft(const ThemeMenuFrame&m){return m.kind==ThemeMenuKind::Calibration?"LEFT:CANCEL":"LEFT:BACK";}
String menuRight(const ThemeMenuFrame&m){return m.kind==ThemeMenuKind::Themes?"CENTER:APPLY":"CENTER:SELECT";}
void aaMenuText(int x,int baseline,const String&s,int width,uint16_t fg,uint16_t bg,bool selected,const StrandFont&font=StrandBody){
 int overflow=measure(s,font)-width;
 if(selected&&overflow>0)rasterText(*g,x-scrollOffset(overflow),baseline,s,x,x+width,fg,bg,font);
 else strandText(*g,x,baseline,s,width,fg,bg,font);
}
void menuPixelFooter(const ThemeMenuFrame&m,uint16_t bg,uint16_t fg){
 g->fillRect(0,220,320,20,bg);pixelText(10,223,menuLeft(m),fg,2,134);pixelText(160,223,menuRight(m),fg,2,152);
}
void rowMarker(const ThemeMenuFrame&m,int index,int x,int y,uint16_t color){
 if(index==m.playingIndex){g->fillTriangle(x,y,x,y+6,x+4,y+3,color);}
 else if(index==m.appliedIndex)g->fillRect(x,y+1,4,4,color);
}
void menuThumb(const ThemeMenuFrame&m,int visible,int x,int y,int height,uint16_t track,uint16_t color){
 if(m.count<=visible)return;
 int h=std::max(7,height*visible/m.count),offset=m.top*(height-h)/std::max(1,m.count-visible);
 g->fillRect(x,y,2,height,track);g->fillRect(x,y+offset,2,h,color);
}
void menuArtBackdrop(uint8_t theme){
 g->fillScreen(p.bg);for(int y=0;y<138;y++)memcpy_P(g->getBuffer()+(15+y)*320+3,ThemeArt[theme]+y*314,314*2);
 static const char*const labels[]={"NEON / FISHBONE","SAKURA / TAPE","SUNVAIR / AUDIO","NASA / AUDIO","POCKET RADIO","LUNAR RADIO","TIDAL RADIO","AMBER RADIO","IRON CIRCUIT"};
 pixelText(9,5,labels[theme],p.fg);line(0,14,320,p.dim);line(0,154,320,p.dim);
}
void artworkMenu(uint8_t theme,const ThemeMenuFrame&m){
 menuArtBackdrop(theme);g->fillRect(25,34,274,150,blend(p.fg,p.bg,7));g->fillRect(22,31,274,150,p.fg);g->fillRect(24,33,270,146,p.bg);
 g->fillRect(28,40,3,12,p.accent);pixelText(36,43,m.heading,p.fg,1,190);pixelText(235,43,menuCounter(m),p.fg,1,53);
 for(int row=0;row<4&&m.top+row<m.count;row++){
  int index=m.top+row,y=62+row*26;bool focus=index==m.selected;uint16_t bg=focus?p.fg:p.bg,fg=focus?p.bg:p.fg;
  if(focus)g->fillRect(30,y,258,25,bg);
  rowMarker(m,index,33,y+8,fg);
  pixelText(42,y+3,m.items[index],fg,2,238,focus);
  if(m.secondary[row].length())pixelText(42,y+18,m.secondary[row],focus?p.bg:p.dim,1,238,focus);
 }
 menuThumb(m,4,290,62,106,p.panel,p.fg);
 if(!m.count)pixelText(36,91,m.emptyMessage,p.fg,1,245);
 // Context remains outside the modal; no cropped playback lettering underneath.
 pixelText(14,198,f->title,p.fg,1,292,true);
 uint16_t foot=theme==0?rgb(255,219,39):theme==1?rgb(243,223,198):p.bg;
 menuPixelFooter(m,foot,p.fg);
}
void melodyMenu(const ThemeMenuFrame&m){
 uint16_t ink=rgb(52,38,62),lavender=rgb(155,130,196),gold=p.accent;
 g->fillRect(0,0,320,18,ink);pixelText(9,6,"MUSIC / LOCAL",p.fg);pixelText(253,6,menuCounter(m),rgb(131,217,176),1,63);
 g->fillRect(9,25,302,38,lavender);box(9,25,302,38,ink);pixelText(16,31,m.heading,ink,1,282);
 g->fillRect(15,43,290,15,ink);pixelText(20,47,m.kind==ThemeMenuKind::List?"TRACK LIBRARY":"MENU / SELECT",p.fg);
 for(int row=0;row<4&&m.top+row<m.count;row++){
  int index=m.top+row,y=72+row*32;bool focus=index==m.selected;g->fillRect(9,y,302,28,focus?gold:lavender);box(9,y,302,28,ink);
  pixelText(17,y+10,String(index+1),ink,1,25);pixelText(47,y+8,m.items[index],ink,2,249,focus);rowMarker(m,index,298,y+10,ink);
 }
 if(!m.count)pixelText(20,105,m.emptyMessage,p.fg,1,285);
 menuPixelFooter(m,lavender,ink);
}
void pocketMenu(const ThemeMenuFrame&m){
 uint16_t ink=rgb(52,38,62),cream=rgb(255,241,210),mint=rgb(131,217,176),selectedBg=rgb(40,90,159);
 for(int y=18;y<240;y+=4)for(int x=(y%8?0:2);x<320;x+=4)g->drawPixel(x,y,rgb(102,152,211));
 g->fillRect(0,0,320,18,cream);pixelText(8,6,"POCKET OS",ink);pixelText(112,6,"MUSIC",ink);pixelText(267,6,menuCounter(m),ink,1,49);
 windowPanel(12,29,266,177,m.heading);
 for(int row=0;row<5&&m.top+row<m.count;row++){
  int index=m.top+row,y=56+row*27;bool focus=index==m.selected;g->fillRect(22,y,246,25,focus?selectedBg:cream);uint16_t fg=focus?cream:ink;
  g->fillRect(29,y+7,14,11,focus?mint:rgb(182,213,233));box(29,y+7,14,11,ink);g->fillRect(30,y+4,6,3,ink);rowMarker(m,index,33,y+9,ink);
  pixelText(52,y+3,m.items[index],fg,2,208,focus);if(m.secondary[row].length())pixelText(52,y+18,m.secondary[row],fg,1,208,focus);
 }
 if(!m.count)pixelText(24,83,m.emptyMessage,ink,1,238);
 box(290,37,20,23,ink);g->fillRect(294,40,12,10,rgb(232,141,120));pixelText(291,66,"SD",cream,1,20);menuPixelFooter(m,p.bg,cream);
}
void dungeonMenu(const ThemeMenuFrame&m){
 ornate(7,7,306,39);pixelText(19,19,"VOL "+String(f->volume),p.fg,2,153);pixelText(193,19,menuCounter(m),p.fg,2,112);
 pixelText(15,57,m.heading,p.fg,1,290);ornate(8,69,304,109);
 for(int row=0;row<3&&m.top+row<m.count;row++){
  int index=m.top+row,y=78+row*29;bool focus=index==m.selected;uint16_t bg=focus?p.fg:p.bg,fg=focus?p.bg:p.fg;if(focus)g->fillRect(17,y,286,24,bg);
  rowMarker(m,index,21,y+8,fg);pixelText(29,y+5,m.items[index],fg,2,265,focus);
 }
 if(!m.count)pixelText(23,104,m.emptyMessage,p.fg,1,270);
 ornate(8,185,304,27);pixelText(20,195,f->title,p.fg,1,280,true);menuPixelFooter(m,p.panel,p.fg);
}
void navigatorMenu(const ThemeMenuFrame&m){
 box(4,4,312,232,p.fg);g->fillRect(9,9,302,25,p.fg);pixelText(16,18,m.heading,p.bg,1,232);pixelText(268,18,menuCounter(m),p.bg,1,39);
 for(int row=0;row<4&&m.top+row<m.count;row++){
  int index=m.top+row,x=row%2?163:10,y=row/2?126:41;bool focus=index==m.selected;uint16_t bg=focus?p.fg:p.bg,fg=focus?p.bg:p.fg;
  g->fillRect(x,y,147,78,bg);box(x,y,147,78,p.fg);pixelText(x+10,y+13,m.items[index],fg,2,127,focus);
  // Four distinctive instrument tiles, matching the navigator mockup's layout.
  if(row==0){g->fillRect(x+20,y+46,35,16,fg);g->fillRect(x+25,y+42,12,4,fg);}
  else if(row==1){disk(x+108,y+52,12,fg);disk(x+108,y+52,5,bg);}
  else if(row==2){box(x+93,y+35,34,27,fg);box(x+99,y+41,22,15,fg);}
  else{g->fillRect(x+96,y+35,8,29,fg);g->fillRect(x+86,y+46,28,8,fg);}
  rowMarker(m,index,x+12,y+50,fg);pixelText(x+12,y+65,String(index+1),fg,1,45);
 }
 if(!m.count)pixelText(17,83,m.emptyMessage,p.fg,1,286);
 g->fillRect(10,218,300,12,p.fg);pixelText(16,221,menuLeft(m),p.bg,1,125);pixelText(225,221,menuRight(m),p.bg,1,84);
}
void technicalMenu(uint8_t theme,const ThemeMenuFrame&m){
 int rowX=12,rowY=68,rowW=296,rowH=27,visible=5;
 bool tapeSkin=theme==14,arcSkin=theme==16,chronoSkin=theme==17;
 if(theme==13||theme==15){uint16_t tag=rgb(225,234,60);g->fillRect(8,7,89,18,tag);aaMenuText(13,21,"STRAND / 01",83,rgb(24,53,70),tag,false,StrandSmall);aaMenuText(109,20,"LOCAL AUDIO",129,p.fg,p.bg,false,StrandSmall);line(12,31,296,p.dim);}
 else if(tapeSkin){aaMenuText(12,24,"STRAND / SESSION",230,p.fg,p.bg,false);line(12,33,296,p.dim);rowW=200;
  g->fillRect(222,43,86,137,rgb(7,14,18));for(int y=52;y<175;y+=12)line(226,y,78,rgb(28,43,51));g->drawFastVLine(246,53,115,p.panel);g->drawFastVLine(283,53,115,p.panel);
  scope(246,57,57,2,13,f->scope.lowL+35,f->scope.highL+35,p.fg,true);scope(283,57,57,2,13,f->scope.lowR+35,f->scope.highR+35,p.accent,true);
 }
 else if(arcSkin){
  rowX=106;rowY=66;rowW=202;rowH=32;visible=4;
  const float cx=-106,cy=127,base=185;for(int j=-12;j<=12;j++){float a=j*.063f;bool major=j%4==0;float rr=base-(major?22:10);g->drawLine(int(cx+cosf(a)*rr),int(cy+sinf(a)*rr),int(cx+cosf(a)*base),int(cy+sinf(a)*base),p.dim);}
  line(58,126,31,p.accent);line(95,126,4,p.accent);
 }
 else{aaMenuText(14,24,theme==18?"PHOSPHOR / LOCAL":"LOCAL / DISPLAY",235,p.accent,p.bg,false,StrandSmall);line(12,33,296,p.panel);}
 if(chronoSkin){rowY=72;rowH=32;visible=4;}
 aaMenuText(rowX,55,m.heading,rowW-65,p.fg,p.bg,false);aaMenuText(rowX+rowW-62,55,menuCounter(m),62,p.dim,p.bg,false,StrandSmall);
 for(int row=0;row<visible&&m.top+row<m.count;row++){
  int index=m.top+row,y=rowY+row*rowH;bool focus=index==m.selected;uint16_t bg=focus?p.panel:p.bg,fg=p.fg;
  if(focus){if(chronoSkin){bg=p.accent;fg=0;g->fillRoundRect(rowX,y,rowW,rowH-3,8,bg);}else if(arcSkin)g->fillRoundRect(rowX,y,rowW,rowH-3,13,bg);else{g->fillRect(rowX,y,rowW,rowH-3,bg);g->fillRect(rowX,y,3,rowH-3,p.accent);}}
  rowMarker(m,index,rowX+7,y+9,focus?fg:p.accent);aaMenuText(rowX+17,y+19,m.items[index],rowW-23,fg,bg,focus);
 }
 if(!m.count)aaMenuText(rowX,104,m.emptyMessage,rowW,p.dim,p.bg,false);
 menuThumb(m,visible,311,rowY,rowH*visible-4,p.panel,p.accent);
 if(arcSkin)g->fillRect(0,212,320,28,p.bg);
 line(12,216,296,p.panel);aaMenuText(13,234,menuLeft(m),126,p.dim,p.bg,false,StrandSmall);aaMenuText(217,234,menuRight(m),99,p.accent,p.bg,false,StrandSmall);
}
void retroMenu(const ThemeMenuFrame&m){
 box(5,5,310,230,p.fg);checker(8,8,304,7,p.fg);g->fillRect(8,19,304,13,p.fg);pixelText(14,22,"LIBRARY.EXE",p.bg);pixelText(292,22,"X",p.bg,1,14);
 pixelText(15,43,m.heading,p.fg,1,228);pixelText(267,43,menuCounter(m),p.fg,1,40);
 box(14,58,292,148,p.fg);
 for(int row=0;row<5&&m.top+row<m.count;row++){
  int index=m.top+row,y=64+row*27;bool focus=index==m.selected;uint16_t bg=focus?p.fg:p.bg,fg=focus?p.bg:p.fg;if(focus)g->fillRect(18,y-1,284,25,bg);
  rowMarker(m,index,23,y+7,fg);pixelText(33,y+4,m.items[index],fg,2,261,focus);
 }
 if(!m.count)pixelText(23,86,m.emptyMessage,p.fg,1,273);
 pixelText(15,219,menuLeft(m),p.fg,2,135);pixelText(160,219,menuRight(m),p.fg,2,148);
}
void technicalBrightness(uint8_t theme,int value,bool available){
 String number=String(value);bool arcSkin=theme==16,chronoSkin=theme==17;
 int x=25,width=270,baseline=128,barY=151;
 if(theme==13||theme==15){uint16_t tag=rgb(225,234,60);g->fillRect(8,7,89,18,tag);aaMenuText(13,21,"STRAND / 01",83,rgb(24,53,70),tag,false,StrandSmall);aaMenuText(109,20,"DISPLAY",129,p.fg,p.bg,false,StrandSmall);line(12,31,296,p.dim);aaMenuText(14,55,"Brightness",270,p.fg,p.bg,false);}
 else if(theme==14){aaMenuText(12,24,"STRAND / SESSION",230,p.fg,p.bg,false);line(12,33,296,p.dim);aaMenuText(12,54,"Brightness",200,p.fg,p.bg,false);x=13;width=194;
  g->fillRect(222,43,86,137,rgb(7,14,18));for(int y=52;y<175;y+=12)line(226,y,78,rgb(28,43,51));scope(246,57,57,2,13,f->scope.lowL+35,f->scope.highL+35,p.fg,true);scope(283,57,57,2,13,f->scope.lowR+35,f->scope.highR+35,p.accent,true);
 }
 else if(arcSkin){
  const float cx=-106,cy=127,base=185;for(int j=-12;j<=12;j++){float a=j*.063f;float rr=base-(j%4==0?22:10);g->drawLine(int(cx+cosf(a)*rr),int(cy+sinf(a)*rr),int(cx+cosf(a)*base),int(cy+sinf(a)*base),p.dim);}line(58,126,31,p.accent);line(95,126,4,p.accent);
  aaMenuText(111,57,"Brightness",193,p.fg,p.bg,false);g->fillRoundRect(105,81,202,61,30,p.panel);x=119;width=176;baseline=132;barY=164;
 }
 else{aaMenuText(14,27,theme==18?"PHOSPHOR / DISPLAY":"Brightness",291,theme==18?p.accent:p.fg,p.bg,false);line(12,35,296,p.panel);if(theme==18)aaMenuText(15,58,"BACKLIGHT OUTPUT",260,p.dim,p.bg,false,StrandSmall);}
 uint16_t numberBg=arcSkin?p.panel:p.bg;
 aaMenuText(x,baseline,number,width,p.fg,numberBg,false,StrandClock);
 aaMenuText(x+measure(number,StrandClock)+5,baseline-3,"%",32,p.dim,numberBg,false,StrandBody);
 int left=arcSkin?111:x,trackWidth=arcSkin?194:width;
 if(chronoSkin){
  g->fillRoundRect(14,150,292,48,10,p.accent);aaMenuText(25,166,"5%",48,0,p.accent,false,StrandSmall);aaMenuText(268,166,"100%",35,0,p.accent,false,StrandSmall);
  for(int j=0;j<=64;j++)g->drawFastVLine(25+int(j*4.2f),175,j%8==0?9:4,rgb(132,51,11));g->fillRect(25,177,value*269/100,3,0);g->fillRect(24+value*269/100,172,2,13,0);
 }else{
  g->fillRect(left,barY,trackWidth,7,p.panel);if(theme==18){for(int i=0;i<27;i++)g->fillRect(left+i*10,barY,7,7,i<value*27/100?p.accent:p.panel);}else g->fillRect(left,barY,value*trackWidth/100,7,p.accent);
  g->fillRect(left+std::min(trackWidth-3,value*trackWidth/100),barY-3,3,13,p.fg);aaMenuText(left,barY+26,"5%",40,p.dim,p.bg,false,StrandSmall);aaMenuText(left+trackWidth-40,barY+26,"100%",40,p.dim,p.bg,false,StrandSmall);
  aaMenuText(left,arcSkin?209:202,available?"WHEEL: ADJUST":"PWM UNAVAILABLE",trackWidth,p.fg,p.bg,false,StrandSmall);
 }
 if(arcSkin)g->fillRect(0,212,320,28,p.bg);
 line(12,216,296,p.panel);aaMenuText(13,234,"LEFT: CANCEL",140,p.dim,p.bg,false,StrandSmall);aaMenuText(218,234,"CENTER: SAVE",94,p.accent,p.bg,false,StrandSmall);
}
void menuBrightness(uint8_t theme,int value,bool available){
 if(theme>=13&&theme<=18){technicalBrightness(theme,value,available);return;}
 uint16_t bg=p.bg,fg=p.fg,accent=p.accent;int left=25,top=55,w=270;
 if(theme<9){menuArtBackdrop(theme);g->fillRect(22,31,274,150,p.fg);g->fillRect(24,33,270,146,p.bg);pixelText(36,43,"BRIGHTNESS",p.accent);left=36;top=65;w=246;}
 else if(theme==9){g->fillRect(0,0,320,18,rgb(52,38,62));pixelText(9,6,"MUSIC / DISPLAY",p.fg);g->fillRect(9,25,302,179,rgb(155,130,196));box(9,25,302,179,p.accent);bg=rgb(155,130,196);fg=rgb(52,38,62);left=23;top=57;w=274;pixelText(20,34,"BRIGHTNESS",fg,2);}
 else if(theme==10){windowPanel(12,29,286,177,"DISPLAY / BRIGHTNESS");bg=rgb(255,241,210);fg=rgb(52,38,62);left=29;top=66;w=251;}
 else if(theme==11){ornate(7,7,306,39);pixelText(22,20,"DISPLAY",p.fg,2);ornate(8,69,304,109);top=81;left=24;w=272;pixelText(15,56,"BRIGHTNESS",p.fg);}
 else if(theme==12){box(4,4,312,232,p.fg);g->fillRect(9,9,302,25,p.fg);pixelText(16,18,"DECK / DISPLAY",p.bg);box(10,45,300,158,p.fg);pixelText(20,55,"BRIGHTNESS",p.fg,2);top=81;left=25;w=270;}
 else if(theme==19){box(5,5,310,230,p.fg);checker(8,8,304,7,p.fg);g->fillRect(8,19,304,13,p.fg);pixelText(14,22,"DISPLAY.EXE",p.bg);box(14,47,292,156,p.fg);pixelText(24,59,"BRIGHTNESS",p.fg,2);top=87;}
 else{aaMenuText(14,26,"DISPLAY / BRIGHTNESS",287,p.fg,p.bg,false);line(12,35,296,p.panel);top=63;}
 bool pixel=theme<=12||theme==19;
 if(pixel){String number=String(value)+"%";pixelText(left+(w-pixelWidth(number,4))/2,top+12,number,fg,4,w);}
 else aaMenuText(27,top+65,String(value)+"%",266,fg,bg,false,StrandClock);
 int y=top+(pixel?64:83);g->fillRect(left,y,w,7,theme==9?rgb(95,72,114):p.panel);g->fillRect(left,y,value*w/100,7,accent);g->fillRect(left+std::min(w-3,value*w/100),y-3,3,13,fg);
 if(pixel){pixelText(left,y+18,"5%",fg);pixelText(left+w-24,y+18,"100%",fg);pixelText(left,y+39,available?"WHEEL:ADJUST":"PWM UNAVAILABLE",fg,1,w);}
 else{aaMenuText(left,y+26,"5%",50,p.dim,bg,false,StrandSmall);aaMenuText(left+w-43,y+26,"100%",43,p.dim,bg,false,StrandSmall);aaMenuText(left,y+50,available?"WHEEL: ADJUST":"PWM UNAVAILABLE",w,p.fg,bg,false,StrandSmall);}
 if(pixel){g->fillRect(0,220,320,20,p.panel);pixelText(12,223,"LEFT:CANCEL",p.fg,2,142);pixelText(172,223,"CENTER:SAVE",p.fg,2,142);}
 else{line(12,216,296,p.panel);aaMenuText(13,234,"LEFT: CANCEL",140,p.dim,p.bg,false,StrandSmall);aaMenuText(218,234,"CENTER: SAVE",94,p.accent,p.bg,false,StrandSmall);}
}
