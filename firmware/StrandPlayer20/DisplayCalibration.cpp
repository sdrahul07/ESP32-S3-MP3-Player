#include "DisplayCalibration.h"
#include <math.h>
#include <algorithm>
bool validCalibration(const DisplayCalibration&c){return c.red>=50&&c.red<=150&&c.green>=50&&c.green<=150&&c.blue>=50&&c.blue<=150&&c.gamma>=50&&c.gamma<=200&&c.saturation>=0&&c.saturation<=150&&c.contrast>=50&&c.contrast<=150;}
bool identityCalibration(const DisplayCalibration&c){return c.red==100&&c.green==100&&c.blue==100&&c.gamma==100&&c.saturation==100&&c.contrast==100;}
static float clamp(float x){return std::max(0.f,std::min(255.f,x));}
void buildCalibrationLut(const DisplayCalibration&input,uint16_t*lut){
 const DisplayCalibration c=validCalibration(input)?input:DisplayCalibration{};
 if(identityCalibration(c)){for(unsigned i=0;i<65536;i++)lut[i]=uint16_t(i);return;}
 float curve[3][256];int gains[]={c.red,c.green,c.blue};
 for(int channel=0;channel<3;channel++)for(int v=0;v<256;v++){
  float contrasted=(v-127.5f)*(c.contrast/100.f)+127.5f;
  curve[channel][v]=255.f*powf(clamp(contrasted*gains[channel]/100.f)/255.f,100.f/c.gamma);
 }
 for(unsigned pixel=0;pixel<65536;pixel++){
  float r=((pixel>>11)&31)*255.f/31.f,g=((pixel>>5)&63)*255.f/63.f,b=(pixel&31)*255.f/31.f;
  float luma=(77*r+150*g+29*b)/256.f,sat=c.saturation/100.f;
  int ri=int(clamp(luma+(r-luma)*sat)+.5f),gi=int(clamp(luma+(g-luma)*sat)+.5f),bi=int(clamp(luma+(b-luma)*sat)+.5f);
  unsigned R=unsigned(curve[0][ri]+.5f),G=unsigned(curve[1][gi]+.5f),B=unsigned(curve[2][bi]+.5f);
  lut[pixel]=uint16_t(((R>>3)<<11)|((G>>2)<<5)|(B>>3));
 }
}
