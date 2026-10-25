#include "spu.hpp"
#include <stdexcept>
namespace ps1 {
uint16_t Spu::read(unsigned offset)const{
 if(offset>=512||(offset&1))throw std::runtime_error("Invalid SPU halfword read");
 if(offset==0x1a8)throw std::runtime_error("SPU FIFO manual read unsupported");
 // Immediate mode reflection; hardware status is delayed. No DMA requests advertised.
 if(offset==0x1ae)return regs[0x1aa/2]&63;
 return regs[offset/2];
}
uint32_t Spu::dma_read(){
 if(((regs[0x1aa/2]>>4)&3)!=3)throw std::runtime_error("SPU DMA read requires mode 3");
 uint32_t v=ram[cursor/2];cursor=(cursor+2)&0x7ffff;v|=uint32_t(ram[cursor/2])<<16;cursor=(cursor+2)&0x7ffff;return v;
}
void Spu::dma_write(uint32_t value){
 if(((regs[0x1aa/2]>>4)&3)!=2)throw std::runtime_error("SPU DMA write requires mode 2");
 ram[cursor/2]=uint16_t(value);cursor=(cursor+2)&0x7ffff;ram[cursor/2]=uint16_t(value>>16);cursor=(cursor+2)&0x7ffff;writes+=2;
}
void Spu::flush(){for(uint16_t v:fifo){ram[cursor/2]=v;cursor=(cursor+2)&0x7ffff;}fifo.clear();}
void Spu::write(unsigned offset,uint16_t value){
 if(offset>=512||(offset&1))throw std::runtime_error("Invalid SPU halfword write");
 if(offset==0x1ae||offset==0x19c||offset==0x19e||offset==0x1b8||offset==0x1ba)return;
 if(offset==0x1a8){
  if(fifo.size()>=32)throw std::runtime_error("SPU FIFO full (32 halfwords)");
  fifo.push_back(value);if(((regs[0x1aa/2]>>4)&3)==1)flush();writes++;return;
 }
 regs[offset/2]=value;writes++;
 if(offset==0x1a6)cursor=(uint32_t(value)*8)&0x7ffff;
 if(offset==0x1aa&&((value>>4)&3)==1)flush();
}
}
