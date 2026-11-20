// Feeds isolated clicks and reports output energy in time windows after each click,
// to locate echo-like repeats for combinations of COMP / P-MODE / PITCH.
#define CREAM_GRIT_MASTER 1
#include <cstdio>
#include <vector>
#include "../dsp/cream_dsp_sr.h"
using namespace creamvst;
int main(){
  const int fs=48000, period=fs, frames=fs*6;
  std::vector<float> in(frames*2,0.f),out(frames*2);
  for(int n=0;n<frames;n+=period) for(int k=0;k<48;++k){float v=(k%2?-.8f:.8f);in[2*(n+k)]=in[2*(n+k)+1]=v;}
  struct Case{const char* name;int comp,mode,pitch;} cases[]={
    {"orig (comp0 dl p0)",0,2,0},{"comp0 drumlogue p-5",0,2,-5},{"comp0 clean p-5",0,0,-5},{"comp0 clean p+7",0,0,7},{"comp0 sampler p-5",0,1,-5},
    {"comp45 clean p0",45,0,0},{"comp45 clean p-5",45,0,-5},{"comp45 sampler p-5",45,1,-5},{"comp45 sampler p+5",45,1,5}};
  std::printf("%-24s","window(ms)");const int edges[]={0,5,20,50,100,150,250,400,700};
  for(int i=0;i+1<9;++i)std::printf("%5d-%-4d",edges[i],edges[i+1]);std::printf("\n");
  for(auto& c:cases){
    DSP* d=new DSP();d->prepare(fs);
    int p[kCount];for(int i=0;i<kCount;++i)p[i]=kDefault[i];
    p[kComp]=c.comp;p[kPMode]=c.mode;p[12]=c.pitch;d->set(p);d->reset();
    d->process(in.data(),out.data(),frames);
    std::printf("%-24s",c.name);
    const int start=period*4; // after warm-up
    for(int i=0;i+1<9;++i){double e=0;int a=start+edges[i]*48,b=start+edges[i+1]*48;
      for(int n=a;n<b;++n)e+=out[2*n]*out[2*n];
      std::printf("%9.1f ",10*std::log10(e/(b-a)+1e-12));}
    std::printf("\n");delete d;
  }
}
