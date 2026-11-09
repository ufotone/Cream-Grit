#include "cream_dsp.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <cstdint>
using V=std::vector<float>;
using P=std::array<int,cream::kCount>;
static int failures=0;
void check(bool ok,const char* name) { std::printf("%s %s\n",ok?"PASS":"FAIL",name); if(!ok) ++failures; }
V sine(float hz,float amp=.25f,int seconds=2) {
  V x(seconds*48000*2);
  for(size_t i=0;i<x.size()/2;++i) x[2*i]=x[2*i+1]=static_cast<float>(amp*std::sin(6.283185307179586*hz*i/48000.));
  return x;
}
V render(const V& x,P p,int block=64,float bpm=120.f) {
  cream::DSP d; d.set(p.data());d.setTempo(bpm);d.reset(); V y(x.size());
  for(size_t i=0;i<x.size()/2;i+=block) d.process(x.data()+2*i,y.data()+2*i,std::min(size_t(block),x.size()/2-i));
  return y;
}
double rms(const V& x,size_t start=48000) {
  double s=0; for(size_t i=start;i<x.size();i+=2) s+=double(x[i])*x[i];
  return std::sqrt(s/((x.size()-start)/2));
}
double tone(const V& x,double hz) {
  double re=0,im=0; const size_t n=x.size()/2;
  for(size_t i=n/2;i<n;++i) {double a=6.283185307179586*hz*i/48000.;re+=x[2*i]*std::cos(a);im-=x[2*i]*std::sin(a);}
  return 2*std::sqrt(re*re+im*im)/(n-n/2);
}
double difference(const V&a,const V&b) {double e=0;for(size_t i=0;i<a.size();++i) {double d=a[i]-b[i];e+=d*d;}return std::sqrt(e/a.size());}
void tests() {
 P p={0,0,0,55,350,12,0,0};
 auto x=sine(1234);auto y=render(x,p);
 check(x==y,"GRIT=0 BODY=0 LEVEL=0 passes input exactly below limiter");
 check(render(V(96000,0),p)==V(96000,0),"digital silence stays zero");
 double worst=0;
 for(int hz:{40,60,100}) for(int clock:{0,-12,-24}) {
   auto bass=sine(float(hz));p[0]=100;p[1]=clock;
   const auto wet=render(bass,p);double db=20*std::log10(rms(wet)/rms(bass));worst=std::max(worst,std::fabs(db));
 }
 std::printf("Bass change 40/60/100 Hz across CLOCK 0/-12/-24: %.4f dB worst\n",worst);
 check(worst<.15,"low fundamentals protected from grit (<0.15 dB)");
 p={100,-12,0,100,350,12,0,0}; x=sine(10000);
 auto alias=render(x,p);p[1]=0;auto normal=render(x,p);
 const double folded=tone(alias,3020),reference=tone(normal,3020);
 std::printf("10 kHz -> 3020 Hz alias at CLOCK -12: %.6f; CLOCK 0: %.6f\n",folded,reference);
 check(folded>.01 && folded>reference*10,"lowering CLOCK generates a measurable fold component");
 p={100,0,0,55,350,12,0,0};x=sine(1234,.15f);
 auto b12=render(x,p);p[5]=16;auto b16=render(x,p);p[5]=6;auto b6=render(x,p);
 double e12=difference(b12,b16),e6=difference(b6,b16);
 std::printf("Quantization residual vs 16bit: 12bit %.8f; 6bit %.8f\n",e12,e6);
 check(e12>1.e-6 && e6>e12*10,"BITS changes real quantization, independently of CLOCK");
 p={65,-13,40,55,350,12,30,0};x=sine(5678);
 check(difference(render(x,p,1),render(x,p,257))<1.e-7,"render is independent of callback block size");
 V left=x;for(size_t i=1;i<left.size();i+=2)left[i]=0;
 y=render(left,p);bool silent=true;for(size_t i=1;i<y.size();i+=2)silent &= y[i]==0;
 check(silent,"no audio crosstalk into silent right channel");
 cream::DSP d;d.set(p.data());d.reset(); y.resize(x.size()); d.process(x.data(),y.data(),x.size()/2);d.reset();
 V zero(2048),out(2048);d.process(zero.data(),out.data(),1024);
 check(zero==out,"reset clears held samples and filter tails");
 p={0,0,0,55,350,12,0,0,10};x=sine(1000,.2f);
 auto fm=render(x,p);p[8]=0;auto off=render(x,p);
 check(off==x,"RADIO=0 removes noise exactly");
 const double relative=20*std::log10(difference(fm,off)/rms(off));
 std::printf("RADIO=10 FM noise relative to 1kHz sine: %.2f dB\n",relative);
 check(relative < -40 && relative > -70,"default radio layer is very quiet");
 p[8]=100;check(render(V(96000,0),p)==V(96000,0),"RADIO=100 generates no noise without input");
 p[8]=10;check(render(x,p)==render(x,p),"reset gives deterministic noise for repeatable comparisons");
 check(difference(render(x,p,1),render(x,p,257))<1.e-7,"radio render independent of block size");
 V tail=x;tail.resize(x.size()+48000*2,0);auto tailOut=render(tail,p);
 bool quiet=true;for(size_t i=tailOut.size()-960;i<tailOut.size();++i)quiet &= tailOut[i]==0;
 check(quiet,"noise fades away after input stops");
 bool timing=true;
 for(float bpm:{30.f,60.f,123.5f,240.f,300.f})for(int note=0;note<9;++note) {
   cream::SyncEcho e;e.setTiming(bpm,cream::SyncEcho::noteBeats(note));e.reset();
   const float expected=48000.f*60.f*cream::SyncEcho::noteBeats(note)/bpm;
   int at=0;float peak=0;
   for(int n=0;n<int(expected)+8;++n){float v=e.tick(n==0?.2f:0.f,0.f,true);if(std::fabs(v)>peak){peak=std::fabs(v);at=n;}}
   timing &= peak>.02f && std::fabs(at-expected)<=2.f;
 }
 check(timing,"all 9 note divisions track 30/60/123.5/240/300 BPM within 2 samples");
 cream::SyncEcho e;e.setTiming(120,.5f);e.reset();float first=0,second=0;
 for(int n=0;n<24010;++n){float v=e.tick(n==0?.2f:0.f,.5f,true);if(n==12000)first=v;if(n==24000)second=v;}
 check(first>.02f && std::fabs(second/first-.5f)<.002f,"feedback produces a second repeat at the correct interval and gain");
 e.reset();bool clear=true;for(int n=0;n<100000;++n)clear &= e.tick(0,0,true)==0;
 check(clear,"lazy reset prevents stale buffer data, including after ring wrap");
 for(int n=0;n<1000;++n)e.tick(.2f,.8f,true);
 e.tick(0,0,false);clear=true;for(int n=0;n<100000;++n)clear &= e.tick(0,0,true)==0;
 check(clear,"ECHO bypass invalidates old audio before re-enabling");
 e.setTiming(120,.5f);e.reset();float previous=0,maxStep=0;
 for(int n=0;n<120000;++n) {
   if(n==60000)e.setTiming(60,.75f);
   float v=e.tick(.2f*std::sin(n*.0104719755f),.4f,true);
   if(n>60000&&n<62000)maxStep=std::max(maxStep,std::fabs(v-previous));previous=v;
 }
 check(maxStep<.02f,"tempo and note changes crossfade without a large sample discontinuity");
#ifdef CREAM_GRIT_MASTER
 check(sizeof(cream::DSP)<262144,"extended MASTER DSP stays below 256KiB");
#else
 check(cream::SyncEcho::kBytes==96004 && cream::PitchShift::kBytes==32768 && sizeof(cream::DSP)<131072,"delay+pitch rings consume 128772 bytes; total DSP below 128KiB");
#endif
 p={65,-13,40,55,350,12,30,0,10,35,5,40};x=sine(5678);
 check(difference(render(x,p,1),render(x,p,257))<1.e-7,"tempo delay is independent of callback block size");
 bool pitchFreq=true;
 for(int semi:{-36,-24,-12,12,24}) {
   p={0,0,0,55,350,12,0,0,0,0,5,25,semi,100,1};
   auto input=sine(440,.2f);auto shifted=render(input,p);
   const double expected=440*std::pow(2.,semi/12.);
   const double amp=tone(shifted,expected),old=tone(shifted,440);
   std::printf("PITCH %+d: expected %.2f Hz, amplitude %.5f; source amplitude %.5f\n",semi,expected,amp,old);
   pitchFreq &= amp>.08 && amp>old*8;
 }
 check(pitchFreq,"independent pitch produces shifted 440Hz tone across -36..+24 semitones");
 p={0,0,0,55,350,12,0,0,0,0,5,25,-12,100,1};
 auto brightDown=render(sine(12000,.2f),p);
 std::printf("12kHz input at PITCH -12: 6kHz amplitude %.5f\n",tone(brightDown,6000));
 check(rms(brightDown)>.095 && tone(brightDown,6000)>.05,"pitch-down retains upper-band energy including granular sidebands");
 p={0,0,0,55,350,12,0,0,0,0,5,25,-12,100,1};x=sine(440,.2f);
 auto shifted=render(x,p);p[1]=-24;
 check(shifted==render(x,p),"with GRIT=0, CLOCK does not affect independent PITCH");
 p[14]=0;x=sine(60,.2f);auto protectedBass=render(x,p);
 check(std::fabs(20*std::log10(tone(protectedBass,60)/tone(x,60)))<.15,"HIGH scope protects the low fundamental while pitching");
 p[14]=1;x=sine(440,.2f);
 check(difference(render(x,p,1),render(x,p,257))<1.e-7,"pitch render independent of callback block size");
 cream::PitchShift ps;ps.setRatio(.5f);ps.reset();bool clean=true;float pi[2]={},po[2];
 for(int n=0;n<12000;++n){ps.tick(pi,po,.5f,true);clean &= po[0]==0&&po[1]==0;}
 check(clean,"pitch stage keeps silence at zero");
 for(int n=0;n<12000;++n){pi[0]=.2f;ps.tick(pi,po,.5f,true);}
 ps.reset();pi[0]=0;clean=true;
 for(int n=0;n<12000;++n){ps.tick(pi,po,.5f,true);clean &= po[0]==0&&po[1]==0;}
 check(clean,"pitch reset invalidates old audio through ring wrap");
 V noise(1024*2),result(noise.size());uint32_t state=42;
 for(auto& v:noise){state=1664525*state+1013904223;v=(int32_t(state)/2147483648.f)*4.f;}
 bool finite=true,bounded=true;
 for(int mask=0;mask<(1<<12);++mask) {
   for(int k=0;k<cream::kCount;++k)p[k]=(mask&(1<<(k%12)))?cream::kMin[k]:cream::kMax[k];
   d.set(p.data()); d.process(noise.data(),result.data(),noise.size()/2);
   for(float v:result){finite &= std::isfinite(v);bounded &= std::fabs(v)<=1.f;}
 }
 check(finite&&bounded,"4096 base-parameter corners with pitch endpoint transitions stay finite and bounded");
 noise.resize(16384*2);result.resize(noise.size());
 for(auto& v:noise){state=1664525*state+1013904223;v=(int32_t(state)/2147483648.f)*4.f;}
 finite=bounded=true;
 for(int mask=0;mask<8;++mask) {
   p={100,-24,100,100,150,6,100,6,100,100,8,85,
      (mask&1)?-36:24,(mask&2)?0:100,(mask&4)?0:1};
   d.set(p.data());d.process(noise.data(),result.data(),noise.size()/2);
   for(float v:result){finite &= std::isfinite(v);bounded &= std::fabs(v)<=1.f;}
 }
 check(finite&&bounded,"all 8 pitch endpoint combinations stay bounded after buffer warmup");
}
int main(int argc,char**argv) {
 if(argc==1) {tests();return failures?1:0;}
 if(argc!=cream::kCount+4 && argc!=cream::kCount+5) {std::fprintf(stderr,"input.f32 output.f32 parameters... block [BPM]\n");return 2;}
 std::ifstream f(argv[1],std::ios::binary|std::ios::ate);if(!f)return 3;
 const size_t bytes=size_t(f.tellg());if(bytes%8)return 4;f.seekg(0);
 V x(bytes/sizeof(float));f.read(reinterpret_cast<char*>(x.data()),bytes);P p;
 for(int i=0;i<cream::kCount;++i)p[i]=std::atoi(argv[3+i]);
 V y=render(x,p,std::max(1,std::atoi(argv[cream::kCount+3])),argc==cream::kCount+5?std::strtof(argv[cream::kCount+4],nullptr):120.f);
 std::ofstream o(argv[2],std::ios::binary);o.write(reinterpret_cast<const char*>(y.data()),y.size()*sizeof(float));
 return o?0:5;
}
