#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace cream {

class PitchShift {
 public:
  static constexpr uint32_t kFrames=8192, kMask=kFrames-1;
  static constexpr uint32_t kBytes=kFrames*2*sizeof(int16_t);
  void setRatio(float ratio) {

    const float cutoff=ratio<=1.f?20000.f:std::min(14000.f,18000.f/ratio);
    aaTarget_=1.f-std::exp(-6.28318530718f*cutoff/48000.f);
    depthTarget_=ratio<=1.f?0.f:1.f;
  }
  void reset() {
    write_=valid_=0;phase_=0.f;ready_=0.f;idle_=true;aa_=aaTarget_;depth_=depthTarget_;
    for(auto &c:filter_)for(float &s:c)s=0.f;
  }
  float readiness()const {return ready_;}
  void tick(const float* input,float* output,float ratio,bool enabled) {
    if(!enabled) {
      if(!idle_)reset();
      output[0]=input[0];output[1]=input[1];return;
    }
    idle_=false;
    aa_+=.0020811647f*(aaTarget_-aa_);
    depth_+=.0020811647f*(depthTarget_-depth_);
    for(int c=0;c<2;++c) {
      filter_[c][0]+=aa_*(input[c]-filter_[c][0]);
      const float light=filter_[c][0];float v=light;
      for(int k=1;k<4;++k){float &s=filter_[c][k];s+=aa_*(v-s);v=s;}
      if(depth_<=0.f)v=light;
      else if(depth_<1.f)v=light+depth_*(v-light);
      const float scaled=std::max(-2.f,std::min(2.f,v))*16383.5f;
      buffer_[write_*2+c]=static_cast<int16_t>(scaled+(scaled>=0.f?.5f:-.5f));
    }
    if(valid_<kFrames)++valid_;
    float other=phase_+.5f;if(other>=1.f)other-=1.f;
    const float weight=1.f-std::fabs(2.f*phase_-1.f);
    const float a=8.f+4800.f*phase_,b=8.f+4800.f*other;
    for(int c=0;c<2;++c)output[c]=weight*read(a,c)+(1.f-weight)*read(b,c);
    write_=(write_+1)&kMask;
    phase_+=(1.f-ratio)/4800.f;
    if(phase_<0.f)phase_+=1.f;
    if(phase_>=1.f)phase_-=1.f;
    if(valid_>=4810)ready_+=.0020811647f*(1.f-ready_);
  }
 private:
  float sample(uint32_t i,int c)const {
    return valid_==kFrames||i<valid_?buffer_[i*2+c]*(2.f/32767.f):0.f;
  }
  float read(float delay,int c)const {
    float pos=static_cast<float>(write_)-delay;
    if(pos<0.f)pos+=kFrames;
#ifdef CREAM_GRIT_MASTER
    if(pos>=kFrames)pos-=kFrames;
#endif
    const uint32_t i=static_cast<uint32_t>(pos);
    const float f=pos-i,a=sample(i,c);
    return a+f*(sample((i+1)&kMask,c)-a);
  }
  int16_t buffer_[kFrames*2];
  uint32_t write_=0,valid_=0;
  float phase_=0.f,ready_=0.f,aa_=.8f,aaTarget_=.8f;
  float depth_=0.f,depthTarget_=0.f;
  float filter_[2][4]={};
  bool idle_=true;
};
}
