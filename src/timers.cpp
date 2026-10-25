#include "timers.hpp"
namespace ps1 {
uint16_t Timer::read(unsigned reg){
 if(reg==0)return count;
 if(reg==8)return target;
 uint16_t value=mode;mode&=~0x1800;return value;
}
void Timer::write(unsigned reg,uint16_t value){
 if(reg==0){count=value;return;}
 if(reg==8){target=value;return;}
 mode=(value&0x3ff)|0x400;count=0;divider=0;fired=false;
}
bool Timer::tick(unsigned index){
 // No video clock/sync source yet: those modes wait instead of using a fake clock.
 if(index<2&&((mode&0x100)||(mode&1)))return false;
 if(index==2&&(mode&1)&&(((mode>>1)&3)==0||((mode>>1)&3)==3))return false;
 if(index==2&&(mode&0x200)){if(++divider<8)return false;divider=0;}
 count=uint16_t(count+1);
 bool match=count==target,overflow=count==0xffff;
 if(match)mode|=0x800;
 if(overflow)mode|=0x1000;
 if(match&&(mode&8))count=0;
 bool event=(match&&(mode&0x10))||(overflow&&(mode&0x20));
 if(!event||(!(mode&0x40)&&fired))return false;
 fired=true;
 if(mode&0x80){mode^=0x400;return !(mode&0x400);}
 // Pulse represented as a latched interrupt-controller event; no cycle-level pulse.
 mode|=0x400;return true;
}
}
