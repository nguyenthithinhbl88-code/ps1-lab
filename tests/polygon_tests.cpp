#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v){if(!v)throw std::runtime_error("polygon test failed");}
int main(){try{
 ps1::Bus b;auto& g=b.gpu;
 auto xy=[](int x,int y){return(uint32_t(y)&65535)<<16|(uint32_t(x)&65535);};
 auto env=[&](){g.gp0(0xe3000000);g.gp0(0xe4000000|100|(100<<10));};
 auto tri=[&](bool reverse){g.gp0(0x200000ff);g.gp0(xy(10,10));g.gp0(reverse?xy(10,30):xy(30,10));g.gp0(reverse?xy(30,10):xy(10,30));};
 env();tri(false);ck(g.vram[15*1024+15]==31);ck(g.vram[30*1024+30]==0);
 const auto original=g.vram;std::fill(g.vram.begin(),g.vram.end(),0);tri(true);ck(original==g.vram);
 // Adjacent triangles in a quad must cover the interior rectangle.
 std::fill(g.vram.begin(),g.vram.end(),0);g.gp0(0x2800ff00);g.gp0(xy(10,10));g.gp0(xy(30,10));g.gp0(xy(10,30));g.gp0(xy(30,30));
 for(int y=10;y<30;y++)for(int x=10;x<30;x++)ck(g.vram[y*1024+x]==0x3e0);
 // Clip bounds are inclusive; offset is sign-extended from 11 bits.
 std::fill(g.vram.begin(),g.vram.end(),0);g.gp0(0xe3000000|14|(14<<10));g.gp0(0xe4000000|16|(16<<10));tri(false);
 ck(g.vram[15*1024+15]==31);ck(g.vram[13*1024+15]==0);ck(g.vram[15*1024+17]==0);
 env();g.gp0(0xe5000000|2047|(2047<<11));std::fill(g.vram.begin(),g.vram.end(),0);tri(false);ck(g.vram[9*1024+9]==31);ck(g.offset_x==-1&&g.offset_y==-1);
 g.gp0(0xe5000000);g.gp0(0xe6000001);tri(false);ck(g.vram[15*1024+15]==0x801f);
 g.gp0(0xe6000002);g.gp0(0x2000ff00);g.gp0(xy(10,10));g.gp0(xy(30,10));g.gp0(xy(10,30));ck(g.vram[15*1024+15]==0x801f);
 // Degenerate triangle is empty.
 std::fill(g.vram.begin(),g.vram.end(),0);g.gp0(0xe6000000);g.gp0(0x200000ff);g.gp0(xy(10,10));g.gp0(xy(20,20));g.gp0(xy(30,30));for(auto p:g.vram)ck(p==0);
 // Gouraud RGB interpolation, winding, six-word framing and next command.
 g.reset();env();std::fill(g.vram.begin(),g.vram.end(),0);
 auto shade=[&](bool reverse){
  g.gp0(0x300000ff);g.gp0(xy(10,10));
  g.gp0(reverse?0x00ff0000:0x0000ff00);g.gp0(reverse?xy(10,30):xy(30,10));
  g.gp0(reverse?0x0000ff00:0x00ff0000);g.gp0(reverse?xy(30,10):xy(10,30));
 };
 shade(false);ck(g.vram[15*1024+15]==uint16_t(15|(7<<5)|(7<<10)));
 const auto shaded=g.vram;std::fill(g.vram.begin(),g.vram.end(),0);shade(true);ck(g.vram==shaded);
 g.gp0(0x1f000000);ck(g.count==0&&(g.status&(1u<<24)));g.gp1(0x02000000);
 // Exact reported opcode, eight words, two triangles and independent vertex colors.
 std::fill(g.vram.begin(),g.vram.end(),0);
 g.gp0(0x380000b2);g.gp0(xy(10,10));g.gp0(0x000000b2);g.gp0(xy(30,10));
 g.gp0(0x000000b2);g.gp0(xy(10,30));g.gp0(0x000000b2);
 ck(g.count==7&&g.vram[15*1024+15]==0);g.gp0(xy(30,30));
 for(int y=10;y<30;y++)for(int x=10;x<30;x++)ck(g.vram[y*1024+x]==22);
 ck(g.count==0);g.gp0(0x1f000000);ck(g.status&(1u<<24));
 // Fourth color belongs to vertex 3; payload high bytes are not GP0 opcodes.
 g.gp0(0x38000000);g.gp0(xy(10,10));g.gp0(0xe1000000);g.gp0(xy(30,10));
 g.gp0(0xe2000000);g.gp0(xy(10,30));g.gp0(0xe30000ff);g.gp0(xy(30,30));
 ck(g.vram[25*1024+25]==15&&g.draw_mode==0&&g.count==0);
 // Reset discards a partial Gouraud packet; clip and mask also apply to shading.
 g.gp0(0x380000ff);g.gp0(xy(0,0));g.gp1(0x01000000);ck(g.count==0);
 g.gp0(0xe6000001);shade(false);ck(g.vram[15*1024+15]&0x8000);
 const auto masked=g.vram[15*1024+15];g.gp0(0xe6000002);shade(true);ck(g.vram[15*1024+15]==masked);
 // DMA linked-list transport must preserve the eight-word quad packet.
 g.reset();env();std::fill(g.vram.begin(),g.vram.end(),0);
 const uint32_t words[]={0x380000b2,xy(10,10),0xb2,xy(30,10),0xb2,xy(10,30),0xb2,xy(30,30),0x1f000000};
 b.write32(0x1000,0x09ffffff);
 for(unsigned j=0;j<9;j++)b.write32(0x1004+j*4,words[j]);
 b.write32(0x1f8010f0,0x800);b.write32(0x1f8010a0,0x1000);b.write32(0x1f8010a8,0x01000401);
 ck(g.vram[25*1024+25]==22&&g.count==0&&(g.status&(1u<<24)));
 g.gp0(0xe3000000|14|(14<<10));g.gp0(0xe4000000|16|(16<<10));
 std::fill(g.vram.begin(),g.vram.end(),0);shade(false);
 ck(g.vram[15*1024+15]!=0&&g.vram[13*1024+15]==0&&g.vram[15*1024+17]==0);
 ps1::Cpu c(b);unsigned n=ps1::install_polygon_demo(b);c.reset(0);for(unsigned i=0;i<n;i++)c.step();ck(g.rgb(160,110)==0x3184ff);ck(g.rgb(160,30)==0xff4200);
 std::cout<<"PASS: triangles, winding, quad coverage, clipping, signed offset, mask, degenerate, CPU polygon demo, Gouraud triangle/quad, packet reset/framing\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
