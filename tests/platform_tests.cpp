#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v){if(!v)throw std::runtime_error("platform check failed");}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);
 c.reset(0x1000);b.write32(0x1000,0x24080007);b.irq_status=8;b.irq_mask=8;
 c.step();ck(c.r[8]==7&&(c.cop0[13]&0x400)&&c.interrupts==0); // SR masks IRQ
 c.cop0[12]=0x401;c.step();ck(c.pc==0x80000080&&c.cop0[14]==0x1004&&c.interrupts==1&&!(c.cop0[12]&1));
 b.irq_status=0;b.write32(0x80,0);c.step();ck(!(c.cop0[13]&0x400));
 c.reset(0x1000);c.cop0[12]=0x400401;b.irq_status=8;c.step();ck(c.pc==0xbfc00180);
 c.reset(0x1000);b.irq_status=0;c.cop0[12]=0x101;c.cop0[13]=0x100;c.step();ck(c.pc==0x80000080);
 // IRQ before branch delay slot: restart branch using EPC/BD.
 c.reset(0x1000);b.write32(0x1000,0x10000002);c.step();c.cop0[12]=0x401;b.irq_status=8;c.step();ck(c.cop0[14]==0x1000&&(c.cop0[13]&0x80000000));
 b.irq_status=0;c.reset(0);b.write32(0,0x8c080100);b.write32(0x100,42);c.step();b.irq_status=8;c.cop0[12]=0x401;c.step();ck(c.r[8]==42&&c.pending_reg==0);
 // DMA4 round trip sound RAM, wrap and DICR channel completion.
 b.write32(0x1f8010f0,0x00080000);b.write32(0x1f8010f4,0x00900000);b.write16(0x1f801da6,0xffff);b.write16(0x1f801daa,0x20);
 b.write32(0x200,0x12345678);b.write32(0x204,0x9abcdef0);b.write32(0x208,0x13572468);
 auto dma4=[&](uint32_t address,uint32_t control){b.write32(0x1f8010c0,address);b.write32(0x1f8010c4,0x10003);b.write32(0x1f8010c8,control);};
 dma4(0x200,0x01000201);ck(b.spu.cursor==4&&(b.dicr&0x90000000)==0x90000000);
 b.write16(0x1f801da6,0xffff);b.write16(0x1f801daa,0x30);dma4(0x300,0x01000200);for(unsigned i=0;i<12;i+=4)ck(b.read32(0x200+i)==b.read32(0x300+i));
 // VRAM upload -> readback (odd count), copy with mask and GPU DMA read.
 auto& g=b.gpu;g.gp0(0xa0000000);g.gp0(0);g.gp0(0x10003);g.gp0(0x23451234);g.gp0(0xffff3456);
 g.gp0(0xc0000000);g.gp0(0);g.gp0(0x10003);ck(b.read32(0x1f801810)==0x23451234);ck(b.read32(0x1f801810)==0x3456&&!(g.status&(1u<<27)));
 g.gp0(0x80000000);g.gp0(0);g.gp0(16);g.gp0(0x10003);ck(g.vram[16]==0x1234&&g.vram[18]==0x3456);
 g.gp0(0xc0000000);g.gp0(16);g.gp0(0x10003);b.write32(0x1f8010f0,0x800);b.write32(0x1f8010a0,0x400);b.write32(0x1f8010a4,0x10002);b.write32(0x1f8010a8,0x01000200);ck(b.read32(0x400)==0x23451234&&b.read32(0x404)==0x3456);
 g.gp0(0xe3000000|12|(13<<10));g.gp1(0x10000003);ck(g.read_gp0()==uint32_t(12|(13<<10)));
 g.gp0(0x1f000000);b.irq_status=0;b.tick_timers();ck(b.irq_status&2);b.write32(0x1f801070,~2u);b.tick_timers();ck(!(b.irq_status&2));g.gp1(0x02000000);ck(!(g.status&(1u<<24)));
 // SIO0 disconnected response, mode/control/baud, acknowledge/reset.
 b.write16(0x1f801048,0x000d);b.write16(0x1f80104e,0x88);b.write16(0x1f80104a,0x0803);b.write8(0x1f801040,1);
 ck(b.read32(0x1f801044)&2);b.tick_timers();ck(b.irq_status&0x80);ck(b.read8(0x1f801040)==255&&!(b.read32(0x1f801044)&2));
 b.write16(0x1f80104a,0x0813);ck(!(b.read32(0x1f801044)&512));b.write16(0x1f80104a,0x40);ck(b.read16(0x1f801048)==0&&b.read16(0x1f80104e)==0);
 std::cout<<"PASS: hardware/software CPU IRQ masks/vector/EPC/BD/load commit, DMA4/SPU wrap roundtrip/DICR, GPU readback/copy/DMA/info/IRQ, disconnected SIO\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
