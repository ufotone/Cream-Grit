#include "../src/cream-grit/cream_unit.h"
#include <memory>
#include <cstdio>
#include <cstring>
#include <vector>

int main(){
 int fails=0;
 auto check=[&](bool ok,const char* name){std::printf("%s %s\n",ok?"PASS":"FAIL",name);fails+=!ok;};
 static_assert(cream::kUiCount<=UNIT_MAX_PARAM_COUNT,"SDK parameter limit");
 auto u=std::unique_ptr<Delay>(new Delay);
 unit_runtime_desc_t desc={};desc.samplerate=48000;desc.input_channels=2;desc.output_channels=2;
 check(u->Init(&desc)==0,"48k stereo init");
 desc.samplerate=44100;check(u->Init(&desc)==k_unit_err_samplerate,"reject unsupported sample rate");desc.samplerate=48000;
 bool ok=true;
 for(uint8_t i=0;i<cream::kUiCount;++i){
   int k=cream::kUiMap[i];
   bool toggle=k==cream::kRing;
#ifdef CREAM_GRIT_MASTER
   toggle|=k==16;
#endif
   u->setParameter(i,9999);ok &= u->getParameterValue(i)==(toggle?1:cream::kMax[k]);
   u->setParameter(i,-9999);ok &= u->getParameterValue(i)==(toggle?0:cream::kMin[k]);
 }
 check(ok,"all UI controls map and clamp correctly");
 uint8_t mode=255,ring=255;
 for(uint8_t i=0;i<cream::kUiCount;++i){if(cream::kUiMap[i]==cream::kPMode)mode=i;if(cream::kUiMap[i]==cream::kRing)ring=i;}
 check(!std::strcmp(u->getParameterStrValue(mode,0),"DRUM")&&!std::strcmp(u->getParameterStrValue(mode,1),"SAMPLE")&&u->getParameterStrValue(mode,2)==nullptr,"two pitch modes, no CLEAN");
 check(!std::strcmp(u->getParameterStrValue(ring,0),"OFF")&&!std::strcmp(u->getParameterStrValue(ring,1),"ON"),"RING toggle labels");
 std::vector<float> in(48000*2),a(in.size()),b(in.size());
 for(int i=0;i<48000;++i)in[2*i]=in[2*i+1]=.2f*std::sin(i*.037f);
 auto ref=std::unique_ptr<cream::DSP>(new cream::DSP);
#ifdef CREAM_GRIT_MASTER
 const int presets=4;
#else
 const int presets=2;
#endif
 for(uint8_t preset=0;preset<presets;++preset)for(int on:{0,1}){
   u->LoadPreset(preset);u->setParameter(ring,on);u->setParameter(mode,1);u->Reset();
   int p[cream::kCount];std::copy(cream::kDefault,cream::kDefault+cream::kCount,p);
   for(uint8_t i=0;i<cream::kUiCount;++i){int k=cream::kUiMap[i];p[k]=k==cream::kRing?(on?50:0):u->getParameterValue(i);}
   p[cream::kRingHz]=on?230:0;
   p[14]=0;
#ifdef CREAM_GRIT_MASTER
   p[16]=p[16]>0?30:0;p[17]=8;p[18]=50;p[21]=60;
#endif
   ref->set(p);ref->reset();ref->process(in.data(),a.data(),48000);u->Process(in.data(),b.data(),48000);
   check(a==b,"preset, fixed parameters and RING toggle match DSP");
 }
 return fails?1:0;
}
