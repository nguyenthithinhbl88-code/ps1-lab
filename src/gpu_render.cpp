#include "gpu.hpp"
#include <algorithm>
namespace ps1 {
namespace {
int sx11(uint32_t word){const int n=word&2047;return n&1024?n-2048:n;}
int64_t edge(const Gpu::Vertex& a,const Gpu::Vertex& b,const Gpu::Vertex& p){return int64_t(b.x-a.x)*(p.y-a.y)-int64_t(b.y-a.y)*(p.x-a.x);}
bool inclusive(const Gpu::Vertex& a,const Gpu::Vertex& b){return b.y<a.y||(b.y==a.y&&b.x>a.x);}
}
uint16_t Gpu::texel(unsigned u,unsigned v,uint16_t clut)const{
 const unsigned mx=(texture_window&31)*8,my=((texture_window>>5)&31)*8;
 u=((u&255)&~mx)|((((texture_window>>10)&31)*8)&mx);
 v=((v&255)&~my)|((((texture_window>>15)&31)*8)&my);
 const unsigned base_x=(draw_mode&15)*64,base_y=((draw_mode>>4)&1)*256;
 const unsigned depth=(draw_mode>>7)&3;
 const unsigned divisor=depth==0?4:depth==1?2:1;
 uint16_t value=vram[((base_y+v)&511)*1024+((base_x+u/divisor)&1023)];
 if(depth<2){
  const unsigned index=depth==0?(value>>((u&3)*4))&15:(value>>((u&1)*8))&255;
  value=vram[((clut>>6)&511)*1024+(((clut&63)*16+index)&1023)];
 }
 return value;
}
void Gpu::fragment(int x,int y,uint32_t color,uint16_t texture,bool textured,bool raw,bool semi,bool dither){
 if(textured&&texture==0)return;
 uint16_t value=0;
 if(textured&&raw)value=texture;
 else{
  static constexpr int matrix[4][4]={{-4,0,-3,1},{2,-2,3,-1},{-3,1,-4,0},{3,-1,2,-2}};
  const int adjustment=dither?matrix[y&3][x&3]:0;
  for(unsigned c=0;c<3;c++){
   int channel=(color>>(c*8))&255;
   if(textured)channel=int((texture>>(c*5))&31)*channel/16;
   channel=std::clamp(channel+adjustment,0,255);
   value|=uint16_t((channel>>3)<<(c*5));
  }
  if(textured)value|=texture&0x8000;
 }
 const unsigned index=unsigned(y*1024+x);
 if(semi&&(!textured||(texture&0x8000))){
  const auto back=vram[index];uint16_t blended=value&0x8000;
  for(unsigned c=0;c<3;c++){
   const int b=(back>>(c*5))&31,f=(value>>(c*5))&31;int result=0;
   switch((draw_mode>>5)&3){case 0:result=b/2+f/2;break;case 1:result=b+f;break;case 2:result=b-f;break;case 3:result=b+f/4;break;}
   blended|=uint16_t(std::clamp(result,0,31)<<(c*5));
  }
  value=blended;
 }
 pixel(index,value);
}
void Gpu::render_triangle(Vertex a,Vertex b,Vertex c,uint16_t clut,bool textured,bool raw,bool semi,bool dither){
 int64_t area=edge(a,b,c);if(!area)return;
 if(std::max({a.x,b.x,c.x})-std::min({a.x,b.x,c.x})>=1024||std::max({a.y,b.y,c.y})-std::min({a.y,b.y,c.y})>=512)return;
 if(area<0){std::swap(b,c);area=-area;}
 const int x0=std::max({0,left,std::min({a.x,b.x,c.x})}),x1=std::min({1023,right,std::max({a.x,b.x,c.x})});
 const int y0=std::max({0,top,std::min({a.y,b.y,c.y})}),y1=std::min({511,bottom,std::max({a.y,b.y,c.y})});
 const bool flat=(a.color&0xffffff)==(b.color&0xffffff)&&(a.color&0xffffff)==(c.color&0xffffff);
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  const Vertex p{x,y,0,0,0};const auto w0=edge(b,c,p),w1=edge(c,a,p),w2=edge(a,b,p);
  if(w0<0||w1<0||w2<0||(w0==0&&!inclusive(b,c))||(w1==0&&!inclusive(c,a))||(w2==0&&!inclusive(a,b)))continue;
  uint32_t color=a.color;
  if(!flat){color=0;for(unsigned shift=0;shift<24;shift+=8)color|=uint32_t((w0*((a.color>>shift)&255)+w1*((b.color>>shift)&255)+w2*((c.color>>shift)&255))/area)<<shift;}
  uint16_t sample=0;
  if(textured){const unsigned u=unsigned((w0*a.u+w1*b.u+w2*c.u)/area),v=unsigned((w0*a.v+w1*b.v+w2*c.v)/area);sample=texel(u,v,clut);}
  fragment(x,y,color,sample,textured,raw,semi,dither);
 }
}
void Gpu::polygon_packet(){
 const unsigned op=packet[0]>>24,n=(op&8)?4:3;const bool textured=op&4,gouraud=op&16,raw=op&1,semi=op&2;
 std::array<Vertex,4> vertices{};unsigned cursor=1;uint16_t clut=0;
 for(unsigned i=0;i<n;i++){
  auto& v=vertices[i];v.color=i&&gouraud?packet[cursor++]:packet[0];
  const uint32_t xy=packet[cursor++];v.x=sx11(xy)+offset_x;v.y=sx11(xy>>16)+offset_y;
  if(textured){const uint32_t uv=packet[cursor++];v.u=uv&255;v.v=(uv>>8)&255;
   if(i==0)clut=uint16_t(uv>>16);
   if(i==1){draw_mode=(draw_mode&~0x1ffu)|((uv>>16)&0x1ff);status=(status&~0x1ffu)|(draw_mode&0x1ff);}
  }
 }
 const bool dither=(draw_mode&0x200)&&(gouraud||(textured&&!raw));
 render_triangle(vertices[0],vertices[1],vertices[2],clut,textured,raw,semi,dither);
 if(n==4)render_triangle(vertices[1],vertices[2],vertices[3],clut,textured,raw,semi,dither);
}
void Gpu::rectangle_packet(){
 const unsigned op=packet[0]>>24;const bool textured=op&4,raw=op&1,semi=op&2;
 const int x=sx11(packet[1])+offset_x,y=sx11(packet[1]>>16)+offset_y;
 unsigned w=1,h=1;const unsigned size=(op>>3)&3;
 if(!size){const uint32_t wh=packet[textured?3:2];w=wh&1023;h=(wh>>16)&511;}
 else if(size==2)w=h=8;else if(size==3)w=h=16;
 const unsigned u=textured?packet[2]&255:0,v=textured?(packet[2]>>8)&255:0;
 const uint16_t clut=textured?uint16_t(packet[2]>>16):0;
 const int du=(draw_mode&0x1000)?-1:1,dv=(draw_mode&0x2000)?-1:1;
 const int x0=std::max({0,left,x}),x1=std::min({1023,right,x+int(w)-1});
 const int y0=std::max({0,top,y}),y1=std::min({511,bottom,y+int(h)-1});
 for(int py=y0;py<=y1;py++)for(int px=x0;px<=x1;px++){
  const uint16_t sample=textured?texel(unsigned(int(u)+(px-x)*du),unsigned(int(v)+(py-y)*dv),clut):0;
  fragment(px,py,packet[0],sample,textured,raw,semi,false);
 }
}
}

