#include "core.hpp"
#include <stdexcept>
#include <iostream>
void ck(bool v){if(!v)throw std::runtime_error("SPU check failed");}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);c.reset(0xbfc06874);
 const uint32_t code[]={0x3c021f80,0x34421c00,0xa4400186};
 for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)b.bios[0x6874+i*4+j]=uint8_t(code[i]>>(j*8));
 for(unsigned i=0;i<3;i++)c.step();
 ck(c.pc==0xbfc06880&&b.spu.writes==1&&b.read16(0x1f801d86)==0);
 b.write16(0xbf801d86,0x1234);ck(b.read16(0x1f801d86)==0x1234);
 b.write32(0x1f801d84,0x56789abc);ck(b.read32(0xbf801d84)==0x56789abc);
 b.write16(0x1f801da6,0xffff);b.write16(0x1f801da8,0x4321);ck(b.spu.ram[0x7fff8/2]==0);
 b.write16(0x1f801daa,0x8010);ck(b.spu.ram[0x7fff8/2]==0x4321&&b.read16(0x1f801dae)==0x10);
 for(unsigned i=0;i<4;i++){b.write16(0x1f801da8,uint16_t(i+1));}ck(b.spu.cursor==2&&b.spu.ram[0]==4);
 ck(b.read16(0x1f801da6)==0xffff);
 b.write16(0x1f801dae,0xffff);ck(b.read16(0x1f801dae)==0x10);
 b.write16(0x1f801daa,0);for(unsigned i=0;i<32;i++)b.write16(0x1f801da8,0);
 bool full=false;try{b.write16(0x1f801da8,1);}catch(const std::runtime_error&){full=true;}ck(full);
 std::cout<<"PASS: exact BIOS trace replay, SPU aliases/halfword/word, FIFO queue/manual flush, wrap, read-only status, overflow guard\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
