#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
constexpr size_t STRAND_SCOPE_BINS=92;
struct PcmScope {
 int8_t lowL[STRAND_SCOPE_BINS]={},highL[STRAND_SCOPE_BINS]={};
 int8_t lowR[STRAND_SCOPE_BINS]={},highR[STRAND_SCOPE_BINS]={};
 uint8_t energyL[STRAND_SCOPE_BINS]={},energyR[STRAND_SCOPE_BINS]={};
 uint8_t rmsL=0,rmsR=0,peakL=0,peakR=0;
 uint32_t serial=0;
};
// Streaming 10ms bins from EVERY decoded block. Chronological history spans 0.92s.
// No allocations, SD, UI, or modification of the audio samples in this analyzer.
class PcmAnalyzer {
 PcmScope ring{};
 uint32_t sampleRate=0,hop=1,count=0,write=0;
 int lo[2]={32767,32767},hi[2]={-32768,-32768};
 uint64_t sum[2]={};
 void finish(){
  int8_t* lows[2]={ring.lowL,ring.lowR};int8_t* highs[2]={ring.highL,ring.highR};
  uint8_t* energies[2]={ring.energyL,ring.energyR};
  for(int c=0;c<2;c++){
   lows[c][write]=int8_t(std::max(-128,std::min(127,lo[c]/256)));
   highs[c][write]=int8_t(std::max(-128,std::min(127,hi[c]/256)));
   unsigned rms=unsigned(sqrt(double(sum[c])/count)*100.0/32768.0+.5);
   unsigned peak=unsigned(std::max(abs(lo[c]),abs(hi[c]))*100U/32768U);
   energies[c][write]=uint8_t(std::min(100U,rms));
   if(c){ring.rmsR=energies[c][write];ring.peakR=uint8_t(std::min(100U,peak));}
   else{ring.rmsL=energies[c][write];ring.peakL=uint8_t(std::min(100U,peak));}
   sum[c]=0;lo[c]=32767;hi[c]=-32768;
  }
  count=0;write=(write+1)%STRAND_SCOPE_BINS;ring.serial++;
 }
public:
 void reset(){ring=PcmScope{};sampleRate=0;hop=1;count=write=0;sum[0]=sum[1]=0;lo[0]=lo[1]=32767;hi[0]=hi[1]=-32768;}
 void push(const int32_t*pcm,size_t frames,uint32_t rate){
  if(!pcm||!rate)return;
  if(rate!=sampleRate){reset();sampleRate=rate;hop=std::max<uint32_t>(1,rate/100);}
  for(size_t i=0;i<frames;i++){
   for(int c=0;c<2;c++){int v=pcm[i*2+c]/65536;lo[c]=std::min(lo[c],v);hi[c]=std::max(hi[c],v);sum[c]+=uint64_t(int64_t(v)*v);}
   if(++count>=hop)finish();
  }
 }
 void snapshot(PcmScope&out)const{
  out=ring;
  for(size_t i=0;i<STRAND_SCOPE_BINS;i++){
   size_t j=(write+i)%STRAND_SCOPE_BINS;
   out.lowL[i]=ring.lowL[j];out.highL[i]=ring.highL[j];out.lowR[i]=ring.lowR[j];out.highR[i]=ring.highR[j];
   out.energyL[i]=ring.energyL[j];out.energyR[i]=ring.energyR[j];
  }
 }
};
// Read-only min/max envelope of actual decoded stereo PCM, before volume scaling.
inline void capturePcmScope(const int32_t*pcm,size_t frames,PcmScope&out){
 if(!frames){out=PcmScope{};return;}
 for(size_t bin=0;bin<STRAND_SCOPE_BINS;bin++){
  size_t start=bin*frames/STRAND_SCOPE_BINS,end=(bin+1)*frames/STRAND_SCOPE_BINS;
  if(end<=start)end=start+1;if(end>frames)end=frames;
  int loL=127,hiL=-128,loR=127,hiR=-128;
  for(size_t i=start;i<end;i++){int l=pcm[i*2]/16777216,r=pcm[i*2+1]/16777216;if(l<loL)loL=l;if(l>hiL)hiL=l;if(r<loR)loR=r;if(r>hiR)hiR=r;}
  out.lowL[bin]=int8_t(loL);out.highL[bin]=int8_t(hiL);out.lowR[bin]=int8_t(loR);out.highR[bin]=int8_t(hiR);
 }
}