#include "core.hpp"
namespace ps1 {
unsigned install_texture_demo(Bus& bus){
 bus.gpu.reset();unsigned n=0;
 auto emit=[&](uint32_t word){bus.write32(n++*4,word);};
 emit(0x3c081f80);emit(0x35081810);
 auto send=[&](uint32_t word){emit(0x3c090000|(word>>16));emit(0x35290000|(word&65535));emit(0xad090000);};
 auto xy=[](unsigned x,unsigned y){return x|(y<<16);};
 send(0x02201810);send(0);send(xy(320,240));
 send(0xe3000000);send(0xe4000000|319|(239<<10));send(0xe5000000);send(0xe6000000);
 // Original 8x8 checkerboard uploaded through actual GP0 CPU-to-VRAM commands.
 send(0xa0000000);send(640);send(xy(8,8));
 for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x+=2){
  const uint16_t a=((x/2+y/2)&1)?0x7c1f:0x03ff;
  const uint16_t b=(((x+1)/2+y/2)&1)?0x7c1f:0x03ff;
  send(uint32_t(a)|(uint32_t(b)<<16));
 }
 auto quad=[&](unsigned x,unsigned y,uint32_t color){
  send(0x2c000000|color);send(xy(x,y));send(0);send(xy(x+120,y));send((0x10au<<16)|8);
  send(xy(x,y+120));send(8<<8);send(xy(x+120,y+120));send(8|(8<<8));
 };
 quad(24,24,0x808080);quad(176,24,0x404040);
 send(0xe100010a);
 send(0x65000000);send(xy(24,176));send(0);send(xy(8,8));
 send(0x6000c0ff);send(xy(48,176));send(xy(96,16));
 send(0xe100012a);send(0x620000ff);send(xy(96,184));send(xy(96,24));
 return n;
}
}
