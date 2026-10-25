#include "core.hpp"
#include <algorithm>
#include <stdexcept>
namespace ps1 {
void load_exe(Bus& bus,Cpu& cpu,const std::vector<uint8_t>& data){
 const char magic[]="PS-X EXE";
 if(data.size()<0x800||!std::equal(magic,magic+8,data.begin()))throw std::runtime_error("Not a PS-X EXE (ELF/Windows EXE unsupported)");
 auto word=[&](unsigned offset){uint32_t v=0;for(unsigned i=0;i<4;i++)v|=uint32_t(data[offset+i])<<(i*8);return v;};
 auto ram_address=[](uint32_t a){return (a<0x200000)||(a>=0x80000000&&a<0x80200000)||(a>=0xa0000000&&a<0xa0200000);};
 uint32_t entry=word(0x10),gp=word(0x14),target=word(0x18),size=word(0x1c),stack=word(0x30),stack_size=word(0x34);
 if(!ram_address(target)||!ram_address(entry)||(target&3)||(entry&3))throw std::runtime_error("EXE entry/load address must be aligned main RAM");
 const uint32_t offset=target&0x1fffff;
 if(!size||size>bus.ram.size()-offset||size>data.size()-0x800)throw std::runtime_error("EXE payload truncated or exceeds main RAM");
 if(stack&&uint64_t(stack)+stack_size>0xffffffffull)throw std::runtime_error("EXE stack overflow");
 // No mutation until validation is complete. BIOS services are not initialized.
 std::copy_n(data.begin()+0x800,size,bus.ram.begin()+offset);
 cpu.reset(entry);cpu.r[28]=gp;
 if(stack){cpu.r[29]=stack+stack_size;cpu.r[30]=cpu.r[29];}
}
}
