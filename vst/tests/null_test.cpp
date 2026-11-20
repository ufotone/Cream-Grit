// Compares the original drumlogue DSP with the VST port at 48 kHz (must be bit-identical)
// and checks the port stays finite and bounded at other sample rates.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../../src/cream-grit/cream_dsp.h"
#include "../dsp/cream_dsp_sr.h"

// MSVC: exact. Clang constant-folds exp()/pow() on the original's constexpr 48 kHz paths
// but calls libm at run time in the port, so allow a few ulps of difference there.
#ifndef NULL_TOLERANCE
#define NULL_TOLERANCE 0.0
#endif
static float noise(uint32_t& s){s^=s<<13;s^=s>>17;s^=s<<5;return (s>>8)*(1.f/8388608.f)-1.f;}

static void fill(std::vector<float>& buf,size_t frames,float fs){
  buf.resize(frames*2);uint32_t s=12345;
  for(size_t n=0;n<frames;++n){
    const float t=n/fs, env=(n%12000)<2000?1.f:.2f;
    const float a=.45f*std::sin(6.2831853f*110.f*t)+.25f*std::sin(6.2831853f*1730.f*t);
    buf[2*n]=env*(a+.1f*noise(s));buf[2*n+1]=env*(.8f*a+.1f*noise(s));
  }
}

// Parameter scenes applied every `block` frames: presets plus pitch / echo / extreme settings.
static std::vector<std::vector<int>> scenes(){
  std::vector<std::vector<int>> v;
  // VST-only params (COMP 0, P-MODE DRUMLOGUE) set so the port must match the original exactly.
  for(int p=0;p<creamvst::kPresets;++p){const int* q=creamvst::presetValues(p);v.emplace_back(q,q+creamvst::kCount);v.back()[creamvst::kComp]=0;v.back()[creamvst::kPMode]=2;v.back()[creamvst::kRing]=0;}
  auto s=v[0];s[12]=7;s[13]=80;s[9]=60;s[11]=70;s[10]=2;v.push_back(s);
  s[12]=-24;s[14]=0;s[1]=-24;s[5]=6;s[6]=100;s[7]=6;v.push_back(s);
#ifdef CREAM_GRIT_MASTER
  s[15]=100;s[16]=100;s[17]=30;s[18]=100;s[19]=100;s[20]=80;s[21]=0;s[22]=80;s[23]=1;v.push_back(s);
#endif
  return v;
}
// Scenes exercising the VST-only features (only checked for sanity, not against the original).
static std::vector<std::vector<int>> extraScenes(){
  auto v=scenes();
  for(auto& s:v){s[creamvst::kComp]=100;s[creamvst::kPMode]=1;s[creamvst::kRing]=30;s[creamvst::kRingHz]=5;s[12]=s[12]?s[12]:-12;}
  return v;
}

int main(){
  int fails=0;
  {  // bit-exact at 48 kHz
    const size_t frames=48000*6,block=64;
    std::vector<float> in,a(frames*2),b(frames*2);fill(in,frames,48000.f);
    auto* ref=new cream::DSP();auto* port=new creamvst::DSP();
    port->prepare(48000.f);
    auto sc=scenes();
    ref->set(sc[0].data());port->set(sc[0].data());ref->reset();port->reset();
    double maxDiff=0;size_t first=SIZE_MAX;
    for(size_t pos=0;pos<frames;pos+=block){
      const size_t idx=(pos/24000)%sc.size();
      ref->set(sc[idx].data());port->set(sc[idx].data());
      const float bpm=(pos/48000)%2?97.f:140.f;ref->setTempo(bpm);port->setTempo(bpm);
      ref->process(&in[2*pos],&a[2*pos],block);port->process(&in[2*pos],&b[2*pos],block);
    }
    for(size_t i=0;i<frames*2;++i){double d=std::fabs(double(a[i])-b[i]);if(d>maxDiff)maxDiff=d;if(d>0&&first==SIZE_MAX)first=i;}
    std::printf("48k null: max diff %.3g (first mismatch %lld)\n",maxDiff,first==SIZE_MAX?-1LL:(long long)first);
    if(maxDiff>NULL_TOLERANCE)++fails;
    delete ref;delete port;
  }
  for(float fs:{44100.f,88200.f,96000.f,192000.f}){
    const size_t frames=size_t(fs)*4,block=512;
    std::vector<float> in,out(frames*2);fill(in,frames,fs);
    auto* port=new creamvst::DSP();port->prepare(fs);
    auto sc=extraScenes();double peak=0,rms=0;bool finite=true;
    for(size_t pos=0;pos+block<=frames;pos+=block){
      port->set(sc[(pos/size_t(fs/2))%sc.size()].data());port->setTempo(120.f);
      port->process(&in[2*pos],&out[2*pos],block);
    }
    for(float v:out){if(!std::isfinite(v))finite=false;peak=std::max(peak,double(std::fabs(v)));rms+=v*v;}
    rms=std::sqrt(rms/out.size());
    std::printf("%6.0f Hz: finite=%d peak=%.3f rms=%.3f\n",fs,finite,peak,rms);
    if(!finite||peak>1.0||rms<1e-3)++fails;
    delete port;
  }
  std::printf(fails?"FAILED\n":"OK\n");
  return fails;
}
