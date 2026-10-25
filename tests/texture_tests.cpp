#include "core.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void ck(bool value,const char* message){if(!value)throw std::runtime_error(message);}
uint32_t xy(int x,int y){return (uint32_t(x)&65535)|((uint32_t(y)&65535)<<16);}
uint32_t uv(unsigned u,unsigned v,unsigned high=0){return(u&255)|((v&255)<<8)|(high<<16);}
void env(ps1::Gpu& g){g.reset();std::fill(g.vram.begin(),g.vram.end(),0);g.gp0(0xe3000000);g.gp0(0xe4000000|200|(200<<10));}
void rectangle(ps1::Gpu& g,unsigned op,unsigned color,int x,int y,unsigned u,unsigned v,unsigned w,unsigned h,unsigned clut=0){
 g.gp0((op<<24)|(color&0xffffff));g.gp0(xy(x,y));if(op&4)g.gp0(uv(u,v,clut));if(!(op&0x18))g.gp0(xy(int(w),int(h)));
}
void polygon(ps1::Gpu& g,unsigned op,unsigned page=0x10a,unsigned color=0x808080){
 const unsigned n=op&8?4:3;
 const int xs[]={10,14,10,14},ys[]={10,10,14,14};const unsigned us[]={0,4,0,4},vs[]={0,0,4,4};
 g.gp0((op<<24)|(color&0xffffff));for(unsigned i=0;i<n;i++){
  if(i&&(op&16)){g.gp0(color);}g.gp0(xy(xs[i],ys[i]));if(op&4)g.gp0(uv(us[i],vs[i],i==1?page:0));
 }
}
}
int main(){try{
 ps1::Bus bus;auto& g=bus.gpu;
 // Exact reported command 2C808080: neutral modulation on all quad pixels.
 env(g);for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)g.vram[y*1024+640+x]=31;
 polygon(g,0x2c);for(unsigned y=10;y<14;y++)for(unsigned x=10;x<14;x++)ck(g.vram[y*1024+x]==31,"2C808080 neutral quad");
 ck(g.draw_mode==0x10a&&(g.status&511)==0x10a&&g.count==0,"polygon TPage/status");
 g.gp0(0x1f000000);ck(g.status&(1u<<24),"command after nine-word packet");
 // All 32 polygon opcodes: complete framing and following packet, max 12 words.
 for(unsigned op=0x20;op<=0x3f;op++){
  env(g);g.vram[640]=31;polygon(g,op);ck(g.count==0&&g.polygons==1,"polygon family framing");
  g.gp0(0x1f000000);ck(g.status&(1u<<24),"polygon family next command");
 }
 // 4-bit CLUT nibbles, including nonzero palette entry zero (not transparent).
 env(g);g.gp0(0xe100000a);const unsigned clut=300<<6;
 g.vram[640]=0x4321;g.vram[641]=0;
 g.vram[300*1024]=0x7c00;g.vram[300*1024+1]=31;g.vram[300*1024+2]=0x3e0;
 g.vram[300*1024+3]=0x7c00;g.vram[300*1024+4]=0x7fff;
 rectangle(g,0x65,0,10,10,0,0,5,1,clut);
 const uint16_t palette_expected[]={31,0x3e0,0x7c00,0x7fff,0x7c00};
 for(unsigned i=0;i<5;i++)ck(g.vram[10*1024+10+i]==palette_expected[i],"4-bit CLUT nibble/order/index zero");
 // 8-bit CLUT low/high bytes, index 255 and CLUT x coordinate.
 env(g);g.gp0(0xe100008a);g.vram[640]=0xff02;g.vram[300*1024+16+2]=31;g.vram[300*1024+16+255]=0x3e0;
 rectangle(g,0x65,0,10,10,0,0,2,1,clut|1);ck(g.vram[10*1024+10]==31&&g.vram[10*1024+11]==0x3e0,"8-bit palette");
 // Direct color: zero texel skips, 8000h remains opaque black with its mask bit.
 env(g);g.gp0(0xe100010a);g.vram[10*1024+10]=0x7fff;g.vram[641]=0x8000;
 rectangle(g,0x65,0,10,10,0,0,2,1);ck(g.vram[10*1024+10]==0x7fff&&g.vram[10*1024+11]==0x8000,"transparent zero/opaque black");
 // Raw bypasses color; modulation 80h is neutral, 40h darkens, FFh saturates.
 env(g);g.gp0(0xe100010a);g.vram[640]=16;
 rectangle(g,0x65,0,10,10,0,0,1,1);ck(g.vram[10*1024+10]==16,"raw bypass");
 rectangle(g,0x64,0x404040,11,10,0,0,1,1);ck(g.vram[10*1024+11]==8,"modulate half");
 rectangle(g,0x64,0xffffff,12,10,0,0,1,1);ck(g.vram[10*1024+12]==31,"modulate saturation");
 // E2 window replaces masked coordinate bits (both axes).
 g.gp0(0xe2000000|1|(1<<5)|(1<<10)|(1<<15));g.vram[8*1024+648]=0x3e0;
 rectangle(g,0x65,0,13,10,0,0,1,1);ck(g.vram[10*1024+13]==0x3e0,"texture window XY");
 // Direct texture X wraps at the VRAM row edge; page Y selects lower half.
 env(g);g.gp0(0xe100011f);g.vram[511*1024+36]=0x7c00;
 rectangle(g,0x65,0,10,10,100,255,1,1);ck(g.vram[10*1024+10]==0x7c00,"texture VRAM wrap/page Y");
 // Sprite U/V flip, signed offsets and clipping retain original UV phase.
 env(g);g.gp0(0xe100310a);g.vram[3*1024+643]=31;g.vram[2*1024+642]=0x3e0;
 rectangle(g,0x65,0,10,10,3,3,2,2);ck(g.vram[10*1024+10]==31&&g.vram[11*1024+11]==0x3e0,"sprite UV flip");
 g.gp0(0xe5000000|2047|(2047<<11));g.gp0(0xe3000000|11|(11<<10));g.vram[1*1024+641]=0x7c00;
 rectangle(g,0x65,0,10,10,3,3,4,4);ck(g.vram[11*1024+11]==0x7c00,"sprite clip/offset UV phase");
 // All four blending modes, channel saturation and texture STP gate.
 for(unsigned mode=0;mode<4;mode++){
  env(g);g.gp0(0xe1000000|(mode<<5));g.vram[10*1024+10]=10;
  rectangle(g,0x62,48,10,10,0,0,1,1);const unsigned expected[]={8,16,4,11};ck(g.vram[10*1024+10]==expected[mode],"four blend modes");
 }
 env(g);g.gp0(0xe100012a);g.vram[640]=6;g.vram[641]=0x8006;g.vram[10*1024+10]=g.vram[10*1024+11]=10;
 rectangle(g,0x67,0,10,10,0,0,2,1);ck(g.vram[10*1024+10]==6&&g.vram[10*1024+11]==0x8010,"textured STP gate");
 g.gp0(0xe1000020);g.vram[10*1024+12]=30;rectangle(g,0x62,80,12,10,0,0,1,1);ck(g.vram[10*1024+12]==31,"blend add saturation");
 g.gp0(0xe1000040);rectangle(g,0x62,0xff,13,10,0,0,1,1);ck(g.vram[10*1024+13]==0,"blend subtract clamp");
 // Force/check mask must also apply to textured output.
 env(g);g.gp0(0xe100010a);g.vram[640]=31;g.gp0(0xe6000001);rectangle(g,0x65,0,10,10,0,0,1,1);
 ck(g.vram[10*1024+10]==0x801f,"texture force mask");g.gp0(0xe6000002);g.vram[640]=0x3e0;
 rectangle(g,0x65,0,10,10,0,0,1,1);ck(g.vram[10*1024+10]==0x801f,"texture check mask");
 // Dither only Gouraud/modulated polygons; raw textures and rectangles bypass it.
 env(g);g.gp0(0xe1000200);g.gp0(0x30808080);g.gp0(xy(10,10));g.gp0(0x808080);g.gp0(xy(30,10));g.gp0(0x808080);g.gp0(xy(10,30));
 ck((g.vram[12*1024+12]&31)==15&&(g.vram[12*1024+13]&31)==16,"Gouraud dither matrix");
 rectangle(g,0x60,0x808080,12,12,0,0,1,1);ck((g.vram[12*1024+12]&31)==16,"no rectangle dither");
 // Affine Gouraud color and UV interpolation must travel with reversed winding.
 env(g);for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++)g.vram[y*1024+640+x]=0x4210;
 auto shaded_texture=[&](bool reverse){
  g.gp0(0x34800000);g.gp0(xy(10,10));g.gp0(uv(0,0));
  g.gp0(reverse?0x000080:0x008000);g.gp0(reverse?xy(10,30):xy(30,10));g.gp0(uv(reverse?0:20,reverse?20:0,0x10a));
  g.gp0(reverse?0x008000:0x000080);g.gp0(reverse?xy(30,10):xy(10,30));g.gp0(uv(reverse?20:0,reverse?0:20));
 };
 shaded_texture(false);ck(g.vram[15*1024+15]==uint16_t(4|(4<<5)|(8<<10)),"Gouraud texture interpolation");
 const auto image=g.vram;for(unsigned y=0;y<201;y++)std::fill(g.vram.begin()+y*1024,g.vram.begin()+y*1024+201,0);
 shaded_texture(true);ck(g.vram==image,"textured winding swaps attributes");
 // UV interpolation across a triangle samples known different source texels.
 env(g);g.vram[5*1024+645]=0x7c00;
 g.gp0(0x25000000);g.gp0(xy(10,10));g.gp0(uv(0,0));g.gp0(xy(30,10));g.gp0(uv(20,0,0x10a));g.gp0(xy(10,30));g.gp0(uv(0,20));
 ck(g.vram[15*1024+15]==0x7c00,"affine UV interpolation");
 // Raw textures bypass dither; flat untextured polygons are also not dithered.
 env(g);g.gp0(0xe1000200);for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)g.vram[y*1024+640+x]=0x4210;
 polygon(g,0x3d);ck(g.vram[12*1024+12]==0x4210,"raw polygon bypasses dither");
 polygon(g,0x20,0,0x808080);ck(g.vram[12*1024+12]==0x4210,"flat polygon bypasses dither");
 // Four rectangle size classes, all texture/raw/semi aliases and zero-size no-op.
 for(unsigned op=0x60;op<=0x7f;op++){
  env(g);g.gp0(0xe100010a);for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++)g.vram[y*1024+640+x]=31;
  rectangle(g,op,0x808080,10,10,0,0,2,3);ck(g.count==0&&g.rectangles==1,"rectangle framing");
  const unsigned size=(op>>3)&3,w=size==0?2:size==1?1:size==2?8:16,h=size==0?3:w;
  ck(g.vram[(10+h-1)*1024+10+w-1]!=0&&g.vram[(10+h)*1024+10]==0,"rectangle extent");
 }
 env(g);rectangle(g,0x60,0xffffff,10,10,0,0,0,5);ck(g.vram[10*1024+10]==0,"zero-width rectangle");
 // A 12-word Gouraud textured quad retains parser state across DMA list nodes.
 env(g);for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)g.vram[y*1024+640+x]=0x7fff;
 const uint32_t stream[]={0x3c808080,xy(10,10),uv(0,0),0x808080,xy(14,10),uv(4,0,0x10a),0x808080,xy(10,14),uv(0,4),0x808080,xy(14,14),uv(4,4),0x1f000000};
 bus.write32(0x1000,0x05002000);for(unsigned i=0;i<5;i++)bus.write32(0x1004+4*i,stream[i]);
 bus.write32(0x2000,0x08ffffff);for(unsigned i=0;i<8;i++)bus.write32(0x2004+4*i,stream[5+i]);
 bus.write32(0x1f8010f0,0x800);bus.write32(0x1f8010a0,0x1000);bus.write32(0x1f8010a8,0x01000401);
 ck(g.vram[12*1024+12]==0x7fff&&g.count==0&&(g.status&(1u<<24)),"12-word DMA split quad");
 g.gp0(0x3c808080);g.gp0(xy(10,10));g.gp1(0x01000000);ck(g.count==0,"partial texture packet reset");
 g.gp1(0x0800007f);ck((g.status&0x7f0000)==0x7f0000,"GPUSTAT display mode bits");
 ps1::Cpu cpu(bus);ck(cpu.diagnostic().find("GPUSTAT=")!=std::string::npos,"GPU diagnostic registers");
 const unsigned n=ps1::install_texture_demo(bus);cpu.reset(0);for(unsigned i=0;i<n;i++)cpu.step();
 ck(g.vram[30*1024+30]==0x03ff,"CPU texture demo neutral sample");
 ck(g.vram[30*1024+182]==uint16_t(15|(15<<5)),"CPU texture demo dark sample");
 ck(g.vram[176*1024+24]==0x03ff&&g.polygons==2&&g.rectangles==3,"CPU texture demo sprite/counters");
 std::cout<<"PASS: polygon/rectangle families, exact 2C808080, 4/8/15-bit textures, CLUT, raw/modulation, window, wrap, flip, masks, blend, dither, DMA packet split, GPUSTAT/log\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
