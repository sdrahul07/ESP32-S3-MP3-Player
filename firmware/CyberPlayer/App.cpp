#include <Arduino.h>
#include <SPI.h>

#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include <JPEGDEC.h>
#include <driver/i2s_std.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <memory>
#include <math.h>
#include "Config.h"
#include "Media.h"
#include "Decoder.h"

// MP3 player release candidate. See README for validation and limitations.
SemaphoreHandle_t spiMutex=nullptr;
SemaphoreHandle_t stateMutex=nullptr;
QueueHandle_t commands=nullptr;
QueueHandle_t inputEvents=nullptr;
Adafruit_ST7789 tft(&SPI,Pins::TFT_CS,Pins::TFT_DC,Pins::TFT_RST);

GFXcanvas16* screen=nullptr;
JPEGDEC jpeg;
i2s_chan_handle_t tx=nullptr;
constexpr uint16_t BLACK=0x0000,CYAN=0x07FF,WHITE=0xFFFF,AMBER=0xFD20,DIM=0x52AA,PANEL=0x0841;
enum class Action:uint8_t{Play,Toggle,Next,Previous,Seek,Stop};
struct Command{Action action;int value;};
struct State{
  int track=-1,queuePos=-1,volume=12,repeat=0;
  bool playing=false,shuffle=false;
  uint32_t elapsed=0,duration=0,rate=0;
  uint8_t vuL=0,vuR=0;
  uint32_t revision=0;
  char message[80]="Ready: Up opens library";
};
State state;
std::vector<int> playQueue;
uint32_t queueRevision=0;
struct StateLock{StateLock(){xSemaphoreTake(stateMutex,portMAX_DELAY);}~StateLock(){xSemaphoreGive(stateMutex);}};
State snapshot(){StateLock lock;return state;}
void message(const String&s){StateLock lock;s.toCharArray(state.message,sizeof(state.message));state.revision++;Serial.println(s);}
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
  bool paused=false;float gain=0;
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
          if(!active->seek(std::max<int64_t>(0,p)))message("Seek failed");prepared.reset();}
        continue;
      }
      if(cmd.action==Action::Stop){active.reset();prepared.reset();StateLock lock;state.playing=false;state.vuL=state.vuR=0;state.revision++;continue;}
      int target=cmd.action==Action::Play?cmd.value:nextPosition(activePos,cmd.action==Action::Previous,false);
      active.reset();prepared.reset();paused=false;
      int index=queueTrack(target);active=openDecoder(index);
      if(!active){message("Cannot decode MP3 / unsupported rate");StateLock lock;state.playing=false;continue;}
      if(clockRate!=active->rate){if(!setupAudioClock(active->rate)){active.reset();message("DAC/I2S clock setup failed");continue;}clockRate=active->rate;}
      // PCM5102A automatically derives its system clock from BCK.
      activePos=target;gain=0;
      {StateLock lock;state.track=index;state.queuePos=target;state.playing=true;state.rate=active->rate;state.elapsed=0;state.duration=active->frames/active->rate;state.revision++;}
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
      activePos=next;
      int index=queueTrack(next);
      {StateLock lock;state.track=index;state.queuePos=next;state.elapsed=0;
        state.rate=active->rate;state.duration=active->frames/active->rate;state.revision++;}
      continue;
    }
    State s=snapshot();float target=s.volume==0?0:powf(10.0f,(-60.0f+0.6f*s.volume)/20.0f);
    uint32_t peakL=0,peakR=0;
    for(size_t i=0;i<n;i++){
      gain+=(target-gain)*0.002f;
      for(int c=0;c<2;c++){int32_t v=int32_t(float(pcm[2*i+c])*gain);pcm[2*i+c]=v;uint32_t level=uint32_t(llabs(int64_t(v)))>>23;if(c)peakR=std::max(peakR,level);else peakL=std::max(peakL,level);}
    }
    size_t bytes=n*8,offset=0;
    while(offset<bytes){size_t written=0;esp_err_t err=i2s_channel_write(tx,reinterpret_cast<uint8_t*>(pcm)+offset,bytes-offset,&written,200);if(err!=ESP_OK||!written){message("I2S output timeout");break;}offset+=written;}
    if(millis()-lastState>=80){StateLock lock;state.elapsed=active->position/active->rate;state.vuL=std::min<uint32_t>(100,peakL*100/255);state.vuR=std::min<uint32_t>(100,peakR*100/255);lastState=millis();}
    delay(1);
  }
}

