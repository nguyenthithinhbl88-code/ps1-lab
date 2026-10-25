#include "core.hpp"
#include <iostream>
#include <stdexcept>
void check(bool v){if(!v) throw std::runtime_error("Test failed");}
int main(){
try {
 ps1::Bus b;ps1::Cpu c(b);
 ps1::install_demo(b);c.reset(0);for(int i=0;i<4;i++)c.step();check(b.read32(256)==12);
 b.write32(0x80000100,0x12345678);check(b.read32(0xa0000100)==0x12345678);check(b.read8(256)==0x78);
 // Taken branch executes exactly one delay slot and skips fallthrough.
 b.write32(0,0x10000002);b.write32(4,0x24080001);b.write32(8,0x24080002);b.write32(12,0x24090003);
 c.reset(0);c.step();c.step();check(c.r[8]==1 && c.pc==12);c.step();check(c.r[9]==3);
 // Load delay: immediate consumer sees old t0; next instruction sees loaded t0.
 b.write32(256,42);b.write32(0,0x8c080100);b.write32(4,0x01004821);b.write32(8,0x01005021);
 c.reset(0);c.r[8]=7;c.step();c.step();c.step();check(c.r[9]==7 && c.r[10]==42);
 // An explicit write in the delay slot wins over a preceding load.
 b.write32(4,0x24080009);c.reset(0);c.step();c.step();check(c.r[8]==9);
 b.write32(0,0x24000009);c.reset(0);c.step();check(c.r[0]==0);
 bool failed=false;try{b.read32(1);}catch(const std::runtime_error&){failed=true;}check(failed);
 failed=false;try{b.write8(0xbfc00000,1);}catch(const std::runtime_error&){failed=true;}check(failed);
 std::cout<<"PASS: demo, RAM aliases/endian, branch/load delays, zero register, bus guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
