#include "../src/cream-grit/cream_dsp.h"
#include "reference/src/cream-grit/cream_dsp.h"
#include "../vst/dsp/cream_dsp_sr.h"
#include <vector>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <chrono>

int main() {
  int fails=0;
  auto check=[&](bool ok,const char* name){std::printf("%s %s\n",ok?"PASS":"FAIL",name);fails+=!ok;};
  constexpr int frames=96000;
  std::vector<float> in(frames*2),a(frames*2),b(frames*2);
  unsigned rng=12345;
  for(int n=0;n<frames;++n){
    rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
    float env=n%12000<1500?.8f:.12f;
    in[2*n]=env*(.6f*std::sin(.0144f*n)+.2f*std::sin(.19f*n)+(rng>>8)*(1.f/8388608.f)-1.f);
    in[2*n+1]=.7f*in[2*n]+.05f*std::sin(.031f*n);
  }
  auto d=std::unique_ptr<cream::DSP>(new cream::DSP);
  auto ref=std::unique_ptr<creamref::DSP>(new creamref::DSP);
  auto vst=std::unique_ptr<creamvst::DSP>(new creamvst::DSP);
  vst->prepare(48000);
  int p[cream::kCount],q[creamvst::kCount];
  auto defaults=[&](){std::copy(cream::kDefault,cream::kDefault+cream::kCount,p);};
  auto difference=[&](){double peak=0,err=0,power=0;for(size_t i=0;i<a.size();++i){double e=a[i]-b[i];peak=std::max(peak,std::fabs(e));err+=e*e;power+=double(b[i])*b[i];}std::printf("peak difference %.8g, relative RMS %.3f dB\n",peak,10*std::log10((err+1e-30)/(power+1e-30)));return peak;};
  // Existing processing must remain bit-exact when the new features are off.
  defaults();p[12]=-7;p[9]=40;p[11]=65;
#ifdef CREAM_GRIT_MASTER
  p[15]=50;p[16]=35;p[19]=25;
#endif
  d->set(p);d->reset();ref->set(p);ref->reset();
  for(int n=0;n<frames;n+=64){d->process(&in[2*n],&a[2*n],64);ref->process(&in[2*n],&b[2*n],64);}
  check(difference()==0,"legacy DSP bit-exact with additions disabled");
  // SAMPLE is the 48 kHz VST algorithm, including CLOCK interaction.
  defaults();p[12]=-12;p[1]=-13;p[cream::kPMode]=1;
  std::copy(p,p+cream::kCount,q);q[creamvst::kPMode]=1;
  d->set(p);d->reset();vst->set(q);vst->reset();
  d->process(in.data(),a.data(),frames);vst->process(in.data(),b.data(),frames);
  check(difference()<1e-6,"SAMPLE matches VST at 48 kHz");
  // Measure the control-rate compressor / polynomial carrier approximation.
  defaults();p[cream::kComp]=100;p[cream::kRing]=50;p[cream::kRingHz]=230;
  std::copy(p,p+cream::kCount,q);q[creamvst::kPMode]=2;
  d->set(p);d->reset();vst->set(q);vst->reset();
  d->process(in.data(),a.data(),frames);vst->process(in.data(),b.data(),frames);
  check(difference()<.04,"COMP/RING approximation has bounded peak error on transient input");
  // All combinations, including live mode changes, maximum spatial FX, reset and stride 4.
  bool finite=true,identical=true;double peak=0;
  for(int mode:{0,1}) for(int pitch:{-36,-12,0,7,24}) for(int clock:{-24,0}){
    defaults();p[cream::kPMode]=mode;p[12]=pitch;p[1]=clock;p[cream::kComp]=100;p[cream::kRing]=50;p[cream::kRingHz]=230;
    p[9]=100;p[11]=85;p[7]=6;
#ifdef CREAM_GRIT_MASTER
    p[15]=100;p[16]=100;p[19]=100;
#endif
    d->set(p);d->process(in.data(),a.data(),frames);
    for(float x:a){finite&=std::isfinite(x);peak=std::max(peak,double(std::fabs(x)));}
    d->reset();d->process(in.data(),a.data(),frames);
    std::vector<float> in4(frames*4);
    for(int n=0;n<frames;++n){in4[4*n]=in[2*n];in4[4*n+1]=in[2*n+1];in4[4*n+2]=in4[4*n+3]=99;}
    d->reset();for(int n=0;n<frames;n+=64)d->process(&in4[4*n],&b[2*n],64,4);
    identical&=a==b;
  }
  check(finite&&peak<=1,"extreme settings and live mode changes finite, bounded");
  check(identical,"reset reproducible; block size and four-channel stride identical");
  std::printf("sizeof DSP: %zu, original %zu, delta %lld bytes\n",sizeof(cream::DSP),sizeof(creamref::DSP),(long long)sizeof(cream::DSP)-(long long)sizeof(creamref::DSP));
  // Relative desktop timing is only a regression indicator, never a hardware CPU guarantee.
  auto bench=[&](auto& dsp){auto t=std::chrono::steady_clock::now();for(int i=0;i<8;++i)dsp.process(in.data(),a.data(),frames);return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();};
  std::printf("desktop timing (16 seconds audio): new %.2f ms, original %.2f ms\n",bench(*d),bench(*ref));
  return fails?1:0;
}
