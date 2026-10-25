#include "core.hpp"
namespace ps1 {
void Bus::append_tty(char c){if(tty.size()>=8192)tty.erase(0,4096);tty+=c;}
// Register latches only. They do not emulate waitstates, remapping or cache tags.
bool Bus::boot_io_read(uint32_t address,uint8_t& value)const{
 if(address>=0xfffe0130&&address<0xfffe0134){value=uint8_t(cache_control>>((address&3)*8));return true;}
 const uint32_t a=address&0x1fffffff;
 if(a>=0x1f000000&&a<0x1f000100){value=0xff;return true;} // absent expansion header
 if(a>=0x1f801000&&a<0x1f801024){value=uint8_t(memory_control[(a-0x1f801000)/4]>>((a&3)*8));return true;}
 if(a>=0x1f801060&&a<0x1f801064){value=uint8_t(ram_control>>((a&3)*8));return true;}
 if(a>=0x1f801070&&a<0x1f801074){value=a<0x1f801072?uint8_t(irq_status>>((a&1)*8)):0;return true;}
 if(a>=0x1f801074&&a<0x1f801078){value=a<0x1f801076?uint8_t(irq_mask>>((a&1)*8)):0;return true;}
 if(a==0x1f802041){value=post;return true;}
 return false;
}
bool Bus::boot_io_write(uint32_t address,uint8_t value){
 auto byte_write=[&](uint32_t& reg,unsigned lane){unsigned shift=lane*8;reg=(reg&~(255u<<shift))|(uint32_t(value)<<shift);};
 if(address>=0xfffe0130&&address<0xfffe0134){byte_write(cache_control,address&3);return true;}
 const uint32_t a=address&0x1fffffff;
 if(a>=0x1f801000&&a<0x1f801024){auto& reg=memory_control[(a-0x1f801000)/4];byte_write(reg,a&3);if(a<0x1f801008)reg=(reg&0xffffff)|0x1f000000;return true;}
 if(a>=0x1f801060&&a<0x1f801064){byte_write(ram_control,a&3);return true;}
 if(a>=0x1f801070&&a<0x1f801074){if(a<0x1f801072)irq_status&=uint16_t((uint16_t(value)<<((a&1)*8))|(a&1?0xff:0xff00));return true;}
 if(a>=0x1f801074&&a<0x1f801078){if(a<0x1f801076){unsigned shift=(a&1)*8;irq_mask=uint16_t((irq_mask&~(255u<<shift))|(uint16_t(value)<<shift));irq_mask&=0x7ff;}return true;}
 if(a==0x1f802041){post=value;return true;}
 return false;
}
}
