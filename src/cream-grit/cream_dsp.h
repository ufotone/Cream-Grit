#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "sync_echo.h"
#include "pitch_shift.h"
#ifdef CREAM_GRIT_MASTER
#include "../cream-grit-master/master_space.h"
#endif
namespace cream {
constexpr float kFs = 48000.f;
#ifdef CREAM_GRIT_MASTER
constexpr int kCount = 22;
#else
constexpr int kCount = 15;
#endif

static const int kMin[kCount] = {0,-24,0,0,150,6,0,-24,0,0,0,0,-36,0,0
#ifdef CREAM_GRIT_MASTER
,0,0,1,0,0,3,0
#endif
};
static const int kMax[kCount] = {100,0,100,100,1200,16,100,6,100,100,8,85,24,100,1
#ifdef CREAM_GRIT_MASTER
,1,100,30,100,100,80,100
#endif
};
static const int kDefault[kCount] = {65,0,40,55,350,12,30,0,10,0,5,25,0,100,1
#ifdef CREAM_GRIT_MASTER
,0,0,8,40,0,18,60
#endif
};
static const int kDown[kCount] = {80,-13,40,50,350,12,35,0,10,0,5,25,0,100,1
#ifdef CREAM_GRIT_MASTER
,0,0,8,40,0,18,60
#endif
};
#ifdef CREAM_GRIT_MASTER
static const int kWide[kCount]={45,-7,30,65,350,12,30,0,10,25,5,30,0,100,1,1,20,8,35,18,18,60};
#endif
inline float pole(float hz) { return 1.f - std::exp(-6.28318530718f * hz / kFs); }
inline float saturate(float x) {
  x = std::max(-3.f, std::min(3.f, x));
  const float x2 = x*x;
  return x*(27.f+x2)/(27.f+9.f*x2);
}
inline float ceiling(float x) {
  const float a = std::fabs(x);
  if (a <= .85f) return x;
  const float d = a-.85f;
  const float y = .85f + .15f*d/(.15f+d);
  return x < 0.f ? -y : y;
}
struct Channel {
  float low[2] = {}, inputLP[2] = {}, outputLP[2] = {};
  float residualLP[4] = {};
  float previous = 0.f, held = 0.f;
  float noiseEnv = 0.f;
  float pitchHP[4]={};
};
class DSP {
 public:
  DSP() { set(kDefault); reset(); }
  void set(const int* p) {
    int v[kCount];
    for (int i=0;i<kCount;++i) v[i]=std::max(kMin[i],std::min(kMax[i],p[i]));
    target_[0]=v[0]*.01f;
    target_[1]=(26040.f/kFs)*std::pow(2.f,v[1]/12.f);
    target_[2]=v[2]*.01f;
    target_[3]=pole(2200.f*std::pow(6.f,v[3]*.01f));
    target_[4]=pole(static_cast<float>(v[4]));
    target_[5]=std::pow(2.f,static_cast<float>(v[5]-1));
    target_[6]=1.f+4.f*v[6]*.01f;
    target_[7]=std::pow(10.f,v[7]/20.f);
    target_[8]=v[8]*.01f;
    target_[9]=v[9]*.01f;
    target_[10]=SyncEcho::noteBeats(v[10]);
    target_[11]=v[11]*.01f;
    target_[12]=std::pow(2.f,v[12]/12.f);
    target_[13]=v[13]*.01f;
    target_[14]=static_cast<float>(v[14]);
    pitch_.setRatio(target_[12]);
#ifdef CREAM_GRIT_MASTER
    target_[15]=static_cast<float>(v[15]);echo_.setStereo(v[15]!=0);
    target_[16]=v[16]*.01f;target_[17]=v[17]*.1f;target_[18]=v[18]*.01f;
    target_[19]=v[19]*.01f;target_[20]=v[20]*.1f;target_[21]=v[21]*.01f;
    reverb_.configure(target_[20],target_[21]);
#endif
  }
  void setTempo(float bpm) {if(bpm>0.f)bpm_=std::max(30.f,std::min(300.f,bpm));}
  void reset() {
    for (auto &c: ch_) c=Channel{};
    phase_=0.f; envelope_=0.f;
    random_=0x2241200u;
    for(float &v:noiseFilter_) v=0.f;
    for(int i=0;i<kCount;++i) smooth_[i]=target_[i];
    echo_.setTiming(bpm_,target_[10]);echo_.reset();
    pitch_.reset();pitchActive_=target_[12]!=1.f&&target_[13]>0.f?1.f:0.f;
#ifdef CREAM_GRIT_MASTER
    chorus_.reset();reverb_.reset();
#endif
  }
  void process(const float* in, float* out, std::size_t frames, std::size_t inputStride=2) {
    constexpr float slew = .0020811647f;
    const float attack=pole(1.f/(6.28318530718f*.012f));
    const float release=pole(1.f/(6.28318530718f*.140f));
    const float pre=pole(14000.f);
    const float fmHP=pole(2400.f), fmLP=pole(11500.f);
    const float noiseRelease=pole(1.f/(6.28318530718f*.080f));
    echo_.setTiming(bpm_,target_[10]);
    for(std::size_t n=0;n<frames;++n) {
      for(int i=0;i<kCount;++i) smooth_[i]+=slew*(target_[i]-smooth_[i]);
      const float grit=smooth_[0], inc=smooth_[1], body=smooth_[2];
      const float tone=smooth_[3], cross=smooth_[4], q=smooth_[5];
      const float drive=smooth_[6], gain=smooth_[7];
      float x[2]={in[inputStride*n],in[inputStride*n+1]}, lo[2], high[2];
      float wet[2];
      float pitchIn[2],air[2];
      for(int c=0;c<2;++c) {
        lo[c]=lp(lp(x[c],ch_[c].low[0],cross),ch_[c].low[1],cross);
        high[c]=x[c]-lo[c];
      }
      const float peak=std::max(std::fabs(lo[0]),std::fabs(lo[1]));
      envelope_+=(peak>envelope_?attack:release)*(peak-envelope_);
      const float over=std::max(0.f,envelope_-.18f);
      const float reduction=1.f/(1.f+3.f*over);
      const float lowGain=1.f+body*(1.28f*reduction-1.f);
      phase_+=inc;
      const bool tick=phase_>=1.f;
      if(tick) phase_-=1.f;
      const float fraction=tick?std::max(0.f,std::min(1.f,1.f-phase_/inc)):0.f;
      const float norm=.2f/saturate(.2f*drive);
      random_^=random_<<13;random_^=random_>>17;random_^=random_<<5;
      const float white=(random_>>8)*(1.f/8388608.f)-1.f;
      const float radio=lp(white-lp(white,noiseFilter_[0],fmHP),noiseFilter_[1],fmLP);
      for(int c=0;c<2;++c) {
        Channel &s=ch_[c];
        const float limited=saturate(high[c]*drive)*norm;

        const float acquisition=lp(lp(limited,s.inputLP[0],pre),s.inputLP[1],pre);
        if(tick) {
          const float sample=s.previous+fraction*(acquisition-s.previous);
          s.held=std::floor(sample*q+.5f)/q;
        }
        s.previous=acquisition;
        const float dirty=lp(lp(s.held,s.outputLP[0],tone),s.outputLP[1],tone);

        float delta=dirty-high[c];
        for(int k=0;k<4;++k) delta-=lp(delta,s.residualLP[k],cross);
        const float level=std::fabs(x[c]);
        s.noiseEnv+=(level>s.noiseEnv?attack:noiseRelease)*(level-s.noiseEnv);
        if(s.noiseEnv<1.e-6f) s.noiseEnv=0.f;
        air[c]=.12f*smooth_[8]*s.noiseEnv*radio;
        const float y=x[c]+(lowGain-1.f)*lo[c]+grit*delta;
        wet[c]=y;
        const float hi=high[c]+grit*delta;
        pitchIn[c]=hi+smooth_[14]*(y-hi);
      }
      const float pitchGoal=target_[12]!=1.f&&target_[13]>0.f?1.f:0.f;
      pitchActive_+=slew*(pitchGoal-pitchActive_);
      if(pitchGoal==0.f&&pitchActive_<1.e-6f)pitchActive_=0.f;
      const bool pitchOn=pitchActive_>0.f;
      float shifted[2];pitch_.tick(pitchIn,shifted,smooth_[12],pitchOn);
      const float pitchMix=smooth_[13]*pitchActive_*pitch_.readiness();
      for(int c=0;c<2;++c) {
        float delta=shifted[c]-pitchIn[c],hiDelta=delta;
        if(pitchOn)for(float &s:ch_[c].pitchHP)hiDelta-=lp(hiDelta,s,cross);
        else for(float &s:ch_[c].pitchHP)s=0.f;
        wet[c]+=pitchMix*(hiDelta+smooth_[14]*(delta-hiDelta));
        wet[c]+=air[c];
      }
      const bool enabled=target_[9]>0.f || smooth_[9]>1.e-6f;
#ifdef CREAM_GRIT_MASTER
      float mod[2],repeats[2],space[2];
      const bool chorusOn=target_[16]>0.f||smooth_[16]>1.e-6f;
      chorus_.tick(wet,mod,smooth_[17],smooth_[18],chorusOn);
      for(int c=0;c<2;++c)wet[c]+=.35f*smooth_[16]*chorus_.readiness()*mod[c];
      echo_.tick(wet,repeats,smooth_[11],enabled);
      reverb_.tick(wet,space,target_[19]>0.f||smooth_[19]>1.e-6f);
      for(int c=0;c<2;++c)out[2*n+c]=ceiling(gain*(wet[c]+smooth_[9]*repeats[c]+smooth_[19]*space[c]));
#else
      const float repeats=echo_.tick((wet[0]+wet[1])*.5f,smooth_[11],enabled);
      for(int c=0;c<2;++c)out[2*n+c]=ceiling(gain*(wet[c]+smooth_[9]*repeats));
#endif
    }
  }
 private:
  static float lp(float x,float &state,float a) {
    state+=a*(x-state);
    if(std::fabs(state)<1.e-20f) state=0.f;
    return state;
  }
  Channel ch_[2];
  float target_[kCount]={},smooth_[kCount]={};
  float phase_=0.f,envelope_=0.f;
  uint32_t random_=0x2241200u;
  float noiseFilter_[2]={};
  float bpm_=120.f;
#ifdef CREAM_GRIT_MASTER
  MasterEcho echo_;
  SimpleChorus chorus_;
  SimpleReverb reverb_;
#else
  SyncEcho echo_;
#endif
  PitchShift pitch_;
  float pitchActive_=0.f;
};
}
