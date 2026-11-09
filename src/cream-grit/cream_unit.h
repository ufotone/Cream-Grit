#pragma once
#include <atomic>
#include "unit.h"
#include "cream_dsp.h"
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
    if(i<cream::kCount) params_[i].store(std::max(cream::kMin[i],std::min(cream::kMax[i],int(v))),std::memory_order_relaxed);
  }
  int32_t getParameterValue(uint8_t i) const { return i<cream::kCount?params_[i].load(std::memory_order_relaxed):0; }
  void SetTempo(uint32_t t) {if(t)tempo_.store(t,std::memory_order_relaxed);}
  const char* getParameterStrValue(uint8_t i,int32_t v) const {
    if(i==10)return cream::SyncEcho::noteName(v);
    return i==14?(v==0?"HIGH":v==1?"FULL":nullptr):nullptr;
  }
  const uint8_t* getParameterBmpValue(uint8_t,int32_t) const { return nullptr; }
  void LoadPreset(uint8_t i) {
    if(i>1) return;
    const int* p=i?cream::kDown:cream::kDefault;
    for(int k=0;k<cream::kCount;++k) params_[k].store(p[k],std::memory_order_relaxed);
    preset_.store(i,std::memory_order_relaxed);
  }
  uint8_t getPresetIndex() const { return preset_.load(std::memory_order_relaxed); }
  static const char* getPresetName(uint8_t i) { return i==0?"12bit":i==1?"Down Dirt":nullptr; }
 private:
  void sync() {
    int p[cream::kCount];bool changed=!configured_;
    for(int i=0;i<cream::kCount;++i) {
      p[i]=params_[i].load(std::memory_order_relaxed);
      changed|=p[i]!=cached_[i];cached_[i]=p[i];
    }
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
