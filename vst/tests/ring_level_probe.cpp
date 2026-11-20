// Steady input (two sines + noise) through Cream Grit M with COMP off; reports the
// spread of 10 ms RMS (max-min, dB) and the mean level, per RING / RM-FREQ setting.
#define CREAM_GRIT_MASTER 1
#include <cstdio>
#include <vector>
#include "../dsp/cream_dsp_sr.h"
using namespace creamvst;
int main(){
  const int fs=48000,frames=fs*4;uint32_t r=1;
  std::vector<float> in(frames*2),out(frames*2);
  for(int n=0;n<frames;++n){r^=r<<13;r^=r>>17;r^=r<<5;
    const float v=.3f*std::sin(6.2831853f*196.f*n/fs)+.2f*std::sin(6.2831853f*1250.f*n/fs)+.03f*((r>>8)*(1.f/8388608.f)-1.f);
    in[2*n]=in[2*n+1]=v;}
  struct C{int ring,hz;} cs[]={{0,0},{50,0},{25,2},{50,2},{50,8},{50,30},{25,230},{50,230}};
  for(auto c:cs){
    DSP* d=new DSP();d->prepare(fs);int p[kCount];for(int i=0;i<kCount;++i)p[i]=kDefault[i];
    p[kComp]=0;p[kRing]=c.ring;p[kRingHz]=c.hz;d->set(p);d->reset();d->process(in.data(),out.data(),frames);
    double lo=1e9,hi=-1e9,sum=0;int k=0;
    for(int n=fs;n+480<=frames;n+=480){double e=0;for(int i=0;i<480;++i)e+=out[2*(n+i)]*out[2*(n+i)];
      double db=10*std::log10(e/480+1e-12);lo=std::min(lo,db);hi=std::max(hi,db);sum+=db;++k;}
    std::printf("RING %2d%% RM-FREQ %3d Hz: level %6.2f dB, 10ms spread %5.2f dB\n",c.ring,c.hz,sum/k,hi-lo);
    delete d;}
}
