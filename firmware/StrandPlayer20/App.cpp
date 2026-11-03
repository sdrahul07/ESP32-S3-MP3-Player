#include <Arduino.h>
#include <SPI.h>

#include <SD.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Adafruit_ST7789.h>


#include <driver/i2s_std.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <memory>
#include <math.h>
#include "Config.h"
#include "Media.h"
#include "Decoder.h"
#include "Themes.h"
#include "PcmScope.h"
#include "DisplayCalibration.h"
#include "StrandFonts.h"
#include <Preferences.h>

// MP3 player release candidate. See README for validation and limitations.
SemaphoreHandle_t spiMutex=nullptr;
SemaphoreHandle_t stateMutex=nullptr;
QueueHandle_t commands=nullptr;
QueueHandle_t inputEvents=nullptr;
Adafruit_ST7789 tft(&SPI,Pins::TFT_CS,Pins::TFT_DC,Pins::TFT_RST);

GFXcanvas16* screen=nullptr;

i2s_chan_handle_t tx=nullptr;
uint16_t BLACK=0x0000,CYAN=0x57F5,WHITE=0xEFFF,AMBER=0xB0FF,DIM=0x7C70,PANEL=0x08C2;
Preferences appearanceStore;
uint8_t activeTheme=0;
bool appearanceReady=false;
uint8_t displayBrightness=100,brightnessEntry=100;
bool brightnessPwm=false;
DisplayCalibration displayCalibration,calibrationEntry,parameterEntry;
uint16_t* calibrationLut=nullptr;
bool calibrationDirty=true,calibrationActive=false,calibrationMapped=false;
int calibrationPattern=0,calibrationParameter=0,calibrationReturnRow=0;

void setDisplayBrightness(int value){
  displayBrightness=uint8_t(std::max(5,std::min(100,value)));
  if(brightnessPwm)ledcWrite(Pins::BL,(uint32_t(displayBrightness)*255+50)/100);
}
void initBacklight(){
  brightnessPwm=ledcAttach(Pins::BL,20000,8);
  if(brightnessPwm)setDisplayBrightness(displayBrightness);
  else{pinMode(Pins::BL,OUTPUT);digitalWrite(Pins::BL,HIGH);Serial.println("Backlight PWM unavailable; using full brightness.");}
}
void applyThemePalette(){
  ThemePalette p=playerPalette(activeTheme);
  BLACK=p.bg;WHITE=p.fg;CYAN=p.accent;DIM=p.dim;PANEL=p.panel;
}
void loadAppearance(){
  appearanceReady=appearanceStore.begin("player-ui",false);
  uint8_t saved=appearanceReady?appearanceStore.getUChar("theme20",255):0;
  if(saved>=PLAYER_THEME_COUNT&&appearanceReady){uint8_t legacy=appearanceStore.getUChar("theme5",255);const uint8_t map[]={13,14,16,17,0};saved=legacy<5?map[legacy]:0;}
  activeTheme=saved<PLAYER_THEME_COUNT?saved:0;applyThemePalette();
  if(appearanceReady&&appearanceStore.getBytesLength("colorV1")==sizeof(DisplayCalibration)){
    DisplayCalibration storedColor;
    if(appearanceStore.getBytes("colorV1",&storedColor,sizeof(storedColor))==sizeof(storedColor)&&validCalibration(storedColor))displayCalibration=storedColor;
  }
  uint8_t stored=appearanceReady?appearanceStore.getUChar("brightness",100):100;
  displayBrightness=uint8_t(std::max(5,std::min(100,int(stored))));
}
enum class Action:uint8_t{Play,Toggle,Next,Previous,Seek,Stop};
struct Command{Action action;int value;};
struct State{
  int track=-1,queuePos=-1,volume=12,repeat=0;
  bool playing=false,shuffle=false;
  uint32_t elapsed=0,duration=0,rate=0;
  uint16_t bitrate=0;uint8_t channels=2;
  uint8_t vuL=0,vuR=0;
  PcmScope scope;
  uint32_t revision=0,messageAt=0;
  char message[80]="Ready: Up opens library";
};
State state;
std::vector<int> playQueue;
uint32_t queueRevision=0;
struct StateLock{StateLock(){xSemaphoreTake(stateMutex,portMAX_DELAY);}~StateLock(){xSemaphoreGive(stateMutex);}};
State snapshot(){StateLock lock;return state;}
void message(const String&s){StateLock lock;s.toCharArray(state.message,sizeof(state.message));state.revision++;state.messageAt=millis();Serial.println(s);}
void send(Action a,int v=0){Command c={a,v};if(xQueueSend(commands,&c,0)!=pdTRUE)message("Busy: try again");}
void fail(const String&s){Serial.println(s);tft.fillScreen(BLACK);tft.setCursor(8,24);tft.setTextColor(AMBER);tft.setTextWrap(true);tft.print(s);while(true)delay(100);}

