// Single click in; reports the first output sample that responds and the peak position,
// with and without the ring modulator, to show whether RING adds latency.
#define CREAM_GRIT_MASTER 1
#include <cstdio>
#include <vector>
#include "../dsp/cream_dsp_sr.h"
using namespace creamvst;
int main(){
  const int fs=48000,frames=fs;
  for(int ring:{0,25,100}) for(int hz:{220,2000}){
    std::vector<float> in(frames*2,0.f),out(frames*2);
    const int at=24000; in[2*at]=in[2*at+1]=.9f;
    DSP* d=new DSP();d->prepare(fs);int p[kCount];for(int i=0;i<kCount;++i)p[i]=kDefault[i];
    p[kRing]=ring;p[kRingHz]=hz;d->set(p);d->reset();d->process(in.data(),out.data(),frames);
    int first=-1,peak=at;float pk=0;
    for(int n=at-100;n<frames;++n){float v=std::fabs(out[2*n]);if(first<0&&v>1e-4f)first=n;if(v>pk){pk=v;peak=n;}}
    std::printf("RING %3d%% %4d Hz: first response %+d samples, peak %+d samples (%.2f ms)\n",ring,hz,first-at,peak-at,(peak-at)*1000.0/fs);
    delete d;
  }
}
