#pragma once
#include <cstdint>
namespace ps1 {
// Functional digital pad/disconnected model; byte transfers are immediate.
struct Sio {
 uint16_t mode=0,control=0,baud=0;
 bool rx=false,irq=false,connected=false,ack=false;
 unsigned phase=0;
 uint16_t buttons=0xffff,latched_buttons=0xffff;
 uint8_t data=255;
 uint8_t read(unsigned offset);
 void write(unsigned offset,uint8_t value);
};
}