// PCM5102A: hardware-configured Philips I2S. No I2C, reset or codec library.
bool setupAudioClock(uint32_t rate){
  if(tx){
    i2s_channel_disable(tx);
    i2s_del_channel(tx);
    tx=nullptr;
  }
  i2s_chan_config_t ch=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
  ch.dma_desc_num=24;ch.dma_frame_num=256;
  ch.auto_clear=true;
  esp_err_t result=i2s_new_channel(&ch,&tx,nullptr);
  if(result!=ESP_OK){Serial.printf("I2S allocation error: %s\n",esp_err_to_name(result));return false;}
  i2s_std_config_t cfg={};
  cfg.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(rate);
  cfg.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,I2S_SLOT_MODE_STEREO);
  cfg.gpio_cfg.mclk=I2S_GPIO_UNUSED;
  cfg.gpio_cfg.bclk=gpio_num_t(Pins::BCK);
  cfg.gpio_cfg.ws=gpio_num_t(Pins::WS);
  cfg.gpio_cfg.dout=gpio_num_t(Pins::DOUT);
  cfg.gpio_cfg.din=I2S_GPIO_UNUSED;
  result=i2s_channel_init_std_mode(tx,&cfg);
  if(result==ESP_OK)result=i2s_channel_enable(tx);
  if(result!=ESP_OK){Serial.printf("I2S setup error: %s\n",esp_err_to_name(result));i2s_del_channel(tx);tx=nullptr;return false;}
  Serial.printf("PCM5102A I2S: %lu Hz, stereo 32-bit, BCK4/WS5/DIN6\n",(unsigned long)rate);
  return true;
}
int nextPosition(int current,bool backwards,bool automatic){
  StateLock lock;if(playQueue.empty())return -1;
  if(automatic&&state.repeat==2)return current;
  if(state.shuffle&&!backwards&&playQueue.size()>1){int n;do{n=esp_random()%playQueue.size();}while(n==current);return n;}
  int n=current+(backwards?-1:1);
  if(n>=int(playQueue.size()))return automatic&&state.repeat==0?-1:0;
  if(n<0)n=playQueue.size()-1;return n;
}
int queueTrack(int p){StateLock lock;return p>=0&&p<int(playQueue.size())?playQueue[p]:-1;}
uint32_t queueVersion(){StateLock lock;return queueRevision;}
std::unique_ptr<Decoder> openDecoder(int track){
  if(track<0||track>=int(tracks.size()))return nullptr;
  auto d=std::unique_ptr<Decoder>(new Decoder);
  if(!d->open(tracks[track].path)||!(d->rate==32000||d->rate==44100||d->rate==48000))return nullptr;
  return d;
}
void audioTask(void*){
  std::unique_ptr<Decoder> active,prepared;
  int activePos=-1,preparedPos=-1;uint32_t preparedVersion=0,clockRate=0,lastState=0;
  bool paused=false;float gain=0;PcmAnalyzer analyzer;
  int32_t pcm[AUDIO_FRAMES*2];
  for(;;){
    Command cmd;
    while(xQueueReceive(commands,&cmd,0)==pdTRUE){
      if(cmd.action==Action::Toggle){
        if(active){paused=!paused;StateLock lock;state.playing=!paused;state.revision++;continue;}
        cmd.action=Action::Play;cmd.value=std::max(0,activePos);
      }
      if(cmd.action==Action::Seek){
        if(active){int64_t p=int64_t(active->position)+int64_t(cmd.value)*active->rate;
          if(active->frames)p=std::min<int64_t>(p,active->frames-1);
          if(!active->seek(std::max<int64_t>(0,p)))message("Seek failed");prepared.reset();analyzer.reset();{StateLock lock;state.scope=PcmScope{};state.elapsed=active->position/active->rate;}}
        continue;
      }
      if(cmd.action==Action::Stop){active.reset();prepared.reset();analyzer.reset();StateLock lock;state.scope=PcmScope{};state.playing=false;state.vuL=state.vuR=0;state.revision++;continue;}
      int target=cmd.action==Action::Play?cmd.value:nextPosition(activePos,cmd.action==Action::Previous,false);
      active.reset();prepared.reset();paused=false;
      int index=queueTrack(target);active=openDecoder(index);
      if(!active){message("Cannot decode MP3 / unsupported rate");StateLock lock;state.playing=false;continue;}
      if(clockRate!=active->rate){if(!setupAudioClock(active->rate)){active.reset();message("DAC/I2S clock setup failed");continue;}clockRate=active->rate;}
      // PCM5102A automatically derives its system clock from BCK.
      activePos=target;gain=0;analyzer.reset();
      {StateLock lock;state.scope=PcmScope{};state.track=index;state.queuePos=target;state.playing=true;state.rate=active->rate;state.elapsed=0;state.duration=active->frames/active->rate;state.channels=active->channels;state.bitrate=active->mp3?active->mp3->info.bitrate_kbps:0;state.revision++;}
      message("Playing MP3");
    }
    
    if(!active||paused){if(tx){int32_t zero[128]={};size_t written;i2s_channel_write(tx,zero,sizeof(zero),&written,30);}else delay(10);continue;}
    uint32_t version=queueVersion();
    if(prepared&&preparedVersion!=version){prepared.reset();preparedPos=-1;}
    if(active->frames&&active->frames>active->position&&active->frames-active->position<active->rate*2ULL&&!prepared){
      preparedPos=nextPosition(activePos,false,true);int idx=queueTrack(preparedPos);
      if(idx>=0){prepared=openDecoder(idx);preparedVersion=version;}
    }
    size_t n=active->read(pcm,AUDIO_FRAMES);
    if(!n){
      int next=nextPosition(activePos,false,true);
      if(prepared&&preparedPos==next&&preparedVersion==queueVersion())active=std::move(prepared);
      else active=openDecoder(queueTrack(next));
      if(!active){StateLock lock;state.playing=false;state.vuL=state.vuR=0;state.revision++;continue;}
      if(clockRate!=active->rate){if(!setupAudioClock(active->rate)){active.reset();message("Clock change failed");continue;}clockRate=active->rate;}
      activePos=next;analyzer.reset();
      int index=queueTrack(next);
      {StateLock lock;state.scope=PcmScope{};state.track=index;state.queuePos=next;state.elapsed=0;
        state.rate=active->rate;state.duration=active->frames/active->rate;state.channels=active->channels;state.bitrate=active->mp3?active->mp3->info.bitrate_kbps:0;state.revision++;}
      continue;
    }
    analyzer.push(pcm,n,active->rate);
    bool publishScope=millis()-lastState>=20;PcmScope scope;
    if(publishScope)analyzer.snapshot(scope);
    State s=snapshot();float target=s.volume==0?0:powf(10.0f,(-60.0f+0.6f*s.volume)/20.0f);
    uint32_t peakL=0,peakR=0;
    for(size_t i=0;i<n;i++){
      gain+=(target-gain)*0.002f;
      for(int c=0;c<2;c++){uint32_t level=uint32_t(llabs(int64_t(pcm[2*i+c])))>>23;int32_t v=int32_t(float(pcm[2*i+c])*gain);pcm[2*i+c]=v;if(c)peakR=std::max(peakR,level);else peakL=std::max(peakL,level);}
    }
    size_t bytes=n*8,offset=0;
    while(offset<bytes){size_t written=0;esp_err_t err=i2s_channel_write(tx,reinterpret_cast<uint8_t*>(pcm)+offset,bytes-offset,&written,200);if(err!=ESP_OK||!written){message("I2S output timeout");break;}offset+=written;}
    if(publishScope){StateLock lock;state.scope=scope;state.elapsed=active->position/active->rate;state.bitrate=active->mp3?active->mp3->info.bitrate_kbps:0;state.vuL=scope.rmsL;state.vuR=scope.rmsR;lastState=millis();}
    delay(1);
  }
}

