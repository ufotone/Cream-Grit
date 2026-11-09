#include "unit.h"
#include <cstdio>
#include <cstring>
#include <cmath>
static int failures=0;
void check(bool ok,const char* name) {std::printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)++failures;}
int main() {
 unit_runtime_desc_t d={};d.target=unit_header.target;d.api=UNIT_API_VERSION;
 d.samplerate=48000;d.frames_per_buffer=64;d.input_channels=d.output_channels=2;
 check(unit_init(nullptr)==k_unit_err_undef,"null descriptor rejected");
 auto bad=d;bad.target=0;check(unit_init(&bad)==k_unit_err_target,"wrong slot rejected");
 bad=d;bad.api=0;check(unit_init(&bad)==k_unit_err_api_version,"incompatible API rejected");
 bad=d;bad.samplerate=44100;check(unit_init(&bad)==k_unit_err_samplerate,"wrong sample rate rejected");
 bad=d;bad.input_channels=1;check(unit_init(&bad)==k_unit_err_geometry,"wrong channel geometry rejected");
 check(unit_init(&d)==k_unit_err_none,"48k stereo unit initializes");
#ifdef CREAM_GRIT_MASTER
 const int count=24;const int presets=4;
#else
 const int count=15;const int presets=2;
#endif
 check(unit_header.num_params==count && unit_header.num_presets==presets,"metadata matches parameter and preset counts");
 bool defaults=true;for(int i=0;i<count;++i) defaults &= unit_get_param_value(i)==unit_header.params[i].init;
 check(defaults,"parameter defaults match header");
 check(unit_get_param_value(14)==1 && unit_get_param_value(13)==100,"FULL replacement scope and 100% pitch mix are defaults");
 check(unit_get_param_value(8)==10 && unit_get_param_str_value(9,0)==nullptr,"FM radio default is 10; BAND selector removed");
 check(std::strcmp(unit_get_param_str_value(10,5),"1/8")==0 && std::strcmp(unit_get_param_str_value(10,7),"1/8D")==0,"NOTE displays straight and dotted divisions");
 check(std::strcmp(unit_get_param_str_value(14,0),"HIGH")==0 && std::strcmp(unit_get_param_str_value(14,1),"FULL")==0,"pitch scope displays HIGH and FULL");
 unit_set_param_value(12,-100);unit_set_param_value(13,200);
 check(unit_get_param_value(12)==-36&&unit_get_param_value(13)==100,"pitch range and mix clamp to their limits");
 unit_load_preset(1);check(unit_get_param_value(1)==-13 && unit_get_param_value(5)==12 && unit_get_preset_index()==1,"Down Dirt loads virtual clock -13 and 12bit");
 check(std::strcmp(unit_get_preset_name(0),"12bit")==0 && std::strcmp(unit_get_preset_name(1),"Down Dirt")==0,"preset names valid");
#ifdef CREAM_GRIT_MASTER
 check(unit_get_param_value(15)==0&&unit_get_param_str_value(15,1)==nullptr,"main pan defaults to off; stereo echo selector is absent");
 check(std::strcmp(unit_get_param_str_value(23,0),"SINE")==0&&std::strcmp(unit_get_param_str_value(23,1),"SQUARE")==0,"pan waveform labels valid");
 unit_load_preset(2);
 check(unit_get_preset_index()==2&&unit_get_param_value(16)>0&&unit_get_param_value(19)>0,"Wide Space enables chorus and reverb");
 unit_load_preset(3);
 check(unit_get_preset_index()==3&&unit_get_param_value(15)==60&&unit_get_param_value(22)==10,"Random Drift enables main pan");
 unit_load_preset(1);
#endif
 unit_load_preset(99);check(unit_get_preset_index()==1,"invalid preset ignored");
 unit_set_param_value(0,10000);unit_set_param_value(5,-100);
 check(unit_get_param_value(0)==100 && unit_get_param_value(5)==6,"parameter writes clamped");
 unit_set_param_value(255,123);check(unit_get_param_value(255)==0,"invalid parameter ignored");
 unit_load_preset(0);unit_resume();float audio[128]={};unit_render(audio,audio,64);
 bool zero=true;for(float v:audio)zero &= v==0;check(zero,"in-place silence valid");
 for(int i=0;i<128;++i)audio[i]=.2f*std::sin(i*.31f);
 unit_render(audio,audio,64);bool finite=true;for(float v:audio)finite &= std::isfinite(v)&&std::fabs(v)<1;
 check(finite,"in-place stereo render valid");
 unit_reset();std::memset(audio,0,sizeof(audio));unit_render(audio,audio,64);
 zero=true;for(float v:audio)zero &= v==0;check(zero,"unit reset removes audio state");
 unit_set_param_value(0,0);unit_set_param_value(2,0);unit_set_param_value(8,0);
 unit_set_param_value(9,100);unit_set_param_value(10,5);unit_set_param_value(11,0);
 unit_set_tempo((90u<<16)+32768u);unit_reset();
 int at=0;float peak=0;
 for(int n=0;n<17000;++n){float in[2]={n==0?.2f:0.f,n==0?.2f:0.f},out[2];unit_render(in,out,1);if(n>10&&out[0]>peak){peak=out[0];at=n;}}
 check(peak>.02f && std::fabs(at-48000.f*30.f/90.5f)<2.f,"Q16.16 host tempo reaches audio delay, including fractional BPM");
 unit_suspend();unit_teardown();return failures?1:0;
}
