#include "Config.h"
#include <esp_heap_caps.h>
#define MINIMP3_IMPLEMENTATION
#include "Decoder.h"
static size_t readFile(void*user,void*buffer,size_t size){SpiLock lock;int n=static_cast<File*>(user)->read(static_cast<uint8_t*>(buffer),size);return n>0?size_t(n):0;}
static size_t readMp3(void*b,size_t n,void*u){return readFile(u,b,n);}
static int seekMp3(uint64_t p,void*u){SpiLock lock;File&f=*static_cast<File*>(u);return p<=f.size()&&f.seek(uint32_t(p))?0:-1;}
bool Decoder::open(const String&path){
  {SpiLock lock;file=SD.open(path);}if(!file)return false;
  String lower=path;lower.toLowerCase();
  if(lower.endsWith(".mp3")){
    mp3=static_cast<mp3dec_ex_t*>(calloc(1,sizeof(mp3dec_ex_t)));if(!mp3)return false;
    io={readMp3,&file,seekMp3,&file};
    if(mp3dec_ex_open_cb(mp3,&io,MP3D_SEEK_TO_SAMPLE|MP3D_DO_NOT_SCAN))return false;
    rate=mp3->info.hz;channels=mp3->info.channels;bits=16;
    if(channels)frames=mp3->samples/channels;
  }
  return (channels==1||channels==2)&&(rate==32000||rate==44100||rate==48000);
}
size_t Decoder::read(int32_t*out,size_t count){
  size_t n=0;
  if(mp3){
    int16_t pcm[AUDIO_FRAMES*2];
    n=mp3dec_ex_read(mp3,pcm,count*channels)/channels;
    for(size_t i=0;i<n;i++){out[2*i]=int32_t(pcm[i*channels])*65536;out[2*i+1]=int32_t(pcm[i*channels+channels-1])*65536;}
  }
  position+=n;return n;
}
bool Decoder::seek(uint64_t frame){
  bool ok=mp3&&mp3dec_ex_seek(mp3,frame*channels)==0;
  if(ok)position=frame;return ok;
}
Decoder::~Decoder(){if(mp3){mp3dec_ex_close(mp3);free(mp3);}if(file){SpiLock lock;file.close();}}
