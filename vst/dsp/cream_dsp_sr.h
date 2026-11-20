// Sample-rate-aware port of the Cream Grit DSP (src/cream-grit/*.h, src/cream-grit-master/*.h).
// At 48 kHz every constant evaluates exactly as in the drumlogue build, so the output is
// bit-identical (verified by vst/tests/null_test.cpp). Other rates scale sample counts and
// convert one-pole coefficients so time constants and frequencies stay the same.
// Build with CREAM_GRIT_MASTER defined for the MASTER edition, as on the drumlogue.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace creamvst {

constexpr float kRefFs = 48000.f;

// Rescales a per-sample one-pole coefficient designed at 48 kHz to sample rate fs.
inline float rescale(float c, float fs) {
  return fs == kRefFs ? c : 1.f - std::pow(1.f - c, kRefFs / fs);
}
inline uint32_t scaled(float n, float k) { return static_cast<uint32_t>(std::lround(n * k)); }
inline uint32_t pow2AtLeast(uint32_t n) { uint32_t p = 1; while (p < n) p <<= 1; return p; }

// ---------------------------------------------------------------- SyncEcho
class SyncEcho {
 public:
  static float noteBeats(int i) {
    static const float beats[]={.125f,1.f/6.f,.25f,1.f/3.f,.375f,.5f,2.f/3.f,.75f,1.f};
    return beats[i];
  }
  static const char* noteName(int i) {
    static const char* names[]={"1/32","1/16T","1/16","1/8T","1/16D","1/8","1/4T","1/8D","1/4"};
    return i>=0&&i<9?names[i]:nullptr;
  }
  void prepare(float fs) {
    k_=fs/kRefFs;
    size_=fs==kRefFs?48002u:static_cast<uint32_t>(std::ceil(48002.f*k_));
    buffer_.assign(size_,0);
    maxDelay_=48000.f*k_;
    fadeStep_=1.f/(768.f*k_);
    a_=rescale(.600002f,fs);
    requested_=current_=from_=6000.f*k_;
  }
  void setTiming(float bpm,float beats) {
    bpm=std::max(30.f,std::min(300.f,bpm));
    requested_=std::max(1.f,std::min(maxDelay_,24000.f*k_*60.f*beats/bpm));
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
      fade_=std::min(1.f,fade_+fadeStep_);
    }
    inputLP_[0]+=a_*(input-inputLP_[0]);
    inputLP_[1]+=a_*(inputLP_[0]-inputLP_[1]);
    if(!odd_) {
      const float v=std::max(-1.f,std::min(1.f,inputLP_[1]+feedback*echo));
      const float s=v*32767.f;
      buffer_[write_]=static_cast<int16_t>(s+(s>=0.f?.5f:-.5f));
      if(++write_==size_)write_=0;
      if(valid_<size_)++valid_;
    }
    odd_=!odd_;
    return echo;
  }
 private:
  float sample(uint32_t i)const {return (valid_==size_||i<valid_)?buffer_[i]*(1.f/32767.f):0.f;}
  float read(float delay)const {
    float pos=static_cast<float>(write_)-delay-(odd_?.5f:0.f);
    if(pos<0.f)pos+=size_;
    if(pos>=size_)pos-=size_;
    const uint32_t i=static_cast<uint32_t>(pos);
    const uint32_t j=i+1==size_?0:i+1;
    const float f=pos-i;
    const float a=sample(i);return a+f*(sample(j)-a);
  }
  std::vector<int16_t> buffer_;
  uint32_t size_=0,write_=0,valid_=0;
  bool odd_=false,idle_=true;
  float k_=1.f,maxDelay_=48000.f,fadeStep_=1.f/768.f,a_=.600002f;
  float inputLP_[2]={};
  float requested_=6000.f,current_=6000.f,from_=6000.f,fade_=1.f;
};

