#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cream {
class RandomPan {
 public:
  void reset(){random_=0x70616e31u;phase_=from_=pan_=0.f;to_=next();idle_=true;}
  void tick(const float* in,float* out,float depth,float rate,bool square,bool enabled){
    if(!enabled){if(!idle_)reset();out[0]=in[0];out[1]=in[1];return;}
    idle_=false;
    const float position=square?to_:from_+(to_-from_)*curve(phase_);
    pan_+=.02061782f*(depth*position-pan_);
    const float p=std::max(-1.f,std::min(1.f,pan_));
    const float mid=(in[0]+in[1])*.5f,side=(in[0]-in[1])*.5f*(1.f-std::fabs(p));
    out[0]=mid*gain(-p)+side;out[1]=mid*gain(p)-side;
    phase_+=rate/48000.f;
    if(phase_>=1.f){phase_-=1.f;from_=to_;to_=next();}
  }
 private:
  float next(){random_^=random_<<13;random_^=random_>>17;random_^=random_<<5;return (random_>>8)*(1.f/8388608.f)-1.f;}
  static float curve(float t){
    const float u=2.f*t-1.f,z=u*u;
    const float s=u*(1.570796327f+z*(-.645964098f+z*(.079692626f-.004681754f*z)))*1.000156924f;
    return std::max(0.f,std::min(1.f,.5f+.5f*s));
  }
  static float gain(float p){
    static const float table[33]={
      0.f,.25f,.353553391f,.433012702f,.5f,.559016994f,.612372436f,.661437828f,
      .707106781f,.75f,.790569415f,.829156198f,.866025404f,.901387819f,.935414347f,.968245837f,
      1.f,1.030776406f,1.060660172f,1.089724736f,1.118033989f,1.145643924f,1.17260394f,1.198957881f,
      1.224744871f,1.25f,1.274754878f,1.299038106f,1.322875656f,1.346291202f,1.369306394f,1.391941091f,1.414213562f};
    const float q=(p+1.f)*16.f;const int i=std::min(31,static_cast<int>(q));
    return table[i]+(q-i)*(table[i+1]-table[i]);
  }
  uint32_t random_=0x70616e31u;
  float phase_=0.f,from_=0.f,to_=.5f,pan_=0.f;
  bool idle_=true;
};
}
