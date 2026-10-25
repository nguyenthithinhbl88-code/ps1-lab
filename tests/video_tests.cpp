#include "core.hpp"
#include "display.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void ck(bool condition,const char* reason){if(!condition)throw std::runtime_error(reason);}
void scanline(ps1::Bus& b){const auto frame=b.gpu.video_frames;const auto line=b.gpu.video_line;for(unsigned i=0;i<3000;i++){b.tick_timers();if(b.gpu.video_line!=line||b.gpu.video_frames!=frame)return;}throw std::runtime_error("scanline did not advance");}
void line(ps1::Bus& b,unsigned target){for(unsigned i=0;i<315;i++){if(b.gpu.video_line==target)return;scanline(b);}throw std::runtime_error("target line not reached");}
void mode(ps1::Bus& b,unsigned m){b.gpu.reset();b.gpu.gp1(0x07000000|16|(256<<10));b.gpu.gp1(0x08000000|m);}
}
int main(){try{
 ps1::Bus b;auto& g=b.gpu;
 // Reproduce v14's mode/status combination while display output remains disabled.
 mode(b,0x27);ck(g.status&(1u<<19),"480i mode bit");ck(g.status&(1u<<23),"display starts disabled");
 const auto initial=g.status;for(unsigned i=0;i<1000;i++)ck(b.read32(0x1f801814)==initial,"MMIO read cannot advance video");
 ck(g.video_frames==0&&g.video_line==0,"reads are side-effect free");
 line(b,16);ck(!(g.status&0x80000000u),"first field even");scanline(b);ck(!(g.status&0x80000000u),"480i parity stable across active lines");
 line(b,256);ck(g.video_vblank&&(b.irq_status&1)&&g.vblank_events==1,"VBlank entry latches IRQ0");
 b.write32(0x1f801070,~1u);scanline(b);ck(!(b.irq_status&1),"acknowledge during VBlank does not retrigger level");
 line(b,0);ck(g.video_frames==1&&g.video_field,"field wrap");line(b,16);ck(g.status&0x80000000u,"odd field despite display disabled");
 ck(!(g.status&0x2000),"interlace field indicator changes");
 line(b,256);ck(!(g.status&0x80000000u)&&(b.irq_status&1)&&g.vblank_events==2,"VBlank clears bit31 and reasserts next IRQ");
 // Progressive display changes bit31 by line and keeps field bit13 set.
 mode(b,1);line(b,16);ck(!(g.status&0x80000000u)&&(g.status&0x2000),"progressive even");
 scanline(b);ck((g.status&0x80000000u)&&(g.status&0x2000),"progressive odd");
 g.gp1(0x07000000|20|(30<<10));ck(g.video_vblank&&!(g.status&0x80000000u),"range changes blanking");
 line(b,21);ck(!g.video_vblank&&(g.status&0x80000000u),"range active line");
 // PAL mode uses a different total: no wrap at NTSC line263.
 mode(b,9);line(b,263);ck(g.video_frames==0&&g.video_line==263,"PAL total lines");line(b,0);ck(g.video_frames==1,"PAL wrap at314");
 // IRQ0 reaches CPU through the existing interrupt controller, not a BIOS shortcut.
 ps1::Cpu cpu(b);mode(b,1);line(b,255);b.irq_status=0;b.irq_mask=1;cpu.reset(0x1000);cpu.cop0[12]=0x401;
 b.write32(0x1000,0x08000400);b.write32(0x1004,0);
 for(unsigned i=0;i<4000&&cpu.interrupts==0;i++)cpu.step();
 ck(cpu.interrupts==1&&cpu.pc==0x80000080&&cpu.cop0[14]>=0x1000&&cpu.cop0[14]<=0x1004,"CPU services IRQ0");
 // Regression: an actual MIPS loop polls GPUSTAT bit31 until it changes, then
 // enables the display and records success. No Sony BIOS bytes are used.
 mode(b,0x27);cpu.reset(0x1000);b.irq_status=b.irq_mask=0;
 const uint32_t code[]={
  0x3c081f80,0x35081814, // t0 = GPUSTAT
  0x8d090000,0,          // initial t1 (load delay)
  0x8d0a0000,0,          // loop: t2 = current GPUSTAT
  0x012a5826,             // xor t3,t1,t2
  0x000b5fc2,             // srl t3,t3,31
  0x1160fffb,0,          // beq t3,zero,loop; nop
  0x3c0c0300,0xad0c0000, // GP1 display enable
  0x240d0001,0xac0d0100, // success marker RAM[100] = 1
  0x0800040e,0           // finished loop
 };
 for(unsigned i=0;i<sizeof(code)/sizeof(code[0]);i++)b.write32(0x1000+i*4,code[i]);
 g.vram[0]=31;
 const unsigned limit=1300000;
 for(unsigned i=0;i<limit&&b.read32(0x100)!=1;i++)cpu.step();
 ck(b.read32(0x100)==1&&cpu.instructions<limit,"MIPS GPUSTAT polling loop terminates");
 ck(!(g.status&(1u<<23))&&!ps1::display_view(g).disabled,"BIOS-style display enable reached");
 ck(ps1::display_pixel(g,ps1::display_view(g),0,0)==0xff0000,"visible framebuffer after polling");
 ck(cpu.diagnostic().find("vblank_events=")!=std::string::npos,"video diagnostics");
 g.gp1(0);ck(g.video_frames==0&&g.video_line==0&&g.video_dot==0&&g.video_fraction==0&&g.status==0x14802000,"reset video state");
 std::cout<<"PASS: 480i/progressive parity, side-effect-free status reads, display-disabled clock, PAL/NTSC fields, VBlank IRQ0/ack/CPU, bounded MIPS polling regression\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
