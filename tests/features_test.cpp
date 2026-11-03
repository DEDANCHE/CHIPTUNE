#define main adapter_suite_main
#include "adapter_test.cpp"
#undef main
int main(){
  for(int chip=0;chip<chiptune::kNumChips;++chip)for(int voice=0;voice<=22;++voice){
    Osc source{},restored{};
    source.setParameter(Osc::PRESET,chiptune::kPresetYmSawSolo);flush(source);
    source.setParameter(Osc::CHIP,chip);source.setParameter(Osc::SUB,voice);source.setParameter(Osc::DETN,333);flush(source);
    for(int order=0;order<2;++order){
      for(int j=0;j<Osc::NUM_PARAMS;++j){int i=order?Osc::NUM_PARAMS-1-j:j;restored.setParameter(i,source.getParameterValue(i));}
      check(audioDifference(source,restored)<1e-6,"all voice/chip combinations recall identical audio");
    }
  }
  for(int preset : {chiptune::kPresetNesSolo,chiptune::kPresetNesTri,chiptune::kPresetGbHighHit,chiptune::kPresetVrc6Saw}){
    Osc a{},b{};a.setParameter(Osc::PRESET,preset);b.setParameter(Osc::PRESET,preset);
    a.setParameter(Osc::DETN,0);b.setParameter(Osc::DETN,1023);
    // Vibrato is delayed by 0.2 seconds; compare after that point.
    float x[24000],y[24000];renderFresh(a,x,24000);renderFresh(b,y,24000);
    double difference=0;for(int i=10000;i<24000;++i)difference+=std::fabs(x[i]-y[i]);
    if(preset==chiptune::kPresetGbHighHit){difference=0;for(int i=0;i<4000;++i)difference+=std::fabs(x[i]-y[i]);}
    check(difference>0.01,"solo/noise knob has an audible destination");
  }
  for(int preset : {chiptune::kPresetSidPwm,chiptune::kPresetNesLead}){
    Osc a{},b{};a.setParameter(Osc::PRESET,preset);b.setParameter(Osc::PRESET,preset);
    float x[4800],y[4800];renderFresh(a,x,4800);renderFresh(b,y,4800);
    b.channelPressure(127);a.process(nullptr,x,4800);b.process(nullptr,y,4800);
    double d=0;for(int i=0;i<4800;++i)d+=std::fabs(x[i]-y[i]);
    check(d>0.1,"channel pressure modulates filter or detune");
    renderFresh(a,x,4800);renderFresh(b,y,4800);b.aftertouch(58,127);
    a.process(nullptr,x,4800);b.process(nullptr,y,4800);
    check(std::memcmp(x,y,sizeof x)==0,"poly pressure ignores another note");
    b.aftertouch(57,127);a.process(nullptr,x,4800);b.process(nullptr,y,4800);
    d=0;for(int i=0;i<4800;++i)d+=std::fabs(x[i]-y[i]);check(d>0.1,"poly pressure follows active note");
  }
  {
    Osc osc{};osc.setParameter(Osc::PRESET,0);float block[4800];renderFresh(osc,block,4800);
    float last=block[4799];osc.setParameter(Osc::CHIP,4);osc.process(nullptr,block,4800);
    check(std::fabs(block[0]-last)<1e-6,"engine transition begins at previous output sample");
    bool finite=true;for(float y:block)finite &= std::isfinite(y)&&std::fabs(y)<=1;
    check(finite,"transition crossfade stays bounded");
  }
  {
    chiptune::Apu a,b;a.init(48000);b.init(48000);
    auto p=chiptune::preset(chiptune::kPresetNesKick),plain=p;plain.articulation=0;
    p.quantize=plain.quantize=false;a.setParams(p);b.setParams(plain);a.setFrequency(55);b.setFrequency(55);a.noteOn(127);b.noteOn(127);
    double d=0;for(int i=0;i<2400;++i)d+=std::fabs(a.render()-b.render());check(d>1,"kick envelope changes attack pitch");
    a.noteOff();b.noteOff();for(int i=0;i<12000;++i){a.render();b.render();}
    check(std::fabs(a.render())<1e-3,"kick releases to silence");
    a.init(48000);b.init(48000);p=chiptune::preset(chiptune::kPresetSidSlowDeep);plain=p;plain.articulation=0;
    a.setParams(p);b.setParams(plain);a.noteOn(127);b.noteOn(127);d=0;
    for(int i=0;i<48000;++i)d+=std::fabs(a.render()-b.render());check(d>1,"slow deep PWM differs from coupled macro");
  }
  return failures?1:0;
}
