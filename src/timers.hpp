#pragma once
#include <cstdint>
namespace ps1 {
struct Timer {
 uint16_t count=0,mode=0x400,target=0;
 unsigned divider=0;
 bool fired=false;
 uint16_t read(unsigned reg);
 void write(unsigned reg,uint16_t value);
 bool tick(unsigned index);
};
}
