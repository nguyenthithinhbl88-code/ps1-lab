#pragma once
#include <array>
#include <vector>
#include <cstdint>
namespace ps1 {
struct Spu {
 std::array<uint16_t,256> regs{};
 std::vector<uint16_t> ram=std::vector<uint16_t>(256*1024);
 std::vector<uint16_t> fifo;
 uint32_t cursor=0;
 uint64_t writes=0;
 uint16_t read(unsigned offset)const;
 void write(unsigned offset,uint16_t value);
 void flush();
 uint32_t dma_read();
 void dma_write(uint32_t value);
};
}
