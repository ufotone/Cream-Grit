#include "unit.h"
#include "cream_dsp.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

static int failures=0;
static void check(bool ok,const char* name) {
  std::printf("%s %s\n",ok?"PASS":"FAIL",name);
  if(!ok)++failures;
}

int main() {
  unit_runtime_desc_t desc={};
  desc.target=UNIT_TARGET_PLATFORM|k_unit_module_masterfx;
  desc.api=UNIT_API_VERSION;desc.samplerate=48000;
  desc.frames_per_buffer=64;desc.input_channels=4;desc.output_channels=2;
  check(unit_header.target==desc.target && unit_header.unit_id==0x43524732U,"master slot and separate unit identity");
  check(std::strcmp(unit_header.name,"Cream Grit M")==0,"master display name");
  check(unit_init(&desc)==k_unit_err_none,"four-channel master input initializes");
  auto bad=desc;bad.input_channels=3;
  check(unit_init(&bad)==k_unit_err_geometry,"invalid input stride rejected");
  bad=desc;bad.output_channels=4;
  check(unit_init(&bad)==k_unit_err_geometry,"non-stereo output rejected");
  bad=desc;bad.target=UNIT_TARGET_PLATFORM|k_unit_module_delfx;
  check(unit_init(&bad)==k_unit_err_target,"delay runtime rejected");
  unit_init(&desc);
  for(int i=0;i<cream::kCount;++i)unit_set_param_value(i,cream::kDefault[i]);
  unit_set_param_value(0,0);unit_set_param_value(2,0);unit_set_param_value(8,0);
  unit_reset();
  float packed[256],out[130];
  for(int n=0;n<64;++n) {
    packed[4*n]=.1f*std::sin(n*.13f);packed[4*n+1]=-.2f*std::cos(n*.11f);
    packed[4*n+2]=10.f;packed[4*n+3]=-10.f;
  }
  out[0]=out[129]=123.f;unit_render(packed,out+1,64);
  bool exact=true;
  for(int n=0;n<64;++n)exact &= out[1+2*n]==packed[4*n]&&out[2+2*n]==packed[4*n+1];
  check(exact,"main L/R pass at unity without sidechain leakage");
  check(out[0]==123.f&&out[129]==123.f,"render respects stereo output bounds");
  unit_reset();unit_render(packed,packed,64);
  exact=true;for(int n=0;n<128;++n)exact &= packed[n]==out[n+1];
  check(exact,"in-place four-to-two channel compaction");
  unit_load_preset(0);unit_reset();
  for(int n=0;n<64;++n){packed[4*n]=packed[4*n+1]=0;packed[4*n+2]=1;packed[4*n+3]=-1;}
  unit_render(packed,out+1,64);
  exact=true;for(int n=1;n<=128;++n)exact &= out[n]==0;
  check(exact,"sidechain-only input stays silent with default processing");

  const int count=40000;
  std::vector<float> stereo(count*2),quad(count*4),actual(count*2),expected(count*2);
  for(int n=0;n<count;++n) {
    stereo[2*n]=.2f*std::sin(n*.057595865f);
    stereo[2*n+1]=.15f*std::cos(n*.11519173f);
    quad[4*n]=stereo[2*n];quad[4*n+1]=stereo[2*n+1];
    quad[4*n+2]=.9f;quad[4*n+3]=-.7f;
  }
  int p[cream::kCount]={80,-13,40,50,350,12,35,0,10,35,7,25,-12,100,1};
  for(int i=0;i<cream::kCount;++i)unit_set_param_value(i,p[i]);
  unit_set_tempo((93u<<16)+16384u);unit_reset();
  cream::DSP reference;reference.set(p);reference.setTempo(93.25f);reference.reset();
  for(int n=0;n<count;n+=64) {
    const int frames=std::min(64,count-n);
    unit_render(quad.data()+4*n,actual.data()+2*n,frames);
    reference.process(stereo.data()+2*n,expected.data()+2*n,frames);
  }
  check(actual==expected,"master stride matches original DSP with pitch, noise and tempo delay active");
  for(int n=0;n<count;++n){quad[4*n+2]=-.4f;quad[4*n+3]=.3f;}
  unit_reset();
  for(int n=0;n<count;n+=37)unit_render(quad.data()+4*n,actual.data()+2*n,std::min(37,count-n));
  check(actual==expected,"sidechain changes and callback sizes do not change main output");
  desc.input_channels=2;
  check(unit_init(&desc)==k_unit_err_none,"two-channel runtime compatibility");
  for(int i=0;i<cream::kCount;++i)unit_set_param_value(i,p[i]);
  unit_reset();
  for(int n=0;n<count;n+=64)unit_render(stereo.data()+2*n,actual.data()+2*n,std::min(64,count-n));
  check(actual==expected,"stereo compatibility path matches master output");
  check(cream::SyncEcho::kBytes+cream::PitchShift::kBytes==128772,"audio buffers remain 128772 bytes");
  unit_teardown();
  return failures?1:0;
}
