#pragma once
#include <cstdint>
#include <vector>
#include <array>
namespace ps1 {
// Functional VRAM command subset, not a timing-accurate display controller.
struct Gpu {
 std::vector<uint16_t> vram=std::vector<uint16_t>(1024*512);
 std::array<uint32_t,12> packet{};
 unsigned count=0;
 uint64_t words=0,polygons=0,rectangles=0,line_segments=0;
 std::array<uint32_t,64> history{};
 struct Vertex {int x=0,y=0,u=0,v=0;uint32_t color=0;};
 uint16_t texel(unsigned u,unsigned v,uint16_t clut)const;
 void fragment(int x,int y,uint32_t color,uint16_t texture,bool textured,bool raw,bool semi,bool dither);
 void render_triangle(Vertex a,Vertex b,Vertex c,uint16_t clut,bool textured,bool raw,bool semi,bool dither);
 bool polyline_active=false,polyline_color_pending=false;
 unsigned polyline_opcode=0;
 uint32_t polyline_color=0;
 Vertex polyline_vertex{};
 void render_line(Vertex a,Vertex b,bool semi);
 void line_packet();
 void polyline_word(uint32_t word);
 void polygon_packet();
 void rectangle_packet();
 unsigned read_x=0,read_y=0,read_w=0,read_h=0,read_index=0;
 uint32_t read_latch=0;
 uint32_t read_gp0();
 unsigned read_remaining()const{return read_w?read_w*read_h-read_index:0;}
 uint32_t draw_mode=0,texture_window=0;
 std::array<uint32_t,4> display{};
 bool display_configured=false;
 // Approximate video timing: one core instruction tick = one CPU clock.
 unsigned video_fraction=0,video_dot=0,video_line=0;
 bool video_field=false,video_vblank=true;
 uint64_t video_frames=0,vblank_events=0;
 void update_video_status();
 bool tick_video();
 int left=0,top=0,right=0,bottom=0,offset_x=0,offset_y=0;
 bool force_mask=false,check_mask=false;
 void triangle(uint32_t color,uint32_t a,uint32_t b,uint32_t c);
 void shaded_triangle(uint32_t c0,uint32_t a,uint32_t c1,uint32_t b,uint32_t c2,uint32_t c);
 void pixel(unsigned index,uint16_t value);
 uint32_t status=0x14802000;
 unsigned upload_x=0,upload_y=0,upload_w=0,upload_h=0,upload_index=0;
 void gp0(uint32_t word);
 void gp1(uint32_t word);
 void reset();
 uint32_t rgb(unsigned x,unsigned y) const;
};
unsigned install_texture_demo(struct Bus& bus);
unsigned install_polygon_demo(struct Bus& bus);
unsigned install_gpu_demo(struct Bus& bus);
}