// UI/artwork run on the main task; the audio task owns I2S.
enum class View{Now,Home,List,Actions};
View view=View::Now;
enum class ListKind{Tracks,Artists,Albums,Folders,Playlists,Queue};
ListKind listKind=ListKind::Tracks;
std::vector<String> rows;
std::vector<int> rowTracks;
int selected=0,actionTrack=-1,actionQueuePos=-1;bool groupList=false;
bool uiDirty=true;uint32_t lastUi=0,lastRevision=0;
uint16_t artwork[80*80]={};int artTrack=-2;
int artW=0,artH=0;
int jpegDraw(JPEGDRAW*d){
  for(int y=0;y<d->iHeight;y++)for(int x=0;x<d->iWidth;x++){
    int dx=(d->x+x)*80/artW,dy=(d->y+y)*80/artH;
    if(dx>=0&&dx<80&&dy>=0&&dy<80)artwork[dy*80+dx]=d->pPixels[y*d->iWidth+x];
  }return 1;
}
void loadArtwork(int index){
  artTrack=index;memset(artwork,0,sizeof(artwork));if(index<0||index>=int(tracks.size()))return;
  const Track&t=tracks[index];if(!t.artBytes)return;
  uint8_t* data=static_cast<uint8_t*>(heap_caps_malloc(t.artBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(!data)return;
  File f;{SpiLock lock;f=SD.open(t.path);if(f)f.seek(t.artOffset);}
  bool ok=bool(f);size_t done=0;
  while(ok&&done<t.artBytes){size_t n=std::min<size_t>(4096,t.artBytes-done);int got;
    {SpiLock lock;got=f.read(data+done,n);}if(got!=int(n))ok=false;else done+=n;delay(1);}
  {SpiLock lock;f.close();}
  if(ok&&jpeg.openRAM(data,t.artBytes,jpegDraw)){
    int divisor=1,opt=0;while(divisor<8&&std::max(jpeg.getWidth(),jpeg.getHeight())/divisor>320)divisor*=2;
    if(divisor==2)opt=JPEG_SCALE_HALF;else if(divisor==4)opt=JPEG_SCALE_QUARTER;else if(divisor==8)opt=JPEG_SCALE_EIGHTH;
    artW=std::max(1,(jpeg.getWidth()+divisor-1)/divisor);artH=std::max(1,(jpeg.getHeight()+divisor-1)/divisor);
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);jpeg.decode(0,0,opt);jpeg.close();
  }free(data);
}
void textAt(int x,int y,const String&s,uint16_t color=CYAN,int size=1,int maxChars=49){screen->setTextSize(size);screen->setTextColor(color);screen->setCursor(x,y);screen->print(s.substring(0,maxChars));}
String timeText(uint32_t s){char b[20];snprintf(b,sizeof(b),"%lu:%02lu",(unsigned long)(s/60),(unsigned long)(s%60));return b;}
void buildList(ListKind kind){
  listKind=kind;rows.clear();rowTracks.clear();selected=0;groupList=false;
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
  if(groupList){String group=rows[selected];rows.clear();rowTracks.clear();
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
    case 9:message("PCM5102A line out: connect external amplifier");break;
    case 10:{std::vector<int> items;{StateLock lock;items=playQueue;}String p=savePlaylist(items);if(p.length()){playlists.push_back(p);message("Saved "+p);}else message("Could not save queue");}break;
    case 11:send(Action::Stop);break;
  }uiDirty=true;
}
void activate(){if(view==View::Now){State s=snapshot();if(s.track<0)buildList(ListKind::Tracks);else send(Action::Toggle);}
  else if(view==View::Home)homeSelect();else if(view==View::List)selectRow();
  else if(view==View::Actions){
    if(selected<2&&actionTrack>=0){StateLock lock;
      if(selected==0)playQueue.push_back(actionTrack);else {int pos=std::min<int>(playQueue.size(),std::max(0,state.queuePos+1));playQueue.insert(playQueue.begin()+pos,actionTrack);}queueRevision++;}
    else if(selected==2&&listKind==ListKind::Queue){State s=snapshot();if(s.playing)message("Pause/stop before removing queue items");else{send(Action::Stop);StateLock lock;if(actionQueuePos>=0&&actionQueuePos<int(playQueue.size())){playQueue.erase(playQueue.begin()+actionQueuePos);queueRevision++;}}}
    view=View::Home;selected=6;uiDirty=true;
  }
}
void moveSelection(int n){
  if(view==View::Now){StateLock lock;state.volume=std::max(0,std::min(100,state.volume+n));state.revision++;}
  else {int count=view==View::Home?12:view==View::Actions?3:rows.size();if(count)selected=(selected+n%count+count)%count;}
  uiDirty=true;
}
void render(){
  State s=snapshot();if(view==View::Now&&artTrack!=s.track)loadArtwork(s.track);
  screen->fillScreen(BLACK);textAt(8,6,"CYBERPLAYER // MP3",CYAN);textAt(230,6,"LINE OUT",AMBER,1,14);
  screen->drawFastHLine(8,21,304,DIM);
  if(view==View::Now){
    screen->drawRect(7,31,82,82,DIM);screen->drawRGBBitmap(8,32,artwork,80,80);
    if(s.track>=0){const Track&t=tracks[s.track];textAt(99,33,t.title,WHITE,1,35);textAt(99,52,t.artist,CYAN,1,35);textAt(99,71,t.album,DIM,1,35);}
    else {textAt(99,33,"READY",WHITE);textAt(99,52,"UP: browse music",CYAN);}
    textAt(99,94,"MP3 / "+String(s.rate/1000.0f,1)+" kHz",AMBER,1,30);
    textAt(8,126,"L");screen->fillRect(24,126,280,7,PANEL);screen->fillRect(24,126,s.vuL*280/100,7,CYAN);
    textAt(8,141,"R");screen->fillRect(24,141,280,7,PANEL);screen->fillRect(24,141,s.vuR*280/100,7,CYAN);
    textAt(8,160,timeText(s.elapsed),WHITE);textAt(268,160,s.duration?timeText(s.duration):"--:--",WHITE,1,8);
    screen->fillRect(8,178,304,3,DIM);if(s.duration)screen->fillRect(8,178,std::min<uint32_t>(304,s.elapsed*304/s.duration),3,CYAN);
    textAt(8,193,s.playing?"PLAYING":"PAUSED",AMBER);textAt(100,193,"VOL "+String(s.volume));
    textAt(206,193,s.shuffle?"SHUFFLE":"ORDERED",DIM);
    textAt(8,213,s.message,DIM,1,49);
  }else{
    std::vector<String> display;
    if(view==View::Home)display={"Now playing","All tracks","Artists","Albums","Folders","Playlists","Queue",String("Shuffle: ")+(s.shuffle?"ON":"OFF"),String("Repeat: ")+(s.repeat==0?"OFF":s.repeat==1?"QUEUE":"TRACK"),"Output: PCM5102A line out","Save queue as playlist","Stop playback"};
    else if(view==View::Actions)display={"Add to queue","Play next","Remove queue item (stopped)"};
    else display=rows;
    textAt(8,29,view==View::Home?"SYSTEM MENU":view==View::Actions?"TRACK ACTIONS":"MUSIC LIBRARY",AMBER);
    if(display.empty())textAt(8,65,"No items. LEFT: menu",DIM);
    int top=selected/7*7;
    for(int i=top;i<int(display.size())&&i<top+7;i++){int y=50+(i-top)*22;
      if(i==selected)screen->fillRect(5,y-3,310,18,DIM);textAt(10,y,display[i],i==selected?BLACK:CYAN,1,49);}
    textAt(8,216,"CENTER select | hold CENTER menu",DIM);
  }
  // A full frame is composed in RAM. Transfer short chunks, yielding SPI to SD.
  for(int y=0;y<240;y+=4){{SpiLock lock;tft.drawRGBBitmap(0,y,screen->getBuffer()+y*320,320,4);}delay(1);}
  lastRevision=s.revision;uiDirty=false;lastUi=millis();
}
struct Input{int pin;bool raw=true,stable=true,held=false;uint32_t changed=0,pressed=0;};
Input inputs[]={{Pins::CENTER},{Pins::DOWN},{Pins::RIGHT},{Pins::UP},{Pins::LEFT}};
void shortPress(int i){
  if(i==0){activate();return;}
  if(view==View::Now){if(i==1)buildList(ListKind::Queue);if(i==2)send(Action::Next);if(i==3)buildList(ListKind::Tracks);if(i==4)send(Action::Previous);}
  else if(i==1)moveSelection(1);else if(i==3)moveSelection(-1);else if(i==2)activate();else{view=View::Home;selected=0;uiDirty=true;}
}
void longPress(int i){
  if(i==0){if(view==View::List&&!groupList&&selected<int(rowTracks.size())){actionTrack=rowTracks[selected];actionQueuePos=listKind==ListKind::Queue?selected:-1;view=View::Actions;selected=0;}else{view=View::Home;selected=0;}uiDirty=true;}
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
  Serial.begin(115200);delay(200);
  spiMutex=xSemaphoreCreateMutex();stateMutex=xSemaphoreCreateMutex();commands=xQueueCreate(16,sizeof(Command));inputEvents=xQueueCreate(32,sizeof(int));
  if(!spiMutex||!stateMutex||!commands||!inputEvents)while(true)delay(100);
  pinMode(Pins::TFT_CS,OUTPUT);digitalWrite(Pins::TFT_CS,HIGH);pinMode(Pins::SD_CS,OUTPUT);digitalWrite(Pins::SD_CS,HIGH);
  pinMode(Pins::BL,OUTPUT);digitalWrite(Pins::BL,HIGH);
  SPI.begin(Pins::SCK,Pins::MISO,Pins::MOSI,Pins::TFT_CS);
  tft.init(240,320);tft.setRotation(1);tft.setSPISpeed(TFT_SPEED);tft.fillScreen(BLACK);tft.setTextColor(CYAN);tft.setCursor(8,20);tft.print("CYBERPLAYER // BOOT");
  if(!psramFound())fail("PSRAM missing: select OPI PSRAM and restart.");
  heap_caps_malloc_extmem_enable(1024);
  screen=new GFXcanvas16(320,240);if(!screen||!screen->getBuffer())fail("Screen buffer allocation failed");
  if(!SD.begin(Pins::SD_CS,SPI,SD_SPEED))fail("SD failed. Check wiring and FAT32 card.");
  Serial.println("PCM5102A build v1: no I2C detection required.");
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
  State s=snapshot();if((uiDirty||s.revision!=lastRevision||view==View::Now&&s.playing)&&millis()-lastUi>=120)render();
  delay(1);
}


