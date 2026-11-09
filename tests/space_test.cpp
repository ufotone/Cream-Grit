#include "cream_dsp.h"
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

using V=std::vector<float>;
static int failures=0;
static void check(bool ok,const char* name){std::printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)++failures;}
static V impulseReverb(float decay,float damp){
  auto r=std::make_unique<cream::SimpleReverb>();r->configure(decay,damp);r->reset();
  V result(48000*3*2);
  for(int n=0;n<48000*3;++n){float in[2]={n==0?1.f:0.f,n==0?1.f:0.f};r->tick(in,result.data()+2*n,true);}
  return result;
}
static double energy(const V& x,int begin,int end){double e=0;for(int i=begin*2;i<end*2;++i)e+=double(x[i])*x[i];return e;}
static double brightness(const V& x){double e=0,d=0;for(size_t i=2;i<x.size();++i){e+=double(x[i])*x[i];double delta=x[i]-x[i-2];d+=delta*delta;}return std::sqrt(d/e);}
static V render(const V& input,int block,bool wide){
  auto dsp=std::make_unique<cream::DSP>();
  dsp->set(wide?cream::kWide:cream::kDefault);dsp->setTempo(92);dsp->reset();V out(input.size());
  for(size_t n=0;n<input.size()/2;n+=block)dsp->process(input.data()+2*n,out.data()+2*n,std::min(size_t(block),input.size()/2-n));
  return out;
}
int main(){
  auto echo=std::make_unique<cream::MasterEcho>();
  auto mono=std::make_unique<cream::SyncEcho>();
  echo->setTiming(120,.25f);echo->setStereo(false);echo->reset();mono->setTiming(120,.25f);mono->reset();
  bool same=true;
  for(int n=0;n<24000;++n){float in[2]={.2f*std::sin(n*.0576f),.1f*std::cos(n*.093f)},out[2];echo->tick(in,out,.4f,true);float old=mono->tick((in[0]+in[1])*.5f,.4f,true);same &= out[0]==old&&out[1]==old;}
  check(same,"MONO echo matches the previous mono algorithm exactly");
  echo->setStereo(true);echo->reset();bool rightSilent=true;float peak=0;
  for(int n=0;n<10000;++n){float in[2]={n==0?1.f:0.f,0},out[2];echo->tick(in,out,0,true);rightSilent &= out[1]==0;peak=std::max(peak,std::fabs(out[0]));}
  check(rightSilent&&peak>.1f,"STEREO echo preserves independent left and right repeats");
  echo->setTiming(119.99998f,.5f);echo->reset();peak=0;
  for(int n=0;n<12010;++n){float in[2]={n==0?.2f:0.f,0},out[2];echo->tick(in,out,0,true);peak=std::max(peak,std::fabs(out[0]));}
  check(peak>.065f&&peak<.08f,"fractional tempo near a ring boundary does not read past the buffer");
  echo->setStereo(false);bool clears=true;
  for(int n=0;n<10000;++n){float in[2]={},out[2];echo->tick(in,out,.8f,true);if(n>600)clears &= out[0]==0&&out[1]==0;}
  check(clears,"echo mode switching fades and invalidates previous tails");
  auto chorus=std::make_unique<cream::SimpleChorus>();chorus->reset();double difference=0;
  for(int n=0;n<96000;++n){float s=.2f*std::sin(n*.057595865f),in[2]={s,s},out[2];chorus->tick(in,out,.8f,.8f,true);if(n>48000)difference+=(out[0]-out[1])*(out[0]-out[1]);}
  check(difference>1,"chorus creates stereo modulation from a mono source");
  float zero[2]={},out[2];chorus->tick(zero,out,.8f,.8f,false);bool clean=true;
  for(int n=0;n<5000;++n){chorus->tick(zero,out,.8f,.8f,true);clean &= out[0]==0&&out[1]==0;}
  check(clean,"chorus bypass clears old samples without a buffer memset");
  auto shortTail=impulseReverb(.3f,.5f),longTail=impulseReverb(4.f,.5f);
  double early=energy(longTail,4800,24000),late=energy(longTail,120000,144000),width=0;
  for(int n=4800;n<48000;++n){double d=longTail[2*n]-longTail[2*n+1];width+=d*d;}
  std::printf("Reverb early %.9g, late %.9g, stereo difference %.9g\n",early,late,width);
  check(early>1.e-6&&late<early*.2,"reverb produces a decaying tail");
  check(width>early*.01,"reverb tail is stereo");
  check(energy(longTail,33600,62400)>energy(shortTail,33600,62400)*5+1.e-10,"R-DECAY changes tail duration");
  auto bright=impulseReverb(2.f,0),dark=impulseReverb(2.f,1);
  std::printf("Reverb brightness: bright %.6f, dark %.6f\n",brightness(bright),brightness(dark));
  check(brightness(dark)<brightness(bright)*.9,"R-DAMP reduces high-frequency tail energy");
  auto r=std::make_unique<cream::SimpleReverb>();r->configure(8,0);r->reset();
  for(int n=0;n<20000;++n){float in[2]={.5f,.3f};r->tick(in,out,true);}
  r->reset();clean=true;for(int n=0;n<20000;++n){r->tick(zero,out,true);clean &= out[0]==0&&out[1]==0;}
  check(clean,"reverb reset invalidates every delay and diffuser line");
  V input(96000*2);for(int n=0;n<96000;++n){input[2*n]=.2f*std::sin(n*.057595865f);input[2*n+1]=.15f*std::cos(n*.0863938f);}
  check(render(input,1,true)==render(input,257,true),"all new effects remain independent of callback size");
  auto silence=render(V(96000,0),64,true);bool quiet=true;for(float v:silence)quiet &= v==0;
  check(quiet,"Wide Space does not create sound without input");
  check(cream::MasterEcho::kBytes+cream::PitchShift::kBytes+cream::SimpleChorus::kBytes+cream::SimpleReverb::kBytes==246088&&sizeof(cream::DSP)<262144,"all audio buffers total 246088 bytes; complete DSP fits below 256KiB");
  auto dsp=std::make_unique<cream::DSP>();int p[cream::kCount];
  for(int i=0;i<cream::kCount;++i)p[i]=cream::kWide[i];
  p[9]=100;p[11]=85;p[12]=-24;p[16]=100;p[17]=30;p[18]=100;p[19]=100;p[20]=80;p[21]=0;
  dsp->set(p);dsp->reset();bool finite=true,bounded=true;uint32_t random=42;float hot[128],result[128];
#ifdef __arm__
  const int seconds=4;
#else
  const int seconds=20;
#endif
  auto start=std::chrono::steady_clock::now();
  for(int n=0;n<48000*seconds;n+=64){
    for(float& v:hot){random=1664525*random+1013904223;v=int32_t(random)/2147483648.f*4.f;}
    dsp->process(hot,result,64);
    for(float v:result){finite &= std::isfinite(v);bounded &= std::fabs(v)<=1;}
  }
  check(finite&&bounded,"pitch plus maximum stereo echo, chorus and reverb stay finite and bounded on hot input");
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  std::printf("Stress render: %d audio seconds in %.1f ms on this runner; DSP object %zu bytes\n",seconds,ms,sizeof(cream::DSP));
  return failures?1:0;
}
