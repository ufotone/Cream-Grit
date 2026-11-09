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
  auto mono=std::make_unique<cream::SyncEcho>();
  auto core=std::make_unique<cream::DSP>();
  int monoParams[cream::kCount]={0,0,0,55,350,12,0,0,0,100,2,0,0,100,1,0,0,8,40,0,18,60};
  core->set(monoParams);core->reset();bool same=true;float peak=0;
  for(int n=0;n<10000;++n){float in[2]={n==0?.2f:0.f,0},out[2];core->process(in,out,1);if(n>10){same &= out[0]==out[1];peak=std::max(peak,std::fabs(out[0]));}}
  check(same&&peak>.02f,"left-only source produces a centered mono echo return");
  mono->setTiming(119.99998f,.5f);mono->reset();peak=0;
  for(int n=0;n<12010;++n)peak=std::max(peak,std::fabs(mono->tick(n==0?.2f:0.f,0,true)));
  check(peak>.065f&&peak<.08f,"fractional tempo near a ring boundary does not read past the buffer");
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
  auto reverb=std::make_unique<cream::SimpleReverb>();reverb->configure(8,0);reverb->reset();
  for(int n=0;n<20000;++n){float in[2]={.5f,.3f};reverb->tick(in,out,true);}
  reverb->reset();clean=true;for(int n=0;n<20000;++n){reverb->tick(zero,out,true);clean &= out[0]==0&&out[1]==0;}
  check(clean,"reverb reset invalidates every delay and diffuser line");
  V input(96000*2);for(int n=0;n<96000;++n){input[2*n]=.2f*std::sin(n*.057595865f);input[2*n+1]=.15f*std::cos(n*.0863938f);}
  check(render(input,1,true)==render(input,257,true),"all new effects remain independent of callback size");
  auto silence=render(V(96000,0),64,true);bool quiet=true;for(float v:silence)quiet &= v==0;
  check(quiet,"Wide Space does not create sound without input");
  auto pan=std::make_unique<cream::RandomPan>();pan->reset();
  float dry[2]={.1f,-.05f},moved[2];pan->tick(dry,moved,1.f,2.f,false,false);
  check(moved[0]==dry[0]&&moved[1]==dry[1],"pan bypass preserves the original stereo signal exactly");
  V sinePan(48000*4),squarePan(48000*4);
  bool left=false,right=false;float firstHold=0,lastHold=0,nextHold=0;
  for(int wave=0;wave<2;++wave){
    pan->reset();
    for(int n=0;n<48000*4;++n){
      float source[2]={.1f,.1f};pan->tick(source,moved,.9f,2.f,wave!=0,true);
      const float l=moved[0]*moved[0],r=moved[1]*moved[1],position=(r-l)/(l+r);
      (wave?squarePan:sinePan)[n]=position;
      if(wave){left |= position<-.2f;right |= position>.2f;if(n==6000)firstHold=position;if(n==18000)lastHold=position;if(n==30000)nextHold=position;}
    }
  }
  check(left&&right,"random pan visits both left and right positions");
  check(std::fabs(firstHold-lastHold)<1.e-5f&&std::fabs(nextHold-lastHold)>.05f,"square pan holds a position and selects a new target each cycle");
  float sineStep=0,squareStep=0;
  for(size_t i=1;i<sinePan.size();++i){sineStep=std::max(sineStep,std::fabs(sinePan[i]-sinePan[i-1]));squareStep=std::max(squareStep,std::fabs(squarePan[i]-squarePan[i-1]));}
  check(squareStep>sineStep*10&&sineStep<.002f,"SINE moves smoothly while SQUARE switches with short click suppression");
  pan->reset();bool repeatable=true;
  for(int n=0;n<48000*4;++n){float source[2]={.1f,.1f};pan->tick(source,moved,.9f,2.f,false,true);const float l=moved[0]*moved[0],r=moved[1]*moved[1];repeatable &= (r-l)/(l+r)==sinePan[n];}
  check(repeatable,"random pan resets deterministically");
  check(sizeof(cream::RandomPan)<=64,"random pan has no audio delay buffer");
  V pulse(48000*4*2,0);for(int n=0;n<12000;++n)pulse[2*n]=pulse[2*n+1]=.1f*std::sin(n*.057595865f);
  auto tails=[&](int amount){
    auto d=std::make_unique<cream::DSP>();int p[cream::kCount];for(int i=0;i<cream::kCount;++i)p[i]=cream::kWide[i];
    p[15]=amount;p[19]=50;p[20]=40;p[23]=1;d->set(p);d->reset();V result(pulse.size());d->process(pulse.data(),result.data(),pulse.size()/2);return result;
  };
  auto fixedTail=tails(0),movingTail=tails(90);bool sameTail=true;double tailEnergy=0;
  for(size_t i=72000*2;i<fixedTail.size();++i){sameTail &= fixedTail[i]==movingTail[i];tailEnergy+=double(fixedTail[i])*fixedTail[i];}
  check(sameTail&&tailEnergy>1.e-10,"pan moves the main sound without moving echo and reverb tails");
  check(cream::SyncEcho::kBytes+cream::PitchShift::kBytes+cream::SimpleChorus::kBytes+cream::SimpleReverb::kBytes==150084&&sizeof(cream::DSP)<163840,"all audio buffers total 150084 bytes; complete DSP fits below 160KiB");
  auto dsp=std::make_unique<cream::DSP>();int p[cream::kCount];
  for(int i=0;i<cream::kCount;++i)p[i]=cream::kWide[i];
  p[9]=100;p[11]=85;p[12]=-24;p[15]=100;p[16]=100;p[17]=30;p[18]=100;p[19]=100;p[20]=80;p[21]=0;p[22]=80;p[23]=1;
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
  check(finite&&bounded,"pitch, random pan, mono echo, chorus and reverb stay finite and bounded on hot input");
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  std::printf("Stress render: %d audio seconds in %.1f ms on this runner; DSP object %zu bytes\n",seconds,ms,sizeof(cream::DSP));
  return failures?1:0;
}
