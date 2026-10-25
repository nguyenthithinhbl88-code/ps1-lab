#include "core.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
void ck(bool value){if(!value)throw std::runtime_error("line test failed");}
uint32_t xy(int x,int y){return(uint32_t(x)&65535)|((uint32_t(y)&65535)<<16);}
int main(){try{
 ps1::Bus b;auto& g=b.gpu;
 auto reset=[&](){g.reset();std::fill(g.vram.begin(),g.vram.end(),0);g.gp0(0xe3000000);g.gp0(0xe4000000|100|(100<<10));};
 reset();g.gp0(0x400000ff);g.gp0(xy(10,10));g.gp0(xy(20,10));for(unsigned x=10;x<=20;x++)ck(g.vram[10*1024+x]==31);ck(g.vram[10*1024+21]==0);
 reset();g.gp0(0x500000ff);g.gp0(xy(10,10));g.gp0(0xff0000);g.gp0(xy(20,10));ck(g.vram[10*1024+15]==uint16_t(15|(15<<10)));ck(g.vram[10*1024+20]==0x7c00);
 reset();g.gp0(0x480000ff);g.gp0(xy(10,10));g.gp0(xy(20,10));ck(g.polyline_active);g.gp0(xy(20,20));g.gp0(0x50005000);ck(!g.polyline_active&&g.line_segments==2&&g.vram[20*1024+20]==31);
 g.gp0(0x1f000000);ck(g.status&(1u<<24));
 reset();g.gp0(0x580000ff);g.gp0(xy(10,10));g.gp0(0x00ff00);g.gp0(xy(20,10));g.gp0(0xff0000);g.gp0(xy(20,20));g.gp0(0x50005000);ck(!g.polyline_active&&g.vram[20*1024+20]==0x7c00&&g.line_segments==2);
 reset();g.gp0(0xe5000000|2047|(2047<<11));g.gp0(0xe3000000|14|(14<<10));g.gp0(0xe4000000|16|(16<<10));g.gp0(0xe6000001);
 g.gp0(0x400000ff);g.gp0(xy(10,16));g.gp0(xy(20,16));ck(g.vram[15*1024+15]==0x801f&&g.vram[15*1024+13]==0&&g.vram[15*1024+17]==0);
 g.gp0(0xe6000002);g.gp0(0x4000ff00);g.gp0(xy(10,16));g.gp0(xy(20,16));ck(g.vram[15*1024+15]==0x801f);
 reset();g.vram[10*1024+10]=10;g.gp0(0x42000030);g.gp0(xy(10,10));g.gp0(xy(10,10));ck(g.vram[10*1024+10]==8);
 reset();g.gp0(0x480000ff);g.gp0(xy(10,10));g.gp0(xy(20,10));g.gp1(0x01000000);ck(!g.polyline_active&&g.count==0);g.gp0(0);
 reset();g.gp0(0x400000ff);g.gp0(xy(-1024,10));g.gp0(xy(1023,10));ck(g.line_segments==0);
 std::cout<<"PASS: flat/Gouraud lines, endpoints/degenerate, polyline terminators, clip/offset/mask/blend, parser reset and bounds\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