// ---------------------------------------------------------------- PitchShift
class PitchShift {
 public:
  void prepare(float fs) {
    fs_=fs;const float k=fs/kRefFs;
    frames_=pow2AtLeast(static_cast<uint32_t>(std::ceil(8192.f*k)));mask_=frames_-1;
    buffer_.assign(frames_*2,0);
    minDelay_=8.f*k;window_=4800.f*k;cleanWindow_=1920.f*k;samplerWindow_=1056.f*k;readyAt_=scaled(4810.f,k);
    slew_=rescale(.0020811647f,fs);
  }
  void setRatio(float ratio) {
    const float cutoff=ratio<=1.f?20000.f:std::min(14000.f,18000.f/ratio);
    aaTarget_=1.f-std::exp(-6.28318530718f*std::min(cutoff,.45f*fs_)/fs_);
    depthTarget_=ratio<=1.f?0.f:1.f;
  }
  void reset() {
    write_=valid_=0;phase_=0.f;ready_=0.f;idle_=true;aa_=aaTarget_;depth_=depthTarget_;
    holdPhase_=0.f;held_[0]=held_[1]=0.f;
    for(auto &c:filter_)for(float &s:c)s=0.f;
  }
  float readiness()const {return ready_;}
  // Sampler mode: no anti-alias filter, drop-sample reads and a sample-and-hold at the
  // virtual playback clock (holdInc), like replaying a low-rate sample at another key.
  // mode: 0 CLEAN (40 ms grains), 1 SAMPLER, 2 DRUMLOGUE (original 100 ms grains).
  void tick(const float* input,float* output,float ratio,bool enabled,int mode=2,float holdInc=1.f) {
    if(!enabled) {
      if(!idle_)reset();
      output[0]=input[0];output[1]=input[1];return;
    }
    idle_=false;
    if(mode==1) { tickSampler(input,output,ratio,holdInc); return; }
    const float win=mode==0?cleanWindow_:window_;
    aa_+=slew_*(aaTarget_-aa_);
    depth_+=slew_*(depthTarget_-depth_);
    for(int c=0;c<2;++c) {
      filter_[c][0]+=aa_*(input[c]-filter_[c][0]);
      const float light=filter_[c][0];float v=light;
      for(int k=1;k<4;++k){float &s=filter_[c][k];s+=aa_*(v-s);v=s;}
      if(depth_<=0.f)v=light;
      else if(depth_<1.f)v=light+depth_*(v-light);
      const float scaledV=std::max(-2.f,std::min(2.f,v))*16383.5f;
      buffer_[write_*2+c]=static_cast<int16_t>(scaledV+(scaledV>=0.f?.5f:-.5f));
    }
    if(valid_<frames_)++valid_;
    float other=phase_+.5f;if(other>=1.f)other-=1.f;
    const float weight=1.f-std::fabs(2.f*phase_-1.f);
    const float a=minDelay_+win*phase_,b=minDelay_+win*other;
    for(int c=0;c<2;++c)output[c]=weight*read(a,c)+(1.f-weight)*read(b,c);
    write_=(write_+1)&mask_;
    phase_+=(1.f-ratio)/win;
    if(phase_<0.f)phase_+=1.f;
    if(phase_>=1.f)phase_-=1.f;
    if(valid_>=readyAt_)ready_+=slew_*(1.f-ready_);
  }
 private:
  void tickSampler(const float* input,float* output,float ratio,float holdInc) {
    for(int c=0;c<2;++c) {
      const float s=std::max(-2.f,std::min(2.f,input[c]))*16383.5f;
      buffer_[write_*2+c]=static_cast<int16_t>(s+(s>=0.f?.5f:-.5f));
    }
    if(valid_<frames_)++valid_;
    float other=phase_+.5f;if(other>=1.f)other-=1.f;
    const float weight=1.f-std::fabs(2.f*phase_-1.f);
    // Short grains (22 ms) so the second read tap is heard as grain, not as a slapback echo.
    const float a=minDelay_+samplerWindow_*phase_,b=minDelay_+samplerWindow_*other;
    holdPhase_+=std::max(1.e-4f,std::min(1.f,holdInc*ratio));
    if(holdPhase_>=1.f) {
      holdPhase_-=1.f;
      for(int c=0;c<2;++c)held_[c]=weight*readNearest(a,c)+(1.f-weight)*readNearest(b,c);
    }
    output[0]=held_[0];output[1]=held_[1];
    write_=(write_+1)&mask_;
    phase_+=(1.f-ratio)/samplerWindow_;
    if(phase_<0.f)phase_+=1.f;
    if(phase_>=1.f)phase_-=1.f;
    if(valid_>=readyAt_)ready_+=slew_*(1.f-ready_);
  }
  float readNearest(float delay,int c)const {
    float pos=static_cast<float>(write_)-delay;
    if(pos<0.f)pos+=frames_;
    return sample(static_cast<uint32_t>(pos)&mask_,c);
  }
  float holdPhase_=0.f,held_[2]={},samplerWindow_=1056.f,cleanWindow_=1920.f;
  float sample(uint32_t i,int c)const {
    return valid_==frames_||i<valid_?buffer_[i*2+c]*(2.f/32767.f):0.f;
  }
  float read(float delay,int c)const {
    float pos=static_cast<float>(write_)-delay;
    if(pos<0.f)pos+=frames_;
    if(pos>=frames_)pos-=frames_;
    const uint32_t i=static_cast<uint32_t>(pos);
    const float f=pos-i,a=sample(i,c);
    return a+f*(sample((i+1)&mask_,c)-a);
  }
  std::vector<int16_t> buffer_;
  uint32_t frames_=8192,mask_=8191,readyAt_=4810,write_=0,valid_=0;
  float fs_=kRefFs,minDelay_=8.f,window_=4800.f,slew_=.0020811647f;
  float phase_=0.f,ready_=0.f,aa_=.8f,aaTarget_=.8f;
  float depth_=0.f,depthTarget_=0.f;
  float filter_[2][4]={};
  bool idle_=true;
};

