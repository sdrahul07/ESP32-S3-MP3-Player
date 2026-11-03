#include "Config.h"
#include "Media.h"
#include <algorithm>
std::vector<Track> tracks;
std::vector<String> playlists;
static uint32_t be32(const uint8_t*p){return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3];}
static uint32_t sync32(const uint8_t*p){return uint32_t(p[0]&127)<<21|uint32_t(p[1]&127)<<14|uint32_t(p[2]&127)<<7|(p[3]&127);}
static bool readAt(File&f,uint32_t pos,uint8_t*p,size_t n){return uint64_t(pos)+n<=f.size()&&f.seek(pos)&&f.read(p,n)==int(n);}
static bool jpegAt(File&f,uint32_t pos){uint8_t p[2];return readAt(f,pos,p,2)&&p[0]==255&&p[1]==216;}
static String text(File&f,uint32_t pos,uint32_t len,bool id3){
  uint8_t p[512];len=std::min<uint32_t>(len,sizeof(p));
  if(!readAt(f,pos,p,len))return "";
  String s;int enc=id3&&len?p[0]:0;uint32_t i=id3?1:0;
  if(enc==1||enc==2){
    bool little=enc==1;
    if(i+1<len&&p[i]==255&&p[i+1]==254){little=true;i+=2;}
    else if(i+1<len&&p[i]==254&&p[i+1]==255){little=false;i+=2;}
    for(;i+1<len&&s.length()<100;i+=2){uint16_t c=little?(p[i]|p[i+1]<<8):(p[i]<<8|p[i+1]);if(!c)break;s+=c<128?char(c):'?';}
  }else for(;i<len&&p[i]&&s.length()<100;i++)s+=char(p[i]);
  s.trim();return s;
}
static void id3Metadata(File&f,Track&t){
  uint8_t head[10];if(!readAt(f,0,head,10))return;
  if(head[3]!=3&&head[3]!=4)return; // ID3 v2.3/v2.4 only.
  if(head[5]&0x80)return; // Unsynchronised tags need byte remapping, not raw offsets.
  uint32_t pos=10,end=10+sync32(head+6);if(end>f.size())return;
  if(head[5]&0x40){uint8_t b[4];if(!readAt(f,pos,b,4))return;
    uint32_t n=head[3]==4?sync32(b):be32(b)+4;if(n<4||n>end-pos)return;pos+=n;}
  while(end-pos>=10){
    uint8_t h[10];if(!readAt(f,pos,h,10)||!h[0])break;
    uint32_t len=head[3]==4?sync32(h+4):be32(h+4);pos+=10;
    if(len>end-pos)break;
    String id;for(int i=0;i<4;i++)id+=char(h[i]);
    bool unsupported=head[3]==4?(h[9]&0x4F):(h[9]&0xE0);
    if(!unsupported){
      if(id=="TIT2")t.title=text(f,pos,len,true);
      else if(id=="TPE1")t.artist=text(f,pos,len,true);
      else if(id=="TALB")t.album=text(f,pos,len,true);
      else if(id=="APIC"&&len>6){
        uint32_t p=pos,endPic=pos+len;uint8_t enc=0,c=0;
        readAt(f,p++,&enc,1);
        do{if(!readAt(f,p++,&c,1))break;}while(c&&p<endPic);
        uint8_t kind=0;if(p<endPic)readAt(f,p++,&kind,1);
        if(enc==1||enc==2){uint8_t b[2];while(p+1<endPic){if(!readAt(f,p,b,2))break;p+=2;if(!b[0]&&!b[1])break;}}
        else {do{if(p>=endPic||!readAt(f,p++,&c,1))break;}while(c);}
        if(p<endPic&&endPic-p<=MAX_ART_BYTES&&jpegAt(f,p)&&(kind==3||!t.artBytes)){
          t.artOffset=p;t.artBytes=endPic-p;
        }
      }
    }
    pos+=len;
  }
}
static void scanDir(const String&path,int depth){
  if(depth>12||tracks.size()>=MAX_TRACKS)return;
  File dir=SD.open(path);if(!dir||!dir.isDirectory())return;
  while(tracks.size()<MAX_TRACKS){
    File f=dir.openNextFile();if(!f)break;
    String name=f.name();int slash=name.lastIndexOf('/');if(slash>=0)name=name.substring(slash+1);
    String full=path=="/"?"/"+name:path+"/"+name;
    if(name.startsWith(".")){f.close();continue;}
    if(f.isDirectory()){f.close();scanDir(full,depth+1);continue;}
    String lower=name;lower.toLowerCase();
    if(lower.endsWith(".m3u")||lower.endsWith(".m3u8"))playlists.push_back(full);
    if(lower.endsWith(".mp3")){
      Track t;t.path=full;t.title=name.substring(0,name.lastIndexOf('.'));
      t.artist="Unknown artist";t.album="Unknown album";
      uint8_t sig[4];if(readAt(f,0,sig,4)){
        
        if(!memcmp(sig,"ID3",3))id3Metadata(f,t);
      }
      tracks.push_back(t);Serial.printf("Indexed %u: %s\n",unsigned(tracks.size()),full.c_str());
    }
    f.close();delay(1);
  }
  dir.close();
}
void scanMusic(){tracks.clear();playlists.clear();scanDir("/",0);
  std::sort(tracks.begin(),tracks.end(),[](const Track&a,const Track&b){return a.path.compareTo(b.path)<0;});}
bool readPlaylist(const String&path,std::vector<int>&out){
  SpiLock lock;File f=SD.open(path);if(!f)return false;out.clear();
  String base=path.substring(0,path.lastIndexOf('/')+1);
  while(f.available()&&out.size()<MAX_TRACKS){
    String s=f.readStringUntil('\n');s.trim();if(!s.length()||s[0]=='#')continue;
    s.replace('\\','/');if(!s.startsWith("/"))s=base+s;
    for(size_t i=0;i<tracks.size();i++)if(tracks[i].path==s){out.push_back(i);break;}
  }
  f.close();return !out.empty();
}
String savePlaylist(const std::vector<int>&items){
  SpiLock lock;String name;
  for(int i=1;i<10000;i++){name="/CYBERQUEUE-"+String(i)+".m3u";if(!SD.exists(name))break;name="";}
  if(name.isEmpty())return "";
  File f=SD.open(name,FILE_WRITE);if(!f)return "";
  bool ok=f.println("#EXTM3U")>0;
  for(int i:items)if(i>=0&&i<int(tracks.size()))ok=(f.println(tracks[i].path)>0)&&ok;
  f.close();if(!ok)return "";return name;
}

