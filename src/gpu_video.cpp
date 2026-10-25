#include "gpu.hpp"
#include <algorithm>
namespace ps1 {
void Gpu::update_video_status(){
 const unsigned total=(display[3]&8)?314:263;
 const unsigned start=display[2]?std::min(display[2]&1023,total):16;
 const unsigned end=display[2]?std::min((display[2]>>10)&1023,total):256;
 video_vblank=end<=start||video_line<start||video_line>=end;
 const bool interlaced=display[3]&32;
 // GPUSTAT.13 is the field indicator, always 1 without interlace.
 const bool field_indicator=!interlaced||!video_field;
 // 480i changes line parity per field; 240-line modes change it per scanline.
 const bool odd=!video_vblank&&(((display[3]&36)==36)?video_field:bool(video_line&1));
 status=(status&~0x80002000u)|(field_indicator?0x2000u:0)|(odd?0x80000000u:0);
}
bool Gpu::tick_video(){
 // Nominal 11/7 GPU clocks per approximate CPU tick. Reads never advance time.
 video_fraction+=11;video_dot+=video_fraction/7;video_fraction%=7;
 const unsigned clocks=(display[3]&8)?3406:3413;
 if(video_dot<clocks)return false;
 video_dot-=clocks;
 const bool was_blank=video_vblank;
 if(++video_line>=((display[3]&8)?314u:263u)){
  video_line=0;video_frames++;video_field=!video_field;
 }
 update_video_status();
 const bool entered_blank=!was_blank&&video_vblank;
 if(entered_blank)vblank_events++;
 return entered_blank;
}
}