// UI/artwork run on the main task; the audio task owns I2S.
enum class View{Now,Home,List,Actions,Settings,Appearance,Themes,Display,Brightness,Calibration,ColorEdit,ColorTest};
View view=View::Now;
enum class ListKind{Tracks,Artists,Albums,Folders,Playlists,Queue};
ListKind listKind=ListKind::Tracks;
std::vector<String> rows;
std::vector<int> rowTracks;
int listTop=0;int selected=0,actionTrack=-1,actionQueuePos=-1,actionRow=0,parentRow=0;bool groupList=false,groupChild=false;String groupName;
bool uiDirty=true;uint32_t lastUi=0,lastRevision=0;
void textAt(int x,int y,const String&s,uint16_t color=CYAN,int size=1,int maxChars=49){screen->setFont(nullptr);screen->setTextSize(size);screen->setTextColor(color);screen->setCursor(x,y);screen->print(s.substring(0,maxChars));}
String timeText(uint32_t s){char b[20];snprintf(b,sizeof(b),"%lu:%02lu",(unsigned long)(s/60),(unsigned long)(s%60));return b;}
void buildList(ListKind kind){
  listKind=kind;rows.clear();rowTracks.clear();selected=0;listTop=0;groupList=false;groupChild=false;groupName="";
  if(kind==ListKind::Queue){StateLock lock;for(int i:playQueue){rows.push_back(tracks[i].title);rowTracks.push_back(i);}}
  else if(kind==ListKind::Playlists){rows=playlists;}
  else for(size_t i=0;i<tracks.size();i++){
    if(kind==ListKind::Tracks){rows.push_back(tracks[i].title);rowTracks.push_back(i);}
    else {String key=kind==ListKind::Artists?tracks[i].artist:kind==ListKind::Albums?tracks[i].album:tracks[i].path.substring(0,tracks[i].path.lastIndexOf('/'));
      if(key.isEmpty())key="/";if(std::find(rows.begin(),rows.end(),key)==rows.end())rows.push_back(key);groupList=true;}
  }
  if(groupList)std::sort(rows.begin(),rows.end(),[](const String&a,const String&b){return a.compareTo(b)<0;});
  view=View::List;uiDirty=true;
}
void playList(const std::vector<int>&items,int pos){
  send(Action::Stop);{StateLock lock;playQueue=items;queueRevision++;}send(Action::Play,pos);view=View::Now;uiDirty=true;
}
void selectRow(){
  if(rows.empty())return;
  if(listKind==ListKind::Playlists){std::vector<int> items;if(readPlaylist(rows[selected],items))playList(items,0);else message("Playlist empty / paths not found");return;}
  if(groupList){String group=rows[selected];parentRow=selected;groupName=group;groupChild=true;listTop=0;rows.clear();rowTracks.clear();
    for(size_t i=0;i<tracks.size();i++){String key=listKind==ListKind::Artists?tracks[i].artist:listKind==ListKind::Albums?tracks[i].album:tracks[i].path.substring(0,tracks[i].path.lastIndexOf('/'));if(key.isEmpty())key="/";
      if(key==group){rows.push_back(tracks[i].title);rowTracks.push_back(i);}}
    groupList=false;selected=0;uiDirty=true;return;
  }
  if(listKind==ListKind::Queue){send(Action::Play,selected);view=View::Now;}else playList(rowTracks,selected);
  uiDirty=true;
}
void homeSelect(){
  switch(selected){
    case 0:view=View::Now;break;
    case 1:buildList(ListKind::Tracks);break;case 2:buildList(ListKind::Artists);break;
    case 3:buildList(ListKind::Albums);break;case 4:buildList(ListKind::Folders);break;
    case 5:buildList(ListKind::Playlists);break;case 6:buildList(ListKind::Queue);break;
    case 7:{StateLock lock;state.shuffle=!state.shuffle;queueRevision++;state.revision++;}break;
    case 8:{StateLock lock;state.repeat=(state.repeat+1)%3;queueRevision++;state.revision++;}break;
    case 9:view=View::Settings;selected=0;listTop=0;break;
    case 10:{std::vector<int> items;{StateLock lock;items=playQueue;}String p=savePlaylist(items);if(p.length()){playlists.push_back(p);message("Saved "+p);}else message("Could not save queue");}break;
    case 11:send(Action::Stop);break;
  }uiDirty=true;
}
int16_t& calibrationValue(int i){switch(i){case 0:return displayCalibration.red;case 1:return displayCalibration.green;case 2:return displayCalibration.blue;case 3:return displayCalibration.gamma;case 4:return displayCalibration.saturation;default:return displayCalibration.contrast;}}
const char* calibrationName(int i){static const char*names[]={"Red gain","Green gain","Blue gain","Gamma","Saturation","Contrast"};return names[std::max(0,std::min(5,i))];}
void enterCalibration(){calibrationEntry=displayCalibration;calibrationActive=true;view=View::Calibration;selected=0;listTop=0;uiDirty=true;}
void leaveCalibration(bool save){
  if(save){bool saved=appearanceReady&&appearanceStore.putBytes("colorV1",&displayCalibration,sizeof(displayCalibration))==sizeof(displayCalibration);message(saved?"Display colors saved":"Applied; could not save colors");}
  else{displayCalibration=calibrationEntry;calibrationDirty=true;}
  calibrationActive=false;view=View::Display;selected=1;listTop=0;uiDirty=true;
}
void calibrationSelect(){
  if(selected==0){view=View::ColorTest;uiDirty=true;return;}
  if(selected>=1&&selected<=6){calibrationParameter=selected-1;calibrationReturnRow=selected;parameterEntry=displayCalibration;view=View::ColorEdit;uiDirty=true;return;}
  if(selected==7)leaveCalibration(true);
  else if(selected==8)leaveCalibration(false);
  else if(selected==9){displayCalibration=DisplayCalibration{};calibrationDirty=true;uiDirty=true;message("Defaults previewed; Save to keep");}
}
void activate(){if(view==View::Now){State s=snapshot();if(s.track<0)buildList(ListKind::Tracks);else send(Action::Toggle);}
  else if(view==View::Settings){view=selected==0?View::Appearance:View::Display;selected=0;listTop=0;uiDirty=true;}
  else if(view==View::Display){if(selected==0){brightnessEntry=displayBrightness;view=View::Brightness;uiDirty=true;}else enterCalibration();}
  else if(view==View::Calibration)calibrationSelect();
  else if(view==View::ColorEdit){view=View::Calibration;selected=calibrationReturnRow;uiDirty=true;}
  else if(view==View::ColorTest){calibrationPattern=(calibrationPattern+1)%3;uiDirty=true;}
  else if(view==View::Brightness){
    bool saved=displayBrightness==brightnessEntry||(appearanceReady&&appearanceStore.putUChar("brightness",displayBrightness)==1);
    brightnessEntry=displayBrightness;view=View::Display;selected=0;uiDirty=true;
    message(saved?"Brightness saved":"Brightness applied; could not save setting");
  }
  else if(view==View::Appearance){view=View::Themes;selected=activeTheme;listTop=0;uiDirty=true;}
  else if(view==View::Themes){
    if(selected>=0&&selected<PLAYER_THEME_COUNT){
      bool changed=activeTheme!=selected;activeTheme=uint8_t(selected);applyThemePalette();
      bool saved=!changed||(appearanceReady&&appearanceStore.putUChar("theme20",activeTheme)==1);
      view=View::Now;selected=0;uiDirty=true;
      message(saved?String("Theme: ")+playerThemeNames[activeTheme]:"Theme applied; could not save setting");
    }
  }
  else if(view==View::Home)homeSelect();else if(view==View::List)selectRow();
  else if(view==View::Actions){
    if(selected<2&&actionTrack>=0){StateLock lock;
      if(selected==0)playQueue.push_back(actionTrack);else {int pos=std::min<int>(playQueue.size(),std::max(0,state.queuePos+1));playQueue.insert(playQueue.begin()+pos,actionTrack);}queueRevision++;}
    else if(selected==2&&listKind==ListKind::Queue){State s=snapshot();if(s.playing)message("Pause/stop before removing queue items");else{send(Action::Stop);StateLock lock;if(actionQueuePos>=0&&actionQueuePos<int(playQueue.size())){playQueue.erase(playQueue.begin()+actionQueuePos);queueRevision++;}}}
    if(listKind==ListKind::Queue)buildList(ListKind::Queue);else view=View::List;selected=std::min(actionRow,std::max(0,int(rows.size())-1));uiDirty=true;
  }
}
void moveSelection(int n){
  if(view==View::ColorEdit){int16_t&v=calibrationValue(calibrationParameter);int min=calibrationParameter==4?0:50,max=calibrationParameter==3?200:150;v=int16_t(std::max(min,std::min(max,int(v)+n*5)));calibrationDirty=true;}
  else if(view==View::ColorTest){calibrationPattern=(calibrationPattern+n%3+3)%3;}
  else if(view==View::Brightness){if(brightnessPwm)setDisplayBrightness(int(displayBrightness)+n*5);else message("Brightness PWM unavailable");}
  else if(view==View::Now){StateLock lock;state.volume=std::max(0,std::min(100,state.volume+n));state.revision++;}
  else {int count=view==View::Themes?PLAYER_THEME_COUNT:view==View::Calibration?10:view==View::Settings?2:view==View::Display?2:view==View::Appearance?1:view==View::Home?12:view==View::Actions?(actionQueuePos>=0?3:2):rows.size();if(count)selected=(selected+n%count+count)%count;}
  uiDirty=true;
}
uint16_t* previousFrame=nullptr;
bool frameValid=false;