#ifdef CREAM_GRIT_MASTER
// ---------------------------------------------------------------- SimpleChorus
class SimpleChorus {
 public:
  void prepare(float fs) {
    fs_=fs;const float k=fs/kRefFs;
    frames_=pow2AtLeast(static_cast<uint32_t>(std::ceil(1024.f*k)));mask_=frames_-1;
    buffer_.assign(frames_*2,0);
    center_=576.f*k;swing_=336.f*k;readyAt_=scaled(915.f,k);slew_=rescale(.0020811647f,fs);
  }
  void reset(){write_=valid_=0;phase_=ready_=0;idle_=true;}
  float readiness()const{return ready_;}
  void tick(const float* in,float* out,float rate,float depth,bool enabled){
    if(!enabled){if(!idle_)reset();out[0]=out[1]=0;return;}
    idle_=false;
    for(int c=0;c<2;++c){
      const float s=std::max(-2.f,std::min(2.f,in[c]))*16383.5f;
      buffer_[write_*2+c]=static_cast<int16_t>(s+(s>=0?.5f:-.5f));
    }
    if(valid_<frames_)++valid_;
    for(int c=0;c<2;++c){
      float p=phase_+.25f*c;if(p>=1)p-=1;
      if(p>=.5f)p-=1;
      const float t=4*p,mod=t*(2-std::fabs(t));
      const float delay=center_+swing_*depth*mod;
      float pos=static_cast<float>(write_)-delay;if(pos<0)pos+=frames_;
      if(pos>=frames_)pos-=frames_;
      const uint32_t i=static_cast<uint32_t>(pos);const float f=pos-i,a=sample(i,c);
      out[c]=a+f*(sample((i+1)&mask_,c)-a);
    }
    write_=(write_+1)&mask_;
    phase_+=rate/fs_;if(phase_>=1)phase_-=1;
    if(valid_>=readyAt_)ready_+=slew_*(1-ready_);
  }
 private:
  float sample(uint32_t i,int c)const{return valid_==frames_||i<valid_?buffer_[i*2+c]*(2.f/32767.f):0.f;}
  std::vector<int16_t> buffer_;
  uint32_t frames_=1024,mask_=1023,readyAt_=915,write_=0,valid_=0;
  float fs_=kRefFs,center_=576.f,swing_=336.f,slew_=.0020811647f;
  float phase_=0,ready_=0;
  bool idle_=true;
};

