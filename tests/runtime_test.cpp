#include <cstdio>
#include <cmath>
#include <cstring>
#include "../platform/nts1_mkii/unit.cc"
static int failures=0;
static void check(bool ok,const char *name){std::printf("%s: %s\n",name,ok?"ok":"FAIL");if(!ok)++failures;}
int main(){
  unit_runtime_osc_context_t context{}; context.pitch=57u<<8;
  unit_runtime_desc_t desc{unit_header.target,UNIT_API_VERSION,48000,2,1,{&context}};
  check(unit_init(nullptr)==k_unit_err_undef,"reject null runtime");
  auto bad=desc;bad.hooks.runtime_context=nullptr;
  check(unit_init(&bad)==k_unit_err_undef,"reject null context");
  bad=desc;bad.samplerate=44100;check(unit_init(&bad)==k_unit_err_samplerate,"reject unsupported sample rate");
  check(unit_init(&desc)==0,"initialize real callback adapter");
  for(int p=0;p<chiptune::kNumPresets;++p){
    unit_set_param_value(Osc::PRESET,p);
    check(unit_get_param_str_value(Osc::PRESET,p)!=nullptr,"all presets have runtime labels");
  }
  for(int i=0;i<Osc::NUM_PARAMS;++i){
    unit_set_param_value(i,unit_header.params[i].max);
    check(unit_get_param_value(i)<=unit_header.params[i].max,"header and adapter ranges agree");
  }
  float actual[1024],expected[1024];
  unit_reset();unit_set_param_value(Osc::PRESET,71);
  unit_note_on(57,127);unit_tempo_4ppqn_tick(1);unit_render(nullptr,actual,1024);
  chiptune::Apu reference;reference.init(48000);auto params=chiptune::preset(71);
  reference.setParams(params);reference.setFrequency(220);reference.noteOn(127);reference.tempo4ppqnTick(1);
  for(int i=0;i<1024;++i)expected[i]=reference.render();
  check(std::memcmp(actual,expected,sizeof actual)==0,"clock after note-on aligns before first sample");
  unit_reset();unit_set_param_value(Osc::PRESET,71);
  unit_tempo_4ppqn_tick(1);unit_note_on(57,127);unit_render(nullptr,actual,1024);
  reference.reset();reference.noteOn(127);for(int i=0;i<1024;++i)expected[i]=reference.render();
  check(std::memcmp(actual,expected,sizeof actual)==0,"note-on after clock starts a new phrase");
  unit_reset();unit_note_on(57,127);unit_all_note_off();unit_render(nullptr,actual,1024);
  bool silent=true;for(float x:actual)silent &= x==0;
  check(silent,"all-note-off cancels pending onset");
  unit_note_on(57,127);unit_render(nullptr,actual,1024);unit_suspend();unit_resume();unit_render(nullptr,actual,1024);
  silent=true;for(float x:actual)silent &= x==0;check(silent,"suspend/resume remains silent until next note");
  unit_teardown();return failures?1:0;
}
