#include "core.hpp"
#include <stdexcept>
#include <iostream>
static void check(bool v){if(!v)throw std::runtime_error("extended CPU check failed");}
static uint32_t R(unsigned s,unsigned t,unsigned d,unsigned fn){return(s<<21)|(t<<16)|(d<<11)|fn;}
static uint32_t I(unsigned op,unsigned s,unsigned t,unsigned imm){return(op<<26)|(s<<21)|(t<<16)|(imm&65535);}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);
 auto exec=[&](uint32_t ins,uint32_t a,uint32_t v){c.reset(0);c.r[1]=a;c.r[2]=v;b.write32(0,ins);c.step();};
 exec(R(1,2,0,24),0xfffffffe,3);check(c.hi==0xffffffff&&c.lo==0xfffffffa);
 exec(R(1,2,0,25),0xffffffff,2);check(c.hi==1&&c.lo==0xfffffffe);
 exec(R(1,2,0,26),0x80000000,0xffffffff);check(c.lo==0x80000000&&c.hi==0);
 exec(R(1,2,0,26),0xfffffff9,0);check(c.lo==1&&c.hi==0xfffffff9);
 exec(R(1,2,0,27),123,0);check(c.lo==0xffffffff&&c.hi==123);
 exec(R(1,2,3,32),0x7fffffff,1);check(((c.cop0[13]>>2)&31)==12&&c.cop0[14]==0&&c.pc==0xbfc00180&&c.r[3]==0);
 // Even an untaken branch establishes a delay slot for exceptions.
 c.reset(0);c.r[1]=1;b.write32(0,I(4,1,0,2));b.write32(4,12);c.step();c.step();check(c.cop0[14]==0&&c.cop0[13]==0x80000020);
 exec(I(35,1,2,1),0x100,0);check(c.cop0[8]==0x101&&((c.cop0[13]>>2)&31)==4);
 exec(I(43,1,2,2),0x100,123);check(((c.cop0[13]>>2)&31)==5);
 b.write8(0x100,0x80);exec(I(32,1,2,0),0x100,0);b.write32(4,0);c.step();check(c.r[2]==0xffffff80);
 // LWL/LWR pairing forwards pending load when both target same register.
 for(unsigned offset=0;offset<4;offset++){
  c.reset(0);for(unsigned j=0;j<12;j++)b.write8(0x100+j,uint8_t(0x10+j));
  c.r[1]=0x100+offset;b.write32(0,I(34,1,2,3));b.write32(4,I(38,1,2,0));b.write32(8,0);
  c.step();c.step();c.step();uint32_t expected=0;for(unsigned j=0;j<4;j++)expected|=uint32_t(0x10+offset+j)<<(8*j);check(c.r[2]==expected);
  c.reset(0);c.r[1]=0x180+offset;c.r[2]=0xa1b2c3d4;
  for(unsigned j=0;j<12;j++)b.write8(0x17c+j,0x55);
  b.write32(0,I(42,1,2,3));b.write32(4,I(46,1,2,0));c.step();c.step();
  for(unsigned j=0;j<4;j++)check(b.read8(0x180+offset+j)==uint8_t(0xa1b2c3d4>>(8*j)));
  check(b.read8(0x17f+offset)==0x55&&b.read8(0x184+offset)==0x55);
 }
 // MTC0 / MFC0 load delay and RFE mode-stack restoration.
 c.reset(0);c.r[2]=0x1234;b.write32(0,0x40826000);b.write32(4,0x40036000);b.write32(8,0);c.step();c.step();check(c.r[3]==0);c.step();check(c.r[3]==0x1234);
 exec(0x42000010,0,0);check(c.cop0[12]==0x400000);
 exec(0xffffffff,0,0);check(((c.cop0[13]>>2)&31)==10);
 exec(R(1,2,3,7),4,0x80000000);check(c.r[3]==0xf8000000);
 std::cout<<"PASS: HI/LO, multiply/divide edges, overflow, delay-slot exception, alignment, byte load, unaligned pairs, COP0, reserved instruction, arithmetic shift\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
