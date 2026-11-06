#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace cream {

class SyncEcho {
 public:
  static constexpr uint32_t kSize=48002;
  static constexpr uint32_t kBytes=kSize*sizeof(int16_t);
  static float noteBeats(int i) {
    static const float beats[]={.125f,1.f/6.f,.25f,1.f/3.f,.375f,.5f,2.f/3.f,.75f,1.f};
    return beats[i];
  }
  static const char* noteName(int i) {
    static const char* names[]={"1/32","1/16T","1/16","1/8T","1/16D","1/8","1/4T","1/8D","1/4"};
    return i>=0&&i<9?names[i]:nullptr;
  }
  void setTiming(float bpm,float beats) {
    bpm=std::max(30.f,std::min(300.f,bpm));
    requested_=std::max(1.f,std::min(48000.f,24000.f*60.f*beats/bpm));
  }
  void reset() {

    write_=valid_=0;odd_=false;inputLP_[0]=inputLP_[1]=0.f;
    current_=from_=requested_;fade_=1.f;idle_=true;
  }
  float tick(float input,float feedback,bool enabled) {
    if(!enabled) {if(!idle_)reset();return 0.f;}
    idle_=false;
    if(fade_>=1.f && std::fabs(requested_-current_)>.5f) {
      from_=current_;current_=requested_;fade_=0.f;
    }
    float echo=read(current_);
    if(fade_<1.f) {
      const float old=read(from_);
      echo=old+fade_*(echo-old);
      fade_=std::min(1.f,fade_+1.f/768.f);
    }

    constexpr float a=.600002f;
    inputLP_[0]+=a*(input-inputLP_[0]);
    inputLP_[1]+=a*(inputLP_[0]-inputLP_[1]);
    if(!odd_) {
      const float v=std::max(-1.f,std::min(1.f,inputLP_[1]+feedback*echo));
      const float scaled=v*32767.f;
      buffer_[write_]=static_cast<int16_t>(scaled+(scaled>=0.f?.5f:-.5f));
      if(++write_==kSize)write_=0;
      if(valid_<kSize)++valid_;
    }
    odd_=!odd_;
    return echo;
  }
 private:
  float sample(uint32_t i)const {return (valid_==kSize||i<valid_)?buffer_[i]*(1.f/32767.f):0.f;}
  float read(float delay)const {
    float pos=static_cast<float>(write_)-delay-(odd_?.5f:0.f);
    if(pos<0.f)pos+=kSize;
    const uint32_t i=static_cast<uint32_t>(pos);
    const uint32_t j=i+1==kSize?0:i+1;
    const float f=pos-i;
    const float a=sample(i);return a+f*(sample(j)-a);
  }
  int16_t buffer_[kSize];
  uint32_t write_=0,valid_=0;
  bool odd_=false,idle_=true;
  float inputLP_[2]={};
  float requested_=6000.f,current_=6000.f,from_=6000.f,fade_=1.f;
};
}
