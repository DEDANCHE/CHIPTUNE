// Host-only audition renderer and block timing harness. No device CPU claims.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "../src/presets.h"
using namespace chiptune;
static void wav(const std::string &path,const std::vector<float>&s){
  FILE*f=std::fopen(path.c_str(),"wb");if(!f){std::perror(path.c_str());std::exit(1);}
  uint32_t bytes=static_cast<uint32_t>(s.size()*4),riff=36+bytes,fmt=16,sr=48000,rate=192000;
  uint16_t kind=3,ch=1,align=4,bits=32;
  std::fwrite("RIFF",1,4,f);std::fwrite(&riff,4,1,f);std::fwrite("WAVEfmt ",1,8,f);
  std::fwrite(&fmt,4,1,f);std::fwrite(&kind,2,1,f);std::fwrite(&ch,2,1,f);std::fwrite(&sr,4,1,f);
  std::fwrite(&rate,4,1,f);std::fwrite(&align,2,1,f);std::fwrite(&bits,2,1,f);std::fwrite("data",1,4,f);
  std::fwrite(&bytes,4,1,f);std::fwrite(s.data(),4,s.size(),f);std::fclose(f);
}
static std::vector<float> phrase(int id,int pattern){
  const int notes[3][8]={{33,33,40,36,33,31,36,40},{57,60,64,67,64,60,59,57},{36,36,43,36,48,36,43,36}};
  Apu a;a.init(48000);a.setParams(preset(id));a.setTempo(120);
  std::vector<float> s(96000);
  for(int step=0;step<8;++step){
    a.setFrequency(440.f*std::pow(2.f,(notes[pattern][step]-69)/12.f));a.noteOn(110);
    for(int i=0;i<12000;++i){if(i==(pattern==2?2400:9000))a.noteOff();if(i%6000==0)a.tempo4ppqnTick(step*2+i/6000);s[step*12000+i]=a.render();}
  }
  return s;
}
int main(int argc,char**argv){
  if(argc<3){std::fprintf(stderr,"Usage: audio_tools audition|profile output-directory [preset IDs, or all]\n");return 1;}
  const std::string mode=argv[1],dir=argv[2];
  if(mode=="profile"){
    FILE*f=std::fopen((dir+"/host-profile.csv").c_str(),"w");if(!f)return 1;
    std::fprintf(f,"chip,hz,frames,max_microseconds,mean_microseconds,host_only\n");
    volatile float sink=0;
    for(int chip=0;chip<kNumChips;++chip)for(float hz:{55.f,440.f,12000.f})for(int frames:{48,96}){
      Apu a;a.init(48000);ApuParams p;p.chip=static_cast<Chip>(chip);p.sub=3;p.noise=4;p.detune=16;p.quantize=false;
      a.setParams(p);a.setFrequency(hz);a.setShapeMod(1);a.noteOn(127);
      double total=0,max=0;
      for(int block=0;block<200;++block){auto start=std::chrono::steady_clock::now();for(int i=0;i<frames;++i)sink=a.render();
        double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();total+=us;max=std::max(max,us);}
      std::fprintf(f,"%d,%.0f,%d,%.3f,%.3f,true\n",chip,hz,frames,max,total/200);
    }
    std::fclose(f);std::printf("Host-only timing: %s/host-profile.csv (sink %f)\n",dir.c_str(),sink);return 0;
  }
  if(mode!="audition")return 1;
  std::vector<int> ids;
  if(argc==4&&std::string(argv[3])=="all"){for(int i=0;i<kNumPresets;++i)ids.push_back(i);}
  else for(int i=3;i<argc;++i){char*end=nullptr;long id=std::strtol(argv[i],&end,10);if(*end||end==argv[i]||id<0||id>=kNumPresets)return 1;ids.push_back(static_cast<int>(id));}
  if(ids.empty())ids={0,6,9,61,74,75,76};
  FILE*f=std::fopen((dir+"/audition.csv").c_str(),"w");if(!f)return 1;
  std::fprintf(f,"id,name,pattern,raw_rms,raw_peak,matched_gain,matched_rms\n");
  const char*patterns[]={"bass","melody","percussion"};
  for(int id:ids)for(int pattern=0;pattern<3;++pattern){
    auto s=phrase(id,pattern);double sum=0,peak=0;for(float x:s){sum+=x*x;peak=std::max(peak,double(std::fabs(x)));}
    double rms=std::sqrt(sum/s.size()),gain=rms>1e-9?std::min(0.15/rms,0.95/(peak+1e-9)):1;
    std::string base=dir+"/"+std::to_string(id)+"_"+patterns[pattern];wav(base+"_raw.wav",s);
    for(float&x:s)x=static_cast<float>(x*gain);wav(base+"_matched.wav",s);
    std::fprintf(f,"%d,%s,%s,%.6f,%.6f,%.6f,%.6f\n",id,presetName(id),patterns[pattern],rms,peak,gain,rms*gain);
  }
  std::fclose(f);std::printf("Rendered %zu presets, 3 phrases each, raw and RMS-matched (peak-limited).\n",ids.size());return 0;
}
