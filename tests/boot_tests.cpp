#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v){if(!v)throw std::runtime_error("boot check failed");}
int main(int argc,char**argv){try{
 ps1::Bus b;ps1::Cpu c(b);ck(argc==2);b.load_bios(argv[1]);c.reset();
 for(unsigned i=0;i<3000;i++){c.step();}
 ck(b.read32(0x100)==0x600d&&b.post==0x42&&b.ram_control==0xb88&&b.cache_control==0x804);
 ck(b.tty=="BOOT ROM: CPU -> BIOS ROM -> MMIO -> RAM -> B0 OK\n");
 ck(c.diagnostic().find("Recent fetched instructions")!=std::string::npos);
 b.write32(0xfffe0130,0x12345678);b.write8(0xfffe0131,0xab);ck(b.read32(0xfffe0130)==0x1234ab78);
 b.write32(0xbf801000,0);ck(b.read32(0x1f801000)==0x1f000000);ck(b.read32(0x1f000000)==0xffffffff);
 b.irq_status=0x7ff;b.write8(0x1f801070,0x0f);ck(b.irq_status==0x70f);b.write32(0x1f801074,0xffffffff);ck(b.irq_mask==0x7ff);
 bool failed=false;try{b.read32(0x1f801140);}catch(const std::runtime_error&e){failed=std::string(e.what()).find("1f801140")!=std::string::npos;}ck(failed);
 for(int i=0;i<20000;i++){b.append_tty('a');}ck(b.tty.size()<=8192);
 std::cout<<"PASS: reset-ROM execution, memory/cache control, actual RAM B0 dispatch, TTY, POST, IO aliases/byte lanes, IRQ latches, bus fault address, bounded diagnostics\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
