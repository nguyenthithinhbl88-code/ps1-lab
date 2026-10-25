#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v){if(!v)throw std::runtime_error("timer check failed");}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);c.reset(0x15a0);c.r[19]=0x1f801100;
 b.write32(0x15a0,0x0c0009e8);b.write32(0x15a4,0xa6600000);b.write32(0x27a0,0x3c020001);
 c.step();c.step();ck(c.pc==0x27a0&&b.read16(0x1f801100)==1);c.step();ck(c.r[2]==0x10000);
 // COUNT / MODE / TARGET for all three channels through aliases and word accesses.
 for(unsigned i=0;i<3;i++){
  uint32_t a=0x1f801100+i*16;b.write32(a+8,3);b.write32(a+4,0x58);
  ck(b.read32(a)==0&&b.read32(a+4)==0x458);
  for(int j=0;j<3;j++)b.tick_timers();
  ck(b.read16(a)==0&&(b.irq_status&(1u<<(i+4))));ck(b.read16(a+4)&0x800);ck(!(b.read16(a+4)&0x800));
  b.write16(a,0xfffe);b.write16(a+8,0);b.tick_timers();ck(b.read16(a+4)&0x1000);
  b.write32(a+4,0);b.write32(a,0x12345678);ck(b.read16(a)==0x5678);ck(b.read16(a|0xa0000000)==0x5678);
 }
 // Timer2 prescaler and stop mode.
 b.write16(0x1f801124,0x200);for(int i=0;i<7;i++)b.tick_timers();ck(b.read16(0x1f801120)==0);b.tick_timers();ck(b.read16(0x1f801120)==1);
 b.write16(0x1f801124,1);for(int i=0;i<20;i++)b.tick_timers();ck(b.read16(0x1f801120)==0);
 ps1::Timer t;t.write(8,1);t.write(4,0x18);ck(t.tick(0));ck(!t.tick(0));t.write(4,0xd8);ck(t.tick(0));ck(!t.tick(0));ck(t.tick(0));
 // External video clock doesn't silently use CPU ticks.
 t.write(4,0x100);for(int i=0;i<20;i++)t.tick(0);ck(t.count==0);
 std::cout<<"PASS: BIOS JAL/SH delay-slot replay, timer registers/aliases, target/overflow flags, IRQ latches, one-shot/toggle, divide8/stop, external clock wait\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