class SpaceLine {
 public:
  void prepare(uint32_t n){n_=n;data_.assign(n,0.f);reset();}
  void reset(){pos_=valid_=0;}
  float read()const{return valid_==n_?data_[pos_]:0.f;}
  void write(float v){data_[pos_]=std::fabs(v)<1.e-15f?0.f:v;if(++pos_==n_)pos_=0;if(valid_<n_)++valid_;}
  float allpass(float in){const float y=read()-.5f*in;write(in+.5f*y);return y;}
 private:
  std::vector<float> data_;
  uint32_t n_=1,pos_=0,valid_=0;
};

// ---------------------------------------------------------------- SimpleReverb
class SimpleReverb {
 public:
  void prepare(float fs){
    k_=fs/kRefFs;
    static const float base[6]={719,887,1061,1277,149,211};
    SpaceLine* lines[6]={&a_,&b_,&c_,&d_,&apL_,&apR_};
    for(int i=0;i<6;++i){len_[i]=static_cast<float>(scaled(base[i],k_));lines[i]->prepare(static_cast<uint32_t>(len_[i]));}
    hpCoef_=rescale(.018158f,fs);preCoef_=rescale(.544062f,fs);slew_=rescale(.004158f,fs);
    decay_=damp_=-1;
  }
  void configure(float seconds,float damp){
    if(seconds!=decay_){
      for(int i=0;i<4;++i)gainTarget_[i]=std::pow(.001f,len_[i]/(24000.f*k_*seconds));
      decay_=seconds;
    }
    if(damp!=damp_){
      const float cutoff=1200.f*std::pow(8500.f/1200.f,1.f-damp);
      filterTarget_=1.f-std::exp(-6.28318530718f*cutoff/(24000.f*k_));damp_=damp;
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
      const float hp=input[c]-lp(input[c],highpass_[c],hpCoef_);
      in[c]=lp(lp(hp,pre_[c][0],preCoef_),pre_[c][1],preCoef_);
    }
    if(!odd_){
      filter_+=slew_*(filterTarget_-filter_);
      const float read[4]={a_.read(),b_.read(),c_.read(),d_.read()};
      for(int i=0;i<4;++i){lp(read[i],feedback_[i],filter_);gain_[i]+=slew_*(gainTarget_[i]-gain_[i]);}
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
  SpaceLine a_,b_,c_,d_,apL_,apR_;
  float len_[6]={719,887,1061,1277,149,211};
  float k_=1.f,hpCoef_=.018158f,preCoef_=.544062f,slew_=.004158f;
  float feedback_[4]={},gain_[4]={},gainTarget_[4]={.8f,.8f,.8f,.8f};
  float highpass_[2]={},pre_[2][2]={},previous_[2]={},current_[2]={};
  float decay_=-1,damp_=-1,filter_=.5f,filterTarget_=.5f;
  bool odd_=false,idle_=true;
};

// ---------------------------------------------------------------- RandomPan
class RandomPan {
 public:
  void prepare(float fs){fs_=fs;coef_=rescale(.02061782f,fs);}
  void reset(){random_=0x70616e31u;phase_=from_=pan_=0.f;to_=next();idle_=true;}
  void tick(const float* in,float* out,float depth,float rate,bool square,bool enabled){
    if(!enabled){if(!idle_)reset();out[0]=in[0];out[1]=in[1];return;}
    idle_=false;
    const float position=square?to_:from_+(to_-from_)*curve(phase_);
    pan_+=coef_*(depth*position-pan_);
    const float p=std::max(-1.f,std::min(1.f,pan_));
    const float mid=(in[0]+in[1])*.5f,side=(in[0]-in[1])*.5f*(1.f-std::fabs(p));
    out[0]=mid*gain(-p)+side;out[1]=mid*gain(p)-side;
    phase_+=rate/fs_;
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
  float fs_=kRefFs,coef_=.02061782f;
  float phase_=0.f,from_=0.f,to_=.5f,pan_=0.f;
  bool idle_=true;
};
#endif

// ---------------------------------------------------------------- parameter tables
// VST-only parameters are appended after the drumlogue ones (kOrigCount):
//   COMP (0-100 %, full-band compressor) and P-MODE (0 CLEAN / 1 SAMPLER / 2 DRUMLOGUE pitch).
#ifdef CREAM_GRIT_MASTER
constexpr int kOrigCount = 24;
constexpr int kPresets = 4;
#else
constexpr int kOrigCount = 15;
constexpr int kPresets = 2;
#endif
//   RING (0-50 %, ring-modulator blend) and RM-FREQ (0-230 Hz carrier, 0 = off).
constexpr int kCount = kOrigCount + 4;
constexpr int kComp = kOrigCount, kPMode = kOrigCount + 1, kRing = kOrigCount + 2, kRingHz = kOrigCount + 3;

static const int kMin[kCount] = {0,-24,0,0,150,6,0,-24,0,0,0,0,-36,0,0
#ifdef CREAM_GRIT_MASTER
,0,0,1,0,0,3,0,1,0
#endif
,0,0,0,0
};
static const int kMax[kCount] = {100,0,100,100,1200,16,100,6,100,100,8,85,24,100,1
#ifdef CREAM_GRIT_MASTER
,100,100,30,100,100,80,100,80,1
#endif
,100,2,50,230
};
static const int kDefault[kCount] = {65,0,40,55,350,12,30,0,10,0,5,25,0,100,1
#ifdef CREAM_GRIT_MASTER
,0,0,8,40,0,18,60,10,0
#endif
,45,1,0,0
};
static const int kDown[kCount] = {80,-13,40,50,350,12,35,0,10,0,5,25,0,100,1
#ifdef CREAM_GRIT_MASTER
,0,0,8,40,0,18,60,10,0
#endif
,60,1,0,0
};
#ifdef CREAM_GRIT_MASTER
static const int kWide[kCount]={45,-7,30,65,350,12,30,0,10,25,5,30,0,100,1,0,20,8,35,18,18,60,10,0,45,1,0,0};
static const int kDrift[kCount]={45,-7,30,65,350,12,30,0,10,25,5,30,0,100,1,60,20,8,35,18,18,60,10,0,45,1,0,0};
#endif

inline const int* presetValues(int i) {
#ifdef CREAM_GRIT_MASTER
  return i==3?kDrift:i==2?kWide:i==1?kDown:kDefault;
#else
  return i?kDown:kDefault;
#endif
}
inline const char* presetName(int i) {
#ifdef CREAM_GRIT_MASTER
  if(i==2)return "Wide Space";
  if(i==3)return "Random Drift";
#endif
  return i==0?"12bit":i==1?"Down Dirt":"";
}

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

// ---------------------------------------------------------------- DSP
class DSP {
 public:
  DSP() { for(int i=0;i<kCount;++i) params_[i]=kDefault[i]; prepare(kRefFs); }
  // Allocates buffers; call from a non-realtime thread.
  void prepare(float fs) {
    fs_=fs;slew_=rescale(.0020811647f,fs);
    echo_.prepare(fs);pitch_.prepare(fs);
#ifdef CREAM_GRIT_MASTER
    chorus_.prepare(fs);reverb_.prepare(fs);pan_.prepare(fs);
#endif
    set(params_);reset();
  }
  void set(const int* p) {
    int v[kCount];
    for (int i=0;i<kCount;++i) {v[i]=std::max(kMin[i],std::min(kMax[i],p[i]));params_[i]=v[i];}
    target_[0]=v[0]*.01f;
    target_[1]=(26040.f/fs_)*std::pow(2.f,v[1]/12.f);
    target_[2]=v[2]*.01f;
    target_[3]=pole(std::min(2200.f*std::pow(6.f,v[3]*.01f),.45f*fs_));
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
    target_[kComp]=v[kComp]*.01f;
    target_[kPMode]=static_cast<float>(v[kPMode]);
    target_[kRing]=v[kRingHz]>0?v[kRing]*.01f:0.f;  // RM-FREQ 0 = off
    target_[kRingHz]=static_cast<float>(v[kRingHz]);
#ifdef CREAM_GRIT_MASTER
    target_[15]=v[15]*.01f;
    target_[16]=v[16]*.01f;target_[17]=v[17]*.1f;target_[18]=v[18]*.01f;
    target_[19]=v[19]*.01f;target_[20]=v[20]*.1f;target_[21]=v[21]*.01f;
    reverb_.configure(target_[20],target_[21]);
    target_[22]=v[22]*.1f;target_[23]=static_cast<float>(v[23]);
#endif
  }
  void setTempo(float bpm) {if(bpm>0.f)bpm_=std::max(30.f,std::min(300.f,bpm));}
  void reset() {
    for (auto &c: ch_) c=Channel{};
    phase_=0.f; envelope_=0.f;
    random_=0x2241200u;
    for(float &v:noiseFilter_) v=0.f;
    compEnv_=0.f;ringPhase_=0.f;ringPowIn_=ringPowOut_=0.f;ringGain_=1.f;
    for(int i=0;i<kCount;++i) smooth_[i]=target_[i];
    echo_.setTiming(bpm_,target_[10]);echo_.reset();
    pitch_.reset();pitchActive_=target_[12]!=1.f&&target_[13]>0.f?1.f:0.f;
#ifdef CREAM_GRIT_MASTER
    chorus_.reset();reverb_.reset();pan_.reset();
#endif
  }
  // in/out: interleaved stereo. In-place (in == out) is allowed.
  void process(const float* in, float* out, std::size_t frames) {
    const float slew = slew_;
    const float attack=pole(1.f/(6.28318530718f*.012f));
    const float release=pole(1.f/(6.28318530718f*.140f));
    const float pre=pole(std::min(14000.f,.45f*fs_));
    const float fmHP=pole(2400.f), fmLP=pole(std::min(11500.f,.45f*fs_));
    const float noiseRelease=pole(1.f/(6.28318530718f*.080f));
    const float compAttack=pole(1.f/(6.28318530718f*.002f));
    const float compRelease=pole(1.f/(6.28318530718f*.090f));
    const float ringPow=pole(1.f/(6.28318530718f*.008f));
    const float ringGlide=pole(1.f/(6.28318530718f*.002f));
    echo_.setTiming(bpm_,target_[10]);
    for(std::size_t n=0;n<frames;++n) {
      for(int i=0;i<kCount;++i) smooth_[i]+=slew*(target_[i]-smooth_[i]);
      const float grit=smooth_[0], inc=smooth_[1], body=smooth_[2];
      const float tone=smooth_[3], cross=smooth_[4], q=smooth_[5];
      const float drive=smooth_[6], gain=smooth_[7];
      float x[2]={in[2*n],in[2*n+1]}, lo[2], high[2];
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
      float shifted[2];pitch_.tick(pitchIn,shifted,smooth_[12],pitchOn,static_cast<int>(target_[kPMode]),inc);
      const float pitchMix=smooth_[13]*pitchActive_*pitch_.readiness();
      for(int c=0;c<2;++c) {
        float delta=shifted[c]-pitchIn[c],hiDelta=delta;
        if(pitchOn)for(float &s:ch_[c].pitchHP)hiDelta-=lp(hiDelta,s,cross);
        else for(float &s:ch_[c].pitchHP)s=0.f;
        wet[c]+=pitchMix*(hiDelta+smooth_[14]*(delta-hiDelta));
        wet[c]+=air[c];
      }
      if(target_[kComp]>0.f||smooth_[kComp]>1.e-6f) compress(wet,smooth_[kComp],compAttack,compRelease);
      if(target_[kRing]>0.f||smooth_[kRing]>1.e-6f) {
        // Ring-modulated copy added on top: wet *= 1 + m*carrier (m <= .5, never reaches silence).
        ringPhase_+=smooth_[kRingHz]/fs_;if(ringPhase_>=1.f)ringPhase_-=1.f;
        const float carrier=std::sin(6.28318530718f*ringPhase_);
        const float m=smooth_[kRing];
        float dry2=0.f,out2=0.f;
        for(int c=0;c<2;++c){dry2+=wet[c]*wet[c];wet[c]+=m*wet[c]*carrier;out2+=wet[c]*wet[c];}
        // Loudness compensation: the carrier makes the level pump at RM-FREQ (a tremolo).
        // Track input and output power (~8 ms) and restore the input level, so the ring
        // colour stays but the overall volume stays where the master level puts it.
        ringPowIn_+=ringPow*(dry2-ringPowIn_);
        ringPowOut_+=ringPow*(out2-ringPowOut_);
        const float want=std::sqrt((ringPowIn_+1.e-9f)/(ringPowOut_+1.e-9f));
        ringGain_+=ringGlide*(std::max(.25f,std::min(4.f,want))-ringGain_);
        for(int c=0;c<2;++c)wet[c]*=ringGain_;
      } else {ringPowIn_=ringPowOut_=0.f;ringGain_=1.f;}
      const bool enabled=target_[9]>0.f || smooth_[9]>1.e-6f;
#ifdef CREAM_GRIT_MASTER
      float mod[2],space[2],panned[2];
      const bool chorusOn=target_[16]>0.f||smooth_[16]>1.e-6f;
      chorus_.tick(wet,mod,smooth_[17],smooth_[18],chorusOn);
      for(int c=0;c<2;++c)wet[c]+=.35f*smooth_[16]*chorus_.readiness()*mod[c];
      const float repeats=echo_.tick((wet[0]+wet[1])*.5f,smooth_[11],enabled);
      reverb_.tick(wet,space,target_[19]>0.f||smooth_[19]>1.e-6f);
      pan_.tick(wet,panned,smooth_[15],smooth_[22],target_[23]>.5f,target_[15]>0.f||smooth_[15]>1.e-6f);
      for(int c=0;c<2;++c)out[2*n+c]=ceiling(gain*(panned[c]+smooth_[9]*repeats+smooth_[19]*space[c]));
#else
      const float repeats=echo_.tick((wet[0]+wet[1])*.5f,smooth_[11],enabled);
      for(int c=0;c<2;++c)out[2*n+c]=ceiling(gain*(wet[c]+smooth_[9]*repeats));
#endif
    }
  }
 private:
  // Full-band, stereo-linked sampler-style squash: peak detector, threshold falls and
  // ratio rises with amount, auto makeup, then a soft saturation for punch.
  void compress(float* w,float amount,float attack,float release) {
    const float level=std::max(std::fabs(w[0]),std::fabs(w[1]));
    compEnv_+=(level>compEnv_?attack:release)*(level-compEnv_);
    const float db=20.f*std::log10(compEnv_+1.e-9f);
    const float threshold=-4.f-26.f*amount, slope=1.f-1.f/(1.f+11.f*amount), knee=6.f;
    const float over=db-threshold;
    float reduction=0.f;
    if(over>knee*.5f) reduction=over*slope;
    else if(over>-knee*.5f){const float t=over+knee*.5f;reduction=slope*t*t/(2.f*knee);}
    const float makeup=-threshold*slope*.55f;
    const float g=std::pow(10.f,(makeup-reduction)*.05f);
    const float drive=1.f+1.5f*amount;
    for(int c=0;c<2;++c){
      const float y=w[c]*g;
      w[c]=y+amount*(saturate(y*drive)/drive*1.15f-y);
    }
  }
  float compEnv_=0.f,ringPhase_=0.f,ringPowIn_=0.f,ringPowOut_=0.f,ringGain_=1.f;
  float pole(float hz) const { return 1.f - std::exp(-6.28318530718f * hz / fs_); }
  static float lp(float x,float &state,float a) {
    state+=a*(x-state);
    if(std::fabs(state)<1.e-20f) state=0.f;
    return state;
  }
  float fs_=kRefFs,slew_=.0020811647f;
  int params_[kCount]={};
  Channel ch_[2];
  float target_[kCount]={},smooth_[kCount]={};
  float phase_=0.f,envelope_=0.f;
  uint32_t random_=0x2241200u;
  float noiseFilter_[2]={};
  float bpm_=120.f;
  SyncEcho echo_;
#ifdef CREAM_GRIT_MASTER
  SimpleChorus chorus_;
  SimpleReverb reverb_;
  RandomPan pan_;
#endif
  PitchShift pitch_;
  float pitchActive_=0.f;
};

}  // namespace creamvst
