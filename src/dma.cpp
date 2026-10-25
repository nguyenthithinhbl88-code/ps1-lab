#include "core.hpp"
#include <stdexcept>
namespace ps1 {
void Bus::update_dma_irq(){
 const bool old=dicr&0x80000000u;
 const bool active=(dicr&0x8000u)||((dicr&0x800000u)&&(dicr&0x7f000000u));
 dicr=(dicr&0x7fffffffu)|(active?0x80000000u:0);
 if(active&&!old)irq_status|=8;
}
void Bus::dma_complete(unsigned channel){
 if((dicr&0x800000u)&&(dicr&(1u<<(16+channel))))dicr|=1u<<(24+channel);
 update_dma_irq();
}
bool Bus::dma_read(uint32_t a,uint32_t& value) const {
 if(a==0x1f8010f4){value=dicr;return true;}
 if(a==0x1f8010f0){value=dpcr;return true;}
 if(a<0x1f801080||a>=0x1f8010f0||(a&3))return false;
 unsigned ch=(a-0x1f801080)/16,reg=a&15;
 if(reg==0)value=dma[ch].address;else if(reg==4)value=dma[ch].block;else if(reg==8)value=dma[ch].control;else return false;
 return true;
}
bool Bus::dma_write(uint32_t a,uint32_t v){
 if(a==0x1f8010f4){
  // Completion bits are write-one-to-clear; master flag is derived, read-only.
  dicr=(dicr&0xff000000u&~(v&0x7f000000u))|(v&0x00ff807fu);
  update_dma_irq();return true;
 }
 if(a==0x1f8010f0){dpcr=v;for(unsigned i=0;i<7;i++)run_dma(i);return true;}
 if(a<0x1f801080||a>=0x1f8010f0||(a&3))return false;
 unsigned ch=(a-0x1f801080)/16,reg=a&15;
 if(reg==0)dma[ch].address=v&0xffffff;else if(reg==4)dma[ch].block=v;
 else if(reg==8)dma[ch].control=ch==6?(v&0x51000000)|2:v;
 else return false;
 run_dma(ch);return true;
}
void Bus::run_dma(unsigned ch){
 auto& d=dma[ch];uint32_t control=d.control;
 if(!(control&(1u<<24))||!(dpcr&(8u<<(ch*4))))return;
 unsigned mode=(control>>9)&3;
 if(mode==0&&!(control&(1u<<28)))return;
 if(control&0x100)throw std::runtime_error("DMA chopping unsupported");
 if(ch!=2&&ch!=3&&ch!=4&&ch!=6)throw std::runtime_error("DMA channel unsupported");
 if(ch==3&&((control&1)||mode!=0))throw std::runtime_error("CD DMA supports manual device-to-RAM only");
 if(ch==6&&mode!=0)throw std::runtime_error("OTC requires manual mode");

 uint32_t address=d.address&0x1ffffc;
 constexpr uint64_t limit=1024*1024;
 if(mode==2){
  if(ch!=2||!(control&1)||(control&2))throw std::runtime_error("Unsupported linked-list direction");
  // Bound cyclic/corrupt lists so the UI cannot hang indefinitely.
  uint64_t budget=limit;
  for(;;){
   if(!budget--)throw std::runtime_error("DMA list exceeded traversal limit");
   uint32_t header=read32(address),words=header>>24;
   for(uint32_t i=0;i<words;i++){
    if(!budget--)throw std::runtime_error("DMA list exceeded word limit");
    address=(address+4)&0x1ffffc;gpu.gp0(read32(address));dma_words++;
   }
   if(header&0x800000){d.address=0xffffff;break;}
   address=header&0x1ffffc;
  }
 }else if(mode==0||mode==1){
  uint64_t size=d.block&65535;if(!size)size=65536;
  if(mode==1){uint64_t blocks=d.block>>16;if(!blocks)blocks=65536;size*=blocks;}
  if(size>limit)throw std::runtime_error("DMA transfer exceeds emulator limit");
  if(ch==3){
   if(!cd.requested)return;
   const uint64_t available=cd.data.size()/4;
   if(available<size){
    // CD DRQ can arrive after DMA starts; transfer the available sector chunk.
    const uint32_t delta=(control&2)?uint32_t(-4):4;
    for(uint64_t i=0;i<available;i++){
     uint32_t word=0;for(unsigned j=0;j<4;j++)word|=uint32_t(cd.read(2))<<(j*8);
     write32(address,word);address=(address+delta)&0x1ffffc;dma_words++;
    }
    d.address=address;d.block=(d.block&0xffff0000u)|uint32_t((size-available)&65535);
    return;
   }
  }
  if(ch==2&&!(control&1)&&size>(gpu.read_remaining()+1)/2)throw std::runtime_error("GPU read DMA underrun");
  const uint32_t step=(control&2)?uint32_t(-4):4;
  for(uint64_t i=0;i<size;i++){
   if(ch==6)write32(address,i+1==size?0xffffff:(address-4)&0x1fffff);
   else if(ch==3){uint32_t word=0;for(unsigned j=0;j<4;j++)word|=uint32_t(cd.read(2))<<(j*8);write32(address,word);}
   else if(ch==4){if(control&1)spu.dma_write(read32(address));else write32(address,spu.dma_read());}
   else if(!(control&1))write32(address,gpu.read_gp0());
   else gpu.gp0(read32(address));
   address=(address+step)&0x1ffffc;dma_words++;
  }
  d.address=address;if(mode==1)d.block&=65535;
 }else throw std::runtime_error("Reserved DMA sync mode");
 d.control&=~0x11000000u;dma_complete(ch);
}
}