void uiText(int x,int baseline,const String&s,int width,uint16_t color=WHITE,bool bold=false,uint16_t bg=BLACK){
  strandText(*screen,x,baseline,s,width,color,bg,StrandBody);
}
String listTitle(){
  switch(listKind){case ListKind::Artists:return "Artists";case ListKind::Albums:return "Albums";
    case ListKind::Folders:return "Folders";case ListKind::Playlists:return "Playlists";
    case ListKind::Queue:return "Queue";default:return "All songs";}
}

uint16_t testRgb(int r,int g,int b){return uint16_t(((r>>3)<<11)|((g>>2)<<5)|(b>>3));}
void calibrationText(int x,int y,const String&s,int width,const StrandFont&font=StrandSmall){strandText(*screen,x,y,s,width,0xFFFF,0,font);}
void drawCalibrationPage(){
 screen->fillScreen(0);
 calibrationText(14,20,"DISPLAY / LIVE TEST",292);
 if(view==View::ColorEdit){calibrationText(14,49,calibrationName(calibrationParameter),180,StrandBody);calibrationText(200,58,String(calibrationValue(calibrationParameter))+"%",110,StrandTitle);}
 else calibrationText(14,52,"Test patterns",292,StrandBody);
 const char*names[]={"Palette + gray","Grayscale / near black","RGB channel ramps"};
 calibrationText(14,74,names[calibrationPattern],292);
 if(calibrationPattern==0){
  const uint16_t colors[]={0xF800,0x07E0,0x001F,0x07FF,0xF81F,0xFFE0,0xFFFF,0x0000};
  const char*labels[]={"R","G","B","C","M","Y","W","K"};
  for(int i=0;i<8;i++){int x=16+i*36;screen->fillRect(x,83,36,43,colors[i]);calibrationText(x+12,141,labels[i],23);}
  for(int i=0;i<16;i++)screen->fillRect(16+i*18,151,18,28,testRgb(i*17,i*17,i*17));
  calibrationText(16,196,"0",40);calibrationText(108,196,"85",40);calibrationText(194,196,"170",40);calibrationText(280,196,"255",31);
 }else if(calibrationPattern==1){
  for(int i=0;i<16;i++)screen->fillRect(16+i*18,83,18,44,testRgb(i*17,i*17,i*17));
  calibrationText(16,142,"0",40);calibrationText(108,142,"85",40);calibrationText(194,142,"170",40);calibrationText(280,142,"255",31);
  const int levels[]={0,8,16,24,32,48};
  for(int i=0;i<6;i++){int x=16+i*48;screen->fillRect(x,153,48,28,testRgb(levels[i],levels[i],levels[i]));calibrationText(x+13,198,String(levels[i]),33);}
 }else{
  for(int channel=0;channel<3;channel++){
   int y=83+channel*39;calibrationText(14,y+22,channel==0?"R":channel==1?"G":"B",22,StrandBody);
   for(int x=0;x<266;x++){int v=x*255/265;screen->drawFastVLine(40+x,y,30,testRgb(channel==0?v:0,channel==1?v:0,channel==2?v:0));}
  }
 }
 if(view==View::ColorEdit){calibrationText(14,219,"WHEEL: ADJUST  RIGHT: PATTERN",292);calibrationText(14,236,"LEFT: CANCEL  CENTER: ACCEPT",292);}
 else{calibrationText(14,220,"WHEEL / CENTER: NEXT PATTERN",292);calibrationText(14,236,"LEFT: COLOR MENU",292);}
}

