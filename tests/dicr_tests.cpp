#include "core.hpp"
#include <stdexcept>
#include <iostream>
void ck(bool v){if(!v)throw std::runtime_error("DICR check failed");}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);c.reset(0x8005a66c);
 b.write32(0x80079da0,0x1f8010f4);
 const uint32_t code[]={0x3c0e8008,0x8dce9da0,0x3c01800e,0xadc00000};
 for(unsigned i=0;i<4;i++)b.write32(0x8005a66c+i*4,code[i]);
 for(unsigned i=0;i<4;i++)c.step();
 ck(c.pc==0x8005a67c&&b.read32(0x1f8010f4)==0);
 auto set=[&](uint32_t v){b.write32(0xbf8010f4,v);};
 // Enable/mask gating of completion flags, independently from I_MASK.
 for(unsigned ch=0;ch<7;ch++){
  set(0x7f000000);b.irq_status=0; b.dma_complete(ch);ck(b.dicr==0&&b.irq_status==0);
  set(1u<<(16+ch));b.dma_complete(ch);ck(!(b.dicr&0x7f000000));
  set(0x800000);b.dma_complete(ch);ck(!(b.dicr&0x7f000000));
  uint32_t config=0x800000|(1u<<(16+ch)),flag=1u<<(24+ch);
  set(config);b.dma_complete(ch);ck((b.dicr&flag)&&(b.dicr>>31)&&(b.irq_status&8));
  b.write32(0x1f801070,~8u);ck(!(b.irq_status&8));b.dma_complete(ch);ck(!(b.irq_status&8));
  // Clearing channel enable preserves pending flag and master state.
  set(0x800000);ck((b.dicr&flag)&&(b.dicr>>31));
  set(config|flag);ck(!(b.dicr&flag)&&!(b.dicr>>31));b.dma_complete(ch);ck(b.irq_status&8);
  set(config|flag);
 }
 set(0x7f008000);ck(b.dicr>>31);b.write32(0x1f801070,~8u);set(0x8000);ck(!(b.irq_status&8));set(0);set(0x8000);ck(b.irq_status&8);
 set(0x7f000000);set(0x80000000);ck(b.dicr==0); // bit31 cannot be forced by write
 // Actual OTC transfer completion raises channel6 flag + IRQ3.
 set(0xc00000);b.write32(0x1f8010f0,0x0f654321);b.write32(0x1f8010e0,0x100);b.write32(0x1f8010e4,1);b.write32(0x1f8010e8,0x11000002);
 ck(b.read32(0x100)==0xffffff&&(b.dicr&0xc0000000)==0xc0000000);
 set(0x40c00000);ck(!(b.dicr>>31)&&(b.irq_status&8));
 std::cout<<"PASS: exact BIOS DICR write replay, seven-channel enable matrix, W1C, pending flag persistence, IRQ3 edges/ack, bit31 read-only, actual OTC completion\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
