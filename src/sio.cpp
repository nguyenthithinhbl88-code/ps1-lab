#include "sio.hpp"
namespace ps1 {
uint8_t Sio::read(unsigned o){
 if(o==0){rx=false;return data;}
 uint32_t status=5|(rx?2:0)|(irq?512:0)|(ack?128:0);
 if(o>=4&&o<8)return uint8_t(status>>((o-4)*8));
 if(o==8||o==9)return uint8_t(mode>>((o-8)*8));
 if(o==10||o==11)return uint8_t(control>>((o-10)*8));
 if(o==14||o==15)return uint8_t(baud>>((o-14)*8));
 return 0;
}
void Sio::write(unsigned o,uint8_t v){
 auto put=[&](uint16_t& r,unsigned lane){r=uint16_t((r&~(255u<<(lane*8)))|(uint16_t(v)<<(lane*8)));};
 if(o==0){
  if(!(control&1))return;
  ack=false;data=255;
  if(connected&&(control&2)&&!(control&0x2000)){
   switch(phase){
    case 0:if(v==1){phase=1;ack=true;}break;
    case 1:if(v==0x42){data=0x41;phase=2;ack=true;latched_buttons=buttons;}else phase=0;break;
    case 2:data=0x5a;phase=3;ack=true;break;
    case 3:data=uint8_t(latched_buttons);phase=4;ack=true;break;
    case 4:data=uint8_t(latched_buttons>>8);phase=0;break;
    default:phase=0;break;
   }
  }else phase=0;
  if(control&6)rx=true;
  if((control&0x400)||((control&0x800)&&rx)||((control&0x1000)&&ack))irq=true;
  return;
 }
 if(o==8||o==9){put(mode,o-8);mode&=0x1ff;return;}
 if(o==14||o==15){put(baud,o-14);return;}
 if(o==10||o==11){put(control,o-10);if(control&0x40){mode=control=baud=0;rx=irq=ack=false;phase=0;return;}if(control&0x10)irq=false;if(!(control&2)){phase=0;ack=false;}control&=0x3faf;}
}
}
