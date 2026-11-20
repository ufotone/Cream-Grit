#pragma once
#include <atomic>
#include "unit.h"
#include "cream_dsp.h"
namespace cream {
#ifdef CREAM_GRIT_MASTER
// P-SCOPE=HIGH; CHORUS ON=30%/0.8Hz/50% depth; R-DAMP=60%; RING ON=50%/230Hz.
constexpr int kUiCount=23;
static const int kUiMap[kUiCount]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,15,16,19,20,22,23,kComp,kPMode,kRing};
#else
constexpr int kUiCount=17;
static const int kUiMap[kUiCount]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,kComp,kPMode,kRing};
#endif
static_assert(kUiCount<=UNIT_MAX_PARAM_COUNT,"drumlogue parameter limit");
}
class Delay {
 public:
  Delay() { for(int i=0;i<cream::kCount;++i) params_[i].store(cream::kDefault[i]); }
  int8_t Init(const unit_runtime_desc_t* d) {
    if(d->samplerate!=48000) return k_unit_err_samplerate;
#ifdef CREAM_GRIT_MASTER
    if((d->input_channels!=2 && d->input_channels!=4) || d->output_channels!=2) return k_unit_err_geometry;
    inputChannels_=d->input_channels;
#else
    if(d->input_channels!=2 || d->output_channels!=2) return k_unit_err_geometry;
#endif
    Reset(); return k_unit_err_none;
  }
  void Teardown() {}
  void Reset() { sync(); dsp_.reset(); }
  void Resume() { Reset(); }
  void Suspend() {}
  void Process(const float* in,float* out,size_t frames) {
    sync();
#ifdef CREAM_GRIT_MASTER
    if(inputChannels_==4){dsp_.process(in,out,frames,4);return;}
#endif
    dsp_.process(in,out,frames);
  }
  void setParameter(uint8_t i,int32_t v) {
    if(i>=cream::kUiCount)return;
    const int k=cream::kUiMap[i];
    if(k==cream::kRing){
      // One atomic toggle; frequency is derived in sync() on the audio thread.
      params_[k].store(v>0?50:0,std::memory_order_relaxed);
#ifdef CREAM_GRIT_MASTER
    } else if(k==16){
      params_[k].store(v>0?30:0,std::memory_order_relaxed);
#endif
    } else params_[k].store(std::max(cream::kMin[k],std::min(cream::kMax[k],int(v))),std::memory_order_relaxed);
  }
  int32_t getParameterValue(uint8_t i) const {
    if(i>=cream::kUiCount)return 0;
    const int k=cream::kUiMap[i];
    const int v=params_[k].load(std::memory_order_relaxed);
#ifdef CREAM_GRIT_MASTER
    if(k==16)return v>0?1:0;
#endif
    return k==cream::kRing?(v>0?1:0):v;
  }
  void SetTempo(uint32_t t) {if(t)tempo_.store(t,std::memory_order_relaxed);}
  const char* getParameterStrValue(uint8_t i,int32_t v) const {
    if(i>=cream::kUiCount)return nullptr;
    i=static_cast<uint8_t>(cream::kUiMap[i]);
    if(i==cream::kPMode)return v==0?"DRUM":v==1?"SAMPLE":nullptr;
    if(i==cream::kRing)return v==0?"OFF":v==1?"ON":nullptr;
    if(i==10)return cream::SyncEcho::noteName(v);
#ifdef CREAM_GRIT_MASTER
    if(i==16)return v==0?"OFF":v==1?"ON":nullptr;
    if(i==23)return v==0?"SINE":v==1?"SQUARE":nullptr;
#endif
    return i==14?(v==0?"HIGH":v==1?"FULL":nullptr):nullptr;
  }
  const uint8_t* getParameterBmpValue(uint8_t,int32_t) const { return nullptr; }
  void LoadPreset(uint8_t i) {
#ifdef CREAM_GRIT_MASTER
    if(i>3)return;
    const int* p=i==3?cream::kDrift:i==2?cream::kWide:i==1?cream::kDown:cream::kDefault;
#else
    if(i>1) return;
    const int* p=i?cream::kDown:cream::kDefault;
#endif
    for(int k=0;k<cream::kCount;++k) params_[k].store(p[k],std::memory_order_relaxed);
    preset_.store(i,std::memory_order_relaxed);
  }
  uint8_t getPresetIndex() const { return preset_.load(std::memory_order_relaxed); }
  static const char* getPresetName(uint8_t i) {
#ifdef CREAM_GRIT_MASTER
    if(i==2)return "Wide Space";
    if(i==3)return "Random Drift";
#endif
    return i==0?"12bit":i==1?"Down Dirt":nullptr;
  }
 private:
  void sync() {
    int p[cream::kCount];bool changed=!configured_;
    for(int i=0;i<cream::kCount;++i) {
      p[i]=params_[i].load(std::memory_order_relaxed);
      changed|=p[i]!=cached_[i];cached_[i]=p[i];
    }
    p[14]=0; // HIGH scope in both drumlogue editions.
#ifdef CREAM_GRIT_MASTER
    p[16]=p[16]>0?30:0;p[17]=8;p[18]=50;p[21]=60;
#endif
    p[cream::kRingHz]=p[cream::kRing]>0?230:0;
    if(changed){dsp_.set(p);configured_=true;}
    const uint32_t t=tempo_.load(std::memory_order_relaxed);
    if(t!=cachedTempo_){dsp_.setTempo(t*(1.f/65536.f));cachedTempo_=t;}
  }
  std::atomic<int32_t> params_[cream::kCount];
  std::atomic<uint8_t> preset_{0};
  std::atomic<uint32_t> tempo_{120u<<16};
  int cached_[cream::kCount]={};
  uint32_t cachedTempo_=0;
  bool configured_=false;
#ifdef CREAM_GRIT_MASTER
  uint8_t inputChannels_=4;
#endif
  cream::DSP dsp_;
};
