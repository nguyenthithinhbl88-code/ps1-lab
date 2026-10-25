#include "gpu.hpp"
#include "core.hpp"
#include <stdexcept>
#include <algorithm>
#include <sstream>
namespace ps1 {
void Gpu::reset(){read_w=read_h=read_index=read_latch=0;words=polygons=rectangles=line_segments=0;polyline_active=polyline_color_pending=false;history.fill(0);count=0;upload_w=upload_h=upload_index=0;status=0x14802000;draw_mode=texture_window=0;display.fill(0);display_configured=false;video_fraction=video_dot=video_line=0;video_field=false;video_vblank=true;video_frames=vblank_events=0;left=top=right=bottom=offset_x=offset_y=0;force_mask=check_mask=false;}
uint32_t Gpu::rgb(unsigned x,unsigned y)const{
 const uint16_t c=vram[(y&511)*1024+(x&1023)];
 auto expand=[](unsigned v){return(v<<3)|(v>>2);};
 return(expand(c&31)<<16)|(expand((c>>5)&31)<<8)|expand((c>>10)&31);
}
void Gpu::gp1(uint32_t word){
 switch(word>>24){
 case 0:reset();break;
 case 1:polyline_active=polyline_color_pending=false;read_w=0;status&=~(1u<<27);count=0;upload_w=upload_h=upload_index=0;break;
 case 2:status&=~(1u<<24);break;
 case 3:status=(status&~(1u<<23))|((word&1)<<23);break;
 case 5:case 6:case 7:display[(word>>24)-5]=word&0xffffff;display_configured=true;update_video_status();break;
 case 8:
  display[3]=word&255;display_configured=true;
  status=(status&~0x007f4000u)|((word&0x3f)<<17)|((word&64)<<10)|((word&128)<<7);update_video_status();break;
 case 4:status=(status&~(3u<<29))|((word&3)<<29);break;
 default:
  if((word>>24)>=0x10&&(word>>24)<=0x1f){switch(word&7){
   case 2:read_latch=texture_window;break;
   case 3:read_latch=uint32_t(left|(top<<10));break;
   case 4:read_latch=uint32_t(right|(bottom<<10));break;
   case 5:read_latch=(uint32_t(offset_x)&2047)|((uint32_t(offset_y)&2047)<<11);break;
   default:break;}break;}
  throw std::runtime_error("Unsupported GP1 display command");
 }
}
uint32_t Gpu::read_gp0(){
 if(!read_w)return read_latch;
 uint32_t v=0;
 for(unsigned i=0;i<2&&read_index<read_w*read_h;i++,read_index++){
  unsigned x=(read_x+read_index%read_w)&1023,y=(read_y+read_index/read_w)&511;
  v|=uint32_t(vram[y*1024+x])<<(i*16);
 }
 if(read_index==read_w*read_h){read_w=0;status&=~(1u<<27);}
 read_latch=v;return v;
}
static int sx11(uint32_t v){v&=2047;return v&1024?int(v)-2048:int(v);}
void Gpu::pixel(unsigned index,uint16_t value){
 if(check_mask&&(vram[index]&0x8000))return;
 vram[index]=uint16_t(value|(force_mask?0x8000:0));
}
void Gpu::triangle(uint32_t color,uint32_t a,uint32_t b,uint32_t c){
 shaded_triangle(color,a,color,b,color,c);
}
void Gpu::shaded_triangle(uint32_t c0,uint32_t a,uint32_t c1,uint32_t b,uint32_t c2,uint32_t c){
 auto point=[&](uint32_t xy,uint32_t color){return Vertex{sx11(xy)+offset_x,sx11(xy>>16)+offset_y,0,0,color};};
 render_triangle(point(a,c0),point(b,c1),point(c,c2),0,false,false,false,false);
}
void Gpu::gp0(uint32_t word){
 history[words++%history.size()]=word;
 if(upload_w){
  for(unsigned i=0;i<2 && upload_index<upload_w*upload_h;i++,upload_index++){
   unsigned x=(upload_x+upload_index%upload_w)&1023,y=(upload_y+upload_index/upload_w)&511;
   pixel(y*1024+x,uint16_t(word>>(i*16)));
  }
  if(upload_index==upload_w*upload_h)upload_w=0;
  return;
 }
 if(polyline_active){polyline_word(word);return;}
 if(!count){
  unsigned op=word>>24;
  if(op==0||op==1)return;
  if(op==0x1f){status|=1u<<24;return;}
  if(op==0xe1){draw_mode=word&0x3fff;status=(status&~0x87ffu)|(word&0x7ff)|((word&0x800)<<4);return;}
  if(op==0xe2){texture_window=word&0xfffff;return;}
  if(op==0xe3){left=word&1023;top=(word>>10)&511;return;}
  if(op==0xe4){right=word&1023;bottom=(word>>10)&511;return;}
  if(op==0xe5){offset_x=sx11(word);offset_y=sx11(word>>11);return;}
  if(op==0xe6){force_mask=word&1;check_mask=word&2;status=(status&~0x1800u)|((word&3)<<11);return;}
  if(op!=2&&op!=0xa0&&!(op>=0x20&&op<=0x5f)&&!(op>=0x60&&op<=0x7f)&&op!=0xc0&&op!=0x80){std::ostringstream s;s<<"Unsupported GP0 word 0x"<<std::hex<<word;throw std::runtime_error(s.str());}
 }
 packet[count++]=word;
 const unsigned op=packet[0]>>24;
 unsigned needed=op==0x80?4:3;
 if(op>=0x20&&op<=0x3f){const unsigned vertices=(op&8)?4:3;needed=1+vertices*(1+((op&4)?1:0))+((op&16)?vertices-1:0);}
 if(op>=0x40&&op<=0x5f)needed=(op&16)?4:3;
 if(op>=0x60&&op<=0x7f)needed=2+((op&4)?1:0)+((op&0x18)?0:1);
 if(count<needed)return;
 count=0;
 if(op>=0x20&&op<=0x3f){polygon_packet();polygons++;return;}
 if(op>=0x40&&op<=0x5f){line_packet();return;}
 if(op>=0x60&&op<=0x7f){rectangle_packet();rectangles++;return;}
 if(op==0xc0){
  read_x=packet[1]&1023;read_y=(packet[1]>>16)&511;read_w=((packet[2]-1)&1023)+1;read_h=(((packet[2]>>16)-1)&511)+1;read_index=0;status|=1u<<27;return;
 }
 if(op==0x80){
  unsigned sx=packet[1]&1023,sy=(packet[1]>>16)&511,dx=packet[2]&1023,dy=(packet[2]>>16)&511;
  unsigned w=((packet[3]-1)&1023)+1,h=(((packet[3]>>16)-1)&511)+1;
  // Sequential copy is a functional approximation for overlapping hardware blocks.
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++)pixel(((dy+y)&511)*1024+((dx+x)&1023),vram[((sy+y)&511)*1024+((sx+x)&1023)]);
  return;
 }
 unsigned x=packet[1]&1023,y=(packet[1]>>16)&511;
 if((packet[0]>>24)==0xa0){
  upload_x=x;upload_y=y;upload_w=((packet[2]-1)&1023)+1;
  upload_h=(((packet[2]>>16)-1)&511)+1;upload_index=0;return;
 }
 x&=~15u;unsigned w=((packet[2]&1023)+15)&~15u,h=(packet[2]>>16)&511;
 const uint32_t c=packet[0];const uint16_t pixel=uint16_t(((c>>3)&31)|(((c>>11)&31)<<5)|(((c>>19)&31)<<10));
 for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++)vram[((y+j)&511)*1024+((x+i)&1023)]=pixel;
}
unsigned install_gpu_demo(Bus& bus){
 bus.gpu.reset();unsigned n=0;
 auto emit=[&](uint32_t w){bus.write32(n*4,w);n++;};
 emit(0x3c081f80);emit(0x35081810); // t0 = GP0
 auto send=[&](uint32_t w){emit(0x3c090000|(w>>16));emit(0x35290000|(w&65535));emit(0xad090000);};
 auto fill=[&](uint32_t color,unsigned x,unsigned y,unsigned w,unsigned h){send(0x02000000|color);send((y<<16)|x);send((h<<16)|w);};
 fill(0x201008,0,0,320,240);
 fill(0x4040f8,32,32,80,160);fill(0x40e040,128,32,64,160);fill(0xf08030,208,32,80,160);
 fill(0xffffff,32,208,256,8);
 return n;
}
unsigned install_polygon_demo(Bus& bus){
 bus.gpu.reset();unsigned n=0;
 auto emit=[&](uint32_t w){bus.write32(n++*4,w);};
 emit(0x3c081f80);emit(0x35081810);
 auto send=[&](uint32_t w){emit(0x3c090000|(w>>16));emit(0x35290000|(w&65535));emit(0xad090000);};
 auto xy=[](unsigned x,unsigned y){return(y<<16)|x;};
 send(0x02201008);send(0);send(xy(320,240));
 send(0xe3000000);send(0xe4000000|319|(239<<10));send(0xe5000000);send(0xe6000000);
 send(0x200040ff);send(xy(160,20));send(xy(30,190));send(xy(290,190));
 send(0x2000e040);send(xy(160,55));send(xy(70,175));send(xy(250,175));
 send(0x28ff8030);send(xy(120,105));send(xy(200,105));send(xy(120,155));send(xy(200,155));
 return n;
}
}
