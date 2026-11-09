#pragma once
#include "../cream-grit/sync_echo.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cream {
class MasterEcho {
 public:
  static constexpr uint32_t kBytes=2*SyncEcho::kBytes;
  void setTiming(float bpm,float beats){left_.setTiming(bpm,beats);right_.setTiming(bpm,beats);}
  void setStereo(bool stereo){requested_=stereo;}
  void reset(){left_.reset();right_.reset();stereo_=requested_;fade_=1.f;}
  void tick(const float* in,float* out,float feedback,bool enabled){
    if(!enabled){left_.tick(0,0,false);right_.tick(0,0,false);stereo_=requested_;fade_=1.f;out[0]=out[1]=0;return;}
    if(stereo_!=requested_){
      fade_=std::max(0.f,fade_-1.f/480.f);
      if(fade_==0.f){left_.reset();right_.reset();stereo_=requested_;}
    }else fade_=std::min(1.f,fade_+1.f/480.f);
    if(stereo_){out[0]=left_.tick(in[0],feedback,true);out[1]=right_.tick(in[1],feedback,true);}
    else{out[0]=out[1]=left_.tick((in[0]+in[1])*.5f,feedback,true);right_.tick(0,0,false);}
    out[0]*=fade_;out[1]*=fade_;
  }
 private:
  SyncEcho left_,right_;
  bool requested_=false,stereo_=false;
  float fade_=1.f;
};

class SimpleChorus {
 public:
  static constexpr uint32_t kFrames=1024,kMask=kFrames-1,kBytes=kFrames*2*sizeof(int16_t);
  void reset(){write_=valid_=0;phase_=ready_=0;idle_=true;}
  float readiness()const{return ready_;}
  void tick(const float* in,float* out,float rate,float depth,bool enabled){
    if(!enabled){if(!idle_)reset();out[0]=out[1]=0;return;}
    idle_=false;
    for(int c=0;c<2;++c){
      const float s=std::max(-2.f,std::min(2.f,in[c]))*16383.5f;
      buffer_[write_*2+c]=static_cast<int16_t>(s+(s>=0?.5f:-.5f));
    }
    if(valid_<kFrames)++valid_;
    for(int c=0;c<2;++c){
      float p=phase_+.25f*c;if(p>=1)p-=1;
      if(p>=.5f)p-=1;
      const float t=4*p,mod=t*(2-std::fabs(t));
      const float delay=576.f+336.f*depth*mod;
      float pos=static_cast<float>(write_)-delay;if(pos<0)pos+=kFrames;
      if(pos>=kFrames)pos-=kFrames;
      const uint32_t i=static_cast<uint32_t>(pos);const float f=pos-i,a=sample(i,c);
      out[c]=a+f*(sample((i+1)&kMask,c)-a);
    }
    write_=(write_+1)&kMask;
    phase_+=rate/48000.f;if(phase_>=1)phase_-=1;
    if(valid_>=915)ready_+=.0020811647f*(1-ready_);
  }
 private:
  float sample(uint32_t i,int c)const{return valid_==kFrames||i<valid_?buffer_[i*2+c]*(2.f/32767.f):0.f;}
  int16_t buffer_[kFrames*2];
  uint32_t write_=0,valid_=0;
  float phase_=0,ready_=0;
  bool idle_=true;
};

template<uint32_t N> class SpaceLine {
 public:
  void reset(){pos_=valid_=0;}
  float read()const{return valid_==N?data_[pos_]:0.f;}
  void write(float v){data_[pos_]=std::fabs(v)<1.e-15f?0.f:v;if(++pos_==N)pos_=0;if(valid_<N)++valid_;}
  float allpass(float in){const float y=read()-.5f*in;write(in+.5f*y);return y;}
 private:
  float data_[N];
  uint32_t pos_=0,valid_=0;
};

class SimpleReverb {
 public:
  static constexpr uint32_t kBytes=(719+887+1061+1277+149+211)*sizeof(float);
  void configure(float seconds,float damp){
    if(seconds!=decay_){
      const float lengths[4]={719,887,1061,1277};
      for(int i=0;i<4;++i)gainTarget_[i]=std::pow(.001f,lengths[i]/(24000.f*seconds));
      decay_=seconds;
    }
    if(damp!=damp_){
      const float cutoff=1200.f*std::pow(8500.f/1200.f,1.f-damp);
      filterTarget_=1.f-std::exp(-6.28318530718f*cutoff/24000.f);damp_=damp;
    }
  }
  void reset(){
    a_.reset();b_.reset();c_.reset();d_.reset();apL_.reset();apR_.reset();
    for(int i=0;i<4;++i){feedback_[i]=0;gain_[i]=gainTarget_[i];}
    for(int c=0;c<2;++c){highpass_[c]=0;pre_[c][0]=pre_[c][1]=0;previous_[c]=current_[c]=0;}
    filter_=filterTarget_;odd_=false;idle_=true;
  }
  void tick(const float* input,float* out,bool enabled){
    if(!enabled){if(!idle_)reset();out[0]=out[1]=0;return;}
    idle_=false;
    float in[2];
    for(int c=0;c<2;++c){
      const float hp=input[c]-lp(input[c],highpass_[c],.018158f);
      in[c]=lp(lp(hp,pre_[c][0],.544062f),pre_[c][1],.544062f);
    }
    if(!odd_){
      filter_+=.004158f*(filterTarget_-filter_);
      const float read[4]={a_.read(),b_.read(),c_.read(),d_.read()};
      for(int i=0;i<4;++i){lp(read[i],feedback_[i],filter_);gain_[i]+=.004158f*(gainTarget_[i]-gain_[i]);}
      const float x=feedback_[0],y=feedback_[1],z=feedback_[2],w=feedback_[3];
      const float l=apL_.allpass(in[0]),r=apR_.allpass(in[1]);
      a_.write(.35f*l+.5f*(x+y+z+w)*gain_[0]);
      b_.write(.35f*r+.5f*(x-y+z-w)*gain_[1]);
      c_.write(.247487f*(l+r)+.5f*(x+y-z-w)*gain_[2]);
      d_.write(.247487f*(l-r)+.5f*(x-y-z+w)*gain_[3]);
      previous_[0]=current_[0];previous_[1]=current_[1];
      current_[0]=.35f*(x+z-y);current_[1]=.35f*(y+w-z);
    }
    for(int c=0;c<2;++c)out[c]=odd_?.5f*(previous_[c]+current_[c]):previous_[c];
    odd_=!odd_;
  }
 private:
  static float lp(float x,float& state,float a){state+=a*(x-state);if(std::fabs(state)<1.e-20f)state=0;return state;}
  SpaceLine<719>a_;SpaceLine<887>b_;SpaceLine<1061>c_;SpaceLine<1277>d_;
  SpaceLine<149>apL_;SpaceLine<211>apR_;
  float feedback_[4]={},gain_[4]={},gainTarget_[4]={.8f,.8f,.8f,.8f};
  float highpass_[2]={},pre_[2][2]={},previous_[2]={},current_[2]={};
  float decay_=-1,damp_=-1,filter_=.5f,filterTarget_=.5f;
  bool odd_=false,idle_=true;
};
}
