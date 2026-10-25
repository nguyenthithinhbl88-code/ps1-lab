#include "core.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
void ck(bool v){if(!v)throw std::runtime_error("system test failed");}
int main(int argc,char**argv){try{
 ps1::Bus b;ps1::Cpu c(b);
 auto w=[&](uint32_t a,uint32_t v){b.write32(a,v);};
 // OTC channel enable, manual trigger, descending pointers and terminator.
 w(0x1f8010f0,0x0f654b21);w(0x1f8010e0,0x200);w(0x1f8010e4,3);w(0x1f8010e8,0x01000002);ck(b.dma_words==0);
 w(0x1f8010e8,0x11000002);ck(b.read32(0x200)==0x1fc&&b.read32(0x1fc)==0x1f8&&b.read32(0x1f8)==0xffffff);ck(!(b.dma[6].control&0x11000000));
 // GPU block DMA and pending channel triggered by DPCR enable.
 w(0x1000,0x020000ff);w(0x1004,0);w(0x1008,(2<<16)|16);
 w(0x1f8010f0,0);w(0x1f8010a0,0x1000);w(0x1f8010a4,(1<<16)|3);w(0x1f8010a8,0x01000201);ck(b.gpu.vram[0]==0);
 w(0x1f8010f0,0x800);ck(b.gpu.vram[0]==31&&!(b.dma[2].control&(1u<<24)));
 // Multi-node list, including empty node.
 w(0x2000,0x002010);w(0x2010,0x03ffffff);w(0x2014,0x0200ff00);w(0x2018,0);w(0x201c,(1<<16)|16);
 w(0x1f8010a0,0x2000);w(0x1f8010a8,0x01000401);ck(b.gpu.vram[0]==0x3e0&&b.dma[2].address==0xffffff);
 // Cycle guard is bounded.
 w(0x3000,0x3000);w(0x1f8010a0,0x3000);bool rejected=false;try{w(0x1f8010a8,0x01000401);}catch(const std::runtime_error&){rejected=true;}ck(rejected);
 if(argc!=2)throw std::runtime_error("Pass sample EXE path");
 std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});ck(!bytes.empty());
 ps1::Bus fresh;ps1::Cpu freshCpu(fresh);ps1::load_exe(fresh,freshCpu,bytes);ck(freshCpu.pc==0x80010000&&freshCpu.r[29]==0x801fff00);
 for(int i=0;i<1000;i++){freshCpu.step();}ck(fresh.gpu.rgb(160,120)==0x3184ff&&fresh.dma_words==14);
 // Rejected input must not mutate CPU/RAM.
 auto verify_reject=[&](std::vector<uint8_t> bad){auto ram=fresh.ram;uint32_t pc=freshCpu.pc;bool failed=false;try{ps1::load_exe(fresh,freshCpu,bad);}catch(const std::runtime_error&){failed=true;}ck(failed&&ram==fresh.ram&&pc==freshCpu.pc);};
 auto bad=bytes;bad[0]=0;verify_reject(bad);bad=bytes;bad.resize(0x801);verify_reject(bad);
 bad=bytes;bad[0x1b]=0x1f;verify_reject(bad);bad=bytes;bad[0x10]|=1;verify_reject(bad);
 std::cout<<"PASS: OTC trigger, block GPU DMA, DPCR pending start, multi-node list, cycle guard, EXE execution and invalid-file atomic rejection\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
