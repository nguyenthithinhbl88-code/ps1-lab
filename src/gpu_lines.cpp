#include "gpu.hpp"
#include <algorithm>
#include <cstdlib>
namespace ps1 {
namespace {int sx(uint32_t n){const int v=n&2047;return(v&1024)?v-2048:v;}}
void Gpu::render_line(Vertex a,Vertex b,bool semi){
 const int dx=std::abs(b.x-a.x),dy=std::abs(b.y-a.y);if(dx>=1024||dy>=512)return;
 const int n=std::max(dx,dy),step_x=a.x<b.x?1:-1,step_y=a.y<b.y?1:-1;
 int x=a.x,y=a.y,error=dx-dy;
 for(int i=0;;i++){
  uint32_t color=a.color;
  if(n){color=0;for(unsigned shift=0;shift<24;shift+=8)color|=uint32_t(((n-i)*int((a.color>>shift)&255)+i*int((b.color>>shift)&255))/n)<<shift;}
  if(x>=0&&x<=1023&&y>=0&&y<=511&&x>=left&&x<=right&&y>=top&&y<=bottom)fragment(x,y,color,0,false,false,semi,(draw_mode&0x200)!=0);
  if(x==b.x&&y==b.y)break;
  const int twice=2*error;if(twice>-dy){error-=dy;x+=step_x;}if(twice<dx){error+=dx;y+=step_y;}
 }
 line_segments++;
}
void Gpu::line_packet(){
 const unsigned op=packet[0]>>24;const bool shaded=op&16;
 auto point=[&](uint32_t xy,uint32_t color){return Vertex{sx(xy)+offset_x,sx(xy>>16)+offset_y,0,0,color};};
 auto a=point(packet[1],packet[0]);auto b=point(packet[shaded?3:2],shaded?packet[2]:packet[0]);render_line(a,b,op&2);
 if(op&8){polyline_active=true;polyline_opcode=op;polyline_color_pending=false;polyline_vertex=b;polyline_color=packet[0];}
}
void Gpu::polyline_word(uint32_t word){
 const bool shaded=polyline_opcode&16;
 if(!polyline_color_pending&&(word&0xf000f000u)==0x50005000u){polyline_active=false;return;}
 if(shaded&&!polyline_color_pending){polyline_color=word;polyline_color_pending=true;return;}
 Vertex next{sx(word)+offset_x,sx(word>>16)+offset_y,0,0,polyline_color};
 render_line(polyline_vertex,next,polyline_opcode&2);polyline_vertex=next;polyline_color_pending=false;
}
}
