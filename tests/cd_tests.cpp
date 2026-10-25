#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v){if(!v)throw std::runtime_error("CD check failed");}
int main(int argc,char**argv){try{
 ck(argc==2);ps1::Bus b;ps1::Cpu c(b);b.cd.mount(argv[1]);
 auto write=[&](unsigned p,uint8_t v){b.write8(0x1f801800+p,v);};
 auto read=[&](unsigned p){return b.read8(0xbf801800+p);};
 auto ack=[&](){while(read(0)&32)read(1);write(0,1);write(3,31);write(0,0);};
 write(0,1);write(2,31);write(0,0);write(1,1);b.tick_timers();ck(b.cd.flag==3&&(b.irq_status&4));ack();
 write(2,0);write(2,2);write(2,0x16);write(1,2);b.tick_timers();ck(b.cd.lba==16);ack();
 write(1,6);b.tick_timers();ck(b.cd.flag==3);ack();b.tick_timers();ck(b.cd.flag==1&&b.cd.data.size()==2048);ack();
 write(3,0x80);b.write32(0x1f8010f0,0x8000);b.write32(0x1f8010b0,0x10000);b.write32(0x1f8010b4,512);b.write32(0x1f8010b8,0x11000000);ck(b.cd.data.empty()&&b.dma_words==512);
 auto sector=b.cd.sector(16);for(unsigned i=0;i<2048;i++)ck(b.ram[0x10000+i]==sector[i]);
 b.tick_timers();ack();write(1,9);b.tick_timers();ck(b.cd.flag==3);ack();b.tick_timers();ck(b.cd.flag==2&&!b.cd.reading);ack();
 write(2,0xff);write(2,2);write(2,0);write(1,2);b.tick_timers();ck(b.cd.flag==5);ack();
 write(1,0x1a);b.tick_timers();ack();b.tick_timers();ck(b.cd.flag==5);ack(); // No fabricated license ID.
 write(1,0xfe);b.tick_timers();ck(b.cd.flag==5);ack();
 auto exe=b.cd.boot_exe();ps1::load_exe(b,c,exe);for(unsigned i=0;i<1000;i++){c.step();}ck(b.gpu.rgb(160,120)==0x3184ff);
 b.gpu.gp0(0xe1003000);ck(b.gpu.draw_mode==0x3000);b.gpu.gp0(0xe2001234);ck(b.gpu.texture_window==0x1234);
 b.gpu.gp1(0x08000001);ck(b.gpu.display[3]==1);
 bool failed=false;try{b.cd.sector(b.cd.sectors);}catch(const std::runtime_error&){failed=true;}ck(failed);
 std::cout<<"PASS: CD registers/commands/events, BCD validation, IRQ latch, sector read + DMA3, ISO directory/exe execution, GP0 E1/E2 + GP1 state, bounds\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