void render(){
  const uint32_t frameStarted=millis();
  State s=snapshot();
  // Twenty selected themes. Audio analysis is published independently of UI rendering.
  int queueCount;{StateLock lock;queueCount=playQueue.size();}
  screen->fillScreen(BLACK);
    ThemeFrame frame;
    frame.title=s.track>=0&&s.track<int(tracks.size())?tracks[s.track].title:"Your music is ready";
    frame.artist=s.track>=0&&s.track<int(tracks.size())?tracks[s.track].artist:"UP: browse songs";
    frame.album=s.track>=0&&s.track<int(tracks.size())?tracks[s.track].album:String(tracks.size())+" songs on SD";
    frame.elapsed=s.elapsed;frame.duration=s.duration;frame.rate=s.rate;frame.volume=s.volume;
    frame.queuePos=s.queuePos;frame.queueCount=queueCount;frame.left=s.vuL;frame.right=s.vuR;frame.playing=s.playing;
    frame.trackId=s.track;frame.scope=s.scope;
  if(view==View::Now){
    drawPlayerTheme(*screen,activeTheme,frame);
  }else if(view==View::ColorEdit||view==View::ColorTest){
    drawCalibrationPage();
  }else if(view==View::Brightness){
    drawThemeBrightness(*screen,activeTheme,frame,displayBrightness,brightnessPwm);
  }else{
    std::vector<String> generated;
    if(view==View::Home)generated={"Now playing","All songs","Artists","Albums","Folders","Playlists","Queue",
      String("Shuffle   ")+(s.shuffle?"On":"Off"),String("Repeat   ")+(s.repeat==0?"Off":s.repeat==1?"Queue":"Song"),
      "Settings","Save queue as playlist","Stop playback"};
    else if(view==View::Settings)generated={"Appearance","Display"};
    else if(view==View::Display)generated={"Brightness","Color calibration"};
    else if(view==View::Calibration){generated.push_back("Test palette / grayscale");for(int i=0;i<6;i++)generated.push_back(String(calibrationName(i))+"  "+String(calibrationValue(i))+"%");generated.push_back("Save & exit");generated.push_back("Cancel changes");generated.push_back("Reset preview to defaults");}
    else if(view==View::Appearance)generated={"Themes"};
    else if(view==View::Themes){for(int i=0;i<PLAYER_THEME_COUNT;i++)generated.push_back(String(playerThemeNames[i]));}
    else if(view==View::Actions){generated={"Add to queue","Play next"};if(actionQueuePos>=0)generated.push_back("Remove from queue");}
    // Keep large SD libraries by reference while animating selection/metadata.
    const std::vector<String>& display=view==View::List?rows:generated;
    String heading=view==View::Calibration?"Color calibration":view==View::Display?"Display":view==View::Settings?"Settings":view==View::Appearance?"Appearance":view==View::Themes?"Themes":view==View::Home?"Library":view==View::Actions?"Song options":groupChild?groupName:listTitle();
    const int visible=themeMenuRows(activeTheme);
    if(activeTheme==12)listTop=display.empty()?0:(selected/visible)*visible;
    else{
      if(selected<listTop)listTop=selected;
      if(selected>=listTop+visible)listTop=selected-visible+1;
      listTop=std::max(0,std::min(listTop,std::max(0,int(display.size())-visible)));
    }
    ThemeMenuFrame menu;menu.heading=heading;menu.items=display.data();menu.count=int(display.size());menu.selected=selected;menu.top=listTop;
    menu.kind=view==View::Home?ThemeMenuKind::Library:view==View::List?ThemeMenuKind::List:view==View::Settings?ThemeMenuKind::Settings:view==View::Appearance?ThemeMenuKind::Appearance:view==View::Themes?ThemeMenuKind::Themes:view==View::Display?ThemeMenuKind::Display:view==View::Calibration?ThemeMenuKind::Calibration:ThemeMenuKind::Actions;
    menu.emptyMessage=listKind==ListKind::Playlists?"Add M3U playlists to SD":"Nothing here yet";
    if(view==View::Themes)menu.appliedIndex=activeTheme;
    if(view==View::List&&!groupList){
      for(int row=0;row<visible&&listTop+row<int(rowTracks.size());row++){
        int i=listTop+row,track=rowTracks[i];if(track>=0&&track<int(tracks.size()))menu.secondary[row]=tracks[track].artist;
        if(track==s.track)menu.playingIndex=i;
      }
    }
    drawThemeMenu(*screen,activeTheme,frame,menu);
  }
  bool showToast=s.messageAt&&millis()-s.messageAt<2200&&String(s.message)!="Playing MP3";
  if(showToast&&view!=View::ColorEdit&&view!=View::ColorTest)drawThemeToast(*screen,activeTheme,s.message);
  // Find changed horizontal spans within each 8-row band; use one SPI write per span.
  // A fixed internal buffer keeps transfers independent of framebuffer PSRAM layout.
  static uint16_t transferPixels[320*8];
  if(calibrationDirty){
    if(!identityCalibration(displayCalibration)&&!calibrationLut)calibrationLut=static_cast<uint16_t*>(heap_caps_malloc(65536*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    calibrationMapped=!identityCalibration(displayCalibration)&&calibrationLut;
    if(calibrationMapped)buildCalibrationLut(displayCalibration,calibrationLut);
    if(!identityCalibration(displayCalibration)&&!calibrationLut)Serial.println("Calibration LUT allocation failed; raw colors retained.");
    calibrationDirty=false;frameValid=false;
  }
  uint32_t sentPixels=0,transferAt=micros();
  for(int y=0;y<240;y+=8){
    uint16_t*p=screen->getBuffer()+y*320;
    if(previousFrame&&frameValid&&!memcmp(p,previousFrame+y*320,320*8*2))continue;
    int left=0,right=319;
    if(previousFrame&&frameValid){left=320;right=-1;for(int row=0;row<8;row++){
      uint16_t*old=previousFrame+(y+row)*320,*now=p+row*320;int l=0,r=319;
      while(l<320&&old[l]==now[l])l++;while(r>=l&&old[r]==now[r])r--;
      if(l<left)left=l;if(r>right)right=r;
    }}
    if(right<left)continue;int width=right-left+1;
    for(int row=0;row<8;row++)memcpy(transferPixels+row*width,p+row*320+left,width*2);
    if(calibrationMapped)for(int i=0;i<width*8;i++)transferPixels[i]=calibrationLut[transferPixels[i]];
    {SpiLock lock;tft.startWrite();tft.setAddrWindow(left,y,width,8);tft.writePixels(transferPixels,width*8,true);tft.endWrite();}
    if(previousFrame)memcpy(previousFrame+y*320,p,320*8*2);
    sentPixels+=width*8;taskYIELD();
  }
  static uint32_t profileAt=0,profileFrames=0;profileFrames++;if(millis()-profileAt>5000){uint32_t period=millis()-profileAt;Serial.printf("UI: %lu fps; SPI %lu pixels / %lu us\n",(unsigned long)(profileFrames*1000/period),(unsigned long)sentPixels,(unsigned long)(micros()-transferAt));profileFrames=0;profileAt=millis();}
  frameValid=true;lastRevision=s.revision;uiDirty=false;lastUi=frameStarted;
}
struct Input{int pin;bool raw=true,stable=true,held=false;uint32_t changed=0,pressed=0;};
Input inputs[]={{Pins::CENTER},{Pins::DOWN},{Pins::RIGHT},{Pins::UP},{Pins::LEFT}};
void shortPress(int i){
  if(view==View::ColorEdit&&i==2){calibrationPattern=(calibrationPattern+1)%3;uiDirty=true;return;}
  if(i==0){activate();return;}
  if(view==View::Now){if(i==1)buildList(ListKind::Queue);if(i==2)send(Action::Next);if(i==3)buildList(ListKind::Tracks);if(i==4)send(Action::Previous);}
  else if(i==1)moveSelection(1);else if(i==3)moveSelection(-1);else if(i==2)activate();else{
    if(view==View::ColorEdit){displayCalibration=parameterEntry;calibrationDirty=true;view=View::Calibration;selected=calibrationReturnRow;}
    else if(view==View::ColorTest){view=View::Calibration;selected=0;listTop=0;}
    else if(view==View::Calibration){leaveCalibration(false);}
    else if(view==View::Brightness){setDisplayBrightness(brightnessEntry);view=View::Display;selected=0;listTop=0;}
    else if(view==View::Display){view=View::Settings;selected=1;listTop=0;}
    else if(view==View::Themes){view=View::Appearance;selected=0;listTop=0;}
    else if(view==View::Appearance){view=View::Settings;selected=0;listTop=0;}
    else if(view==View::Settings){view=View::Home;selected=9;listTop=5;}
    else if(view==View::Actions){view=View::List;selected=actionRow;}
    else if(view==View::Home){view=View::Now;}
    else if(groupChild){int restore=parentRow;buildList(listKind);selected=std::min(restore,std::max(0,int(rows.size())-1));}
    else{view=View::Home;selected=listKind==ListKind::Tracks?1:listKind==ListKind::Artists?2:listKind==ListKind::Albums?3:listKind==ListKind::Folders?4:listKind==ListKind::Playlists?5:6;}
    uiDirty=true;
  }
}
void longPress(int i){
  if(i==0&&calibrationActive){leaveCalibration(false);return;}
  if(i==0&&view==View::Brightness){setDisplayBrightness(brightnessEntry);view=View::Display;selected=0;uiDirty=true;return;}
  if(i==0){if(view==View::List&&!groupList&&selected<int(rowTracks.size())){actionRow=selected;actionTrack=rowTracks[selected];actionQueuePos=listKind==ListKind::Queue?selected:-1;view=View::Actions;selected=0;}else{view=View::Home;selected=0;}uiDirty=true;}
  else if(view==View::Now&&(i==2||i==4))send(Action::Seek,i==2?10:-10);
}
void inputsTask(void*){
  // Poll on a separate task so SD reads and screen transfers cannot lose edges.
  uint8_t old=(digitalRead(Pins::ENC_A)<<1)|digitalRead(Pins::ENC_B);int quarter=0;
  static const int8_t table[16]={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  for(;;){
    uint8_t ab=(digitalRead(Pins::ENC_A)<<1)|digitalRead(Pins::ENC_B);
    if(old!=ab){quarter+=table[(old<<2)|ab];old=ab;if(quarter>=4||quarter<=-4){int event=quarter>0?100:-100;xQueueSend(inputEvents,&event,0);quarter=0;}}
    uint32_t now=millis();
    for(int i=0;i<5;i++){Input&b=inputs[i];bool v=digitalRead(b.pin);if(v!=b.raw){b.raw=v;b.changed=now;}
      if(v!=b.stable&&now-b.changed>=25){b.stable=v;if(!v){b.pressed=now;b.held=false;}else if(!b.held){int event=i+1;xQueueSend(inputEvents,&event,0);}}
      if(!b.stable&&!b.held&&now-b.pressed>=650){b.held=true;int event=i+11;xQueueSend(inputEvents,&event,0);}}
    delay(1);
  }
}
void setup(){
  Serial.begin(115200);delay(200);loadAppearance();
  spiMutex=xSemaphoreCreateMutex();stateMutex=xSemaphoreCreateMutex();commands=xQueueCreate(16,sizeof(Command));inputEvents=xQueueCreate(32,sizeof(int));
  if(!spiMutex||!stateMutex||!commands||!inputEvents)while(true)delay(100);
  pinMode(Pins::TFT_CS,OUTPUT);digitalWrite(Pins::TFT_CS,HIGH);pinMode(Pins::SD_CS,OUTPUT);digitalWrite(Pins::SD_CS,HIGH);
  initBacklight();
  SPI.begin(Pins::SCK,Pins::MISO,Pins::MOSI,Pins::TFT_CS);
  tft.init(240,320);tft.setRotation(1);tft.setSPISpeed(TFT_SPEED);tft.fillScreen(BLACK);tft.setTextColor(CYAN);tft.setCursor(8,20);tft.print("PLAYER / STARTING");
  if(!psramFound())fail("PSRAM missing: select OPI PSRAM and restart.");
  heap_caps_malloc_extmem_enable(1024);
  previousFrame=static_cast<uint16_t*>(heap_caps_malloc(320*240*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  screen=new GFXcanvas16(320,240);if(!screen||!screen->getBuffer())fail("Screen buffer allocation failed");
  if(!SD.begin(Pins::SD_CS,SPI,SD_SPEED))fail("SD failed. Check wiring and FAT32 card.");
  Serial.println("STRAND / 20 selected themes / PCM5102A / MP3");
  tft.setCursor(8,45);tft.print("Indexing MP3 files... see Serial");scanMusic();
  for(size_t i=0;i<tracks.size();i++)playQueue.push_back(i);
  for(auto&b:inputs){pinMode(b.pin,INPUT_PULLUP);b.raw=b.stable=digitalRead(b.pin);}pinMode(Pins::ENC_A,INPUT_PULLUP);pinMode(Pins::ENC_B,INPUT_PULLUP);
  if(xTaskCreatePinnedToCore(audioTask,"audio",32768,nullptr,3,nullptr,0)!=pdPASS)fail("Audio task allocation failed");
  if(xTaskCreatePinnedToCore(inputsTask,"inputs",4096,nullptr,2,nullptr,1)!=pdPASS)fail("Input task allocation failed");
  message(String(tracks.size())+" MP3 tracks. UP: browse");
  render();
}
void loop(){
  int event;while(xQueueReceive(inputEvents,&event,0)==pdTRUE){if(event==100||event==-100)moveSelection(event>0?1:-1);else if(event>=11)longPress(event-11);else shortPress(event-1);}
  State s=snapshot();if((uiDirty||s.revision!=lastRevision||(view!=View::ColorEdit&&view!=View::ColorTest)||s.messageAt&&millis()-s.messageAt<2500)&&millis()-lastUi>=30)render();
  delay(1);
}









