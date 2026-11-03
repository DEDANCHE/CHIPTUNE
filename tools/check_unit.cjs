// Verify actual ELF PT_LOAD span and required exported callbacks, not just section totals.
const fs=require('fs'),crypto=require('crypto');
const file=process.argv[2],manifest=process.argv[3];
if(!file)throw Error('Usage: node tools/check_unit.cjs package [manifest.json]');
const b=fs.readFileSync(file),u16=o=>b.readUInt16LE(o),u32=o=>b.readUInt32LE(o);
if(b.length<52||b.toString('hex',0,4)!=='7f454c46'||b[4]!==1||b[5]!==1||u16(18)!==40)throw Error('Expected little-endian ELF32 ARM');
const segments=[];
for(let i=0;i<u16(44);i++){
 const p=u32(28)+i*u16(42);if(p+32>b.length)throw Error('Invalid program header');
 if(u32(p)===1){const offset=u32(p+4),base=u32(p+8),files=u32(p+16),memory=u32(p+20);
 if(offset+files>b.length||files>memory)throw Error('Invalid load segment');segments.push({base,memory});}
}
if(!segments.length)throw Error('No loadable segments');
const loadBytes=Math.max(...segments.map(s=>s.base+s.memory))-Math.min(...segments.map(s=>s.base));
if(loadBytes>49152||b.length>49152)throw Error(`48 KiB limit exceeded: load=${loadBytes} package=${b.length}`);
const sections=[];for(let i=0;i<u16(48);i++){const s=u32(32)+i*u16(46);sections.push({type:u32(s+4),offset:u32(s+16),size:u32(s+20),link:u32(s+24),entry:u32(s+36)});}
const callbackExports=new Set();
for(const s of sections.filter(s=>s.type===11)){
 const strings=sections[s.link];if(!s.entry)throw Error('Invalid symbol table');
 for(let p=s.offset;p<s.offset+s.size;p+=s.entry){
  const start=strings.offset+u32(p),end=b.indexOf(0,start);
  if(u16(p+14)!==0 && (b[p+13]&3)===0 && (b[p+12]>>4)!==0)callbackExports.add(b.toString('utf8',start,end));
 }
}
const required=['unit_init','unit_teardown','unit_reset','unit_resume','unit_suspend','unit_render','unit_set_param_value','unit_get_param_value','unit_get_param_str_value','unit_note_on','unit_note_off','unit_all_note_off','unit_set_tempo','unit_tempo_4ppqn_tick','unit_pitch_bend','unit_channel_pressure','unit_aftertouch'];
for(const name of required)if(!callbackExports.has(name))throw Error('Missing callback export: '+name);
const result={package:file,packageBytes:b.length,loadBytes,headroomBytes:49152-loadBytes,sha256:crypto.createHash('sha256').update(b).digest('hex'),callbacks:required.length};
if(manifest)fs.writeFileSync(manifest,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result));
