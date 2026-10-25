#pragma once
#include "gpu.hpp"
#include <algorithm>
namespace ps1 {
// Functional presentation only: no video clocks, field timing or CRT overscan.
struct DisplayView {unsigned x=0,y=0,width=320,height=240;bool rgb24=false,disabled=false;};
inline DisplayView display_view(const Gpu& gpu){
 DisplayView v;if(!gpu.display_configured)return v;
 const auto mode=gpu.display[3];
 const unsigned widths[]={256,320,512,640},clocks[]={10,8,5,4};
 v.x=gpu.display[0]&1023;v.y=(gpu.display[0]>>10)&511;
 v.width=(mode&64)?368:widths[mode&3];
 const unsigned x1=gpu.display[1]&4095,x2=(gpu.display[1]>>12)&4095;
 if(x2>x1)v.width=std::clamp((((x2-x1)/((mode&64)?7:clocks[mode&3]))+2)&~3u,4u,1024u);
 const unsigned y1=gpu.display[2]&1023,y2=(gpu.display[2]>>10)&1023;
 const unsigned scale=(mode&0x24)==0x24?2:1;
 v.height=std::clamp((y2>y1?y2-y1:240)*scale,1u,512u);
 v.rgb24=mode&16;v.disabled=(gpu.status&(1u<<23))!=0;return v;
}
inline uint32_t display_pixel(const Gpu& gpu,const DisplayView& v,unsigned x,unsigned y){
 if(v.disabled)return 0;
 if(!v.rgb24)return gpu.rgb(v.x+x,v.y+y);
 const unsigned row=((v.y+y)&511)*1024;
 auto byte=[&](unsigned i){i&=2047;return unsigned(gpu.vram[row+i/2]>>((i&1)*8))&255;};
 const unsigned start=v.x*2+x*3;
 return (byte(start)<<16)|(byte(start+1)<<8)|byte(start+2);
}
struct ViewRect {int x=0,y=0,width=0,height=0;};
inline ViewRect fit_display(int width,int height){
 if(width<=0||height<=0)return {};
 int w=width,h=width*3/4;if(h>height){h=height;w=height*4/3;}
 return {(width-w)/2,(height-h)/2,w,h};
}
}
