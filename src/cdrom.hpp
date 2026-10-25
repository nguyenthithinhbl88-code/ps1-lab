#pragma once
#include "disc_image.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <vector>
#include <string>
namespace ps1 {
struct Cdrom {
 DiscImage disc;
 uint32_t sectors=0,lba=0,last_lba=0;
 uint8_t index=0,enable=0,flag=0,mode=0,filter_file=0,filter_channel=0;
 bool reading=false,requested=false,motor=true,muted=false,header_valid=false;
 std::array<uint8_t,8> last_header{};
 std::array<uint8_t,4> volume{};
 // 0=auto from BOOT executable prefix, 1=USA, 2=Europe, 3=Japan, 4=unlicensed.
 unsigned region=0,detected_region=4;
 std::string boot_name;
 std::deque<uint8_t> params,response,data;
 struct Event{uint8_t irq;std::vector<uint8_t> bytes;};
 struct Pending{uint8_t command;std::deque<uint8_t> params;};
 std::deque<Event> events;
 std::deque<Pending> pending;
 uint64_t commands=0,sectors_read=0;
 uint8_t last_command=0,last_error=0;
 void mount(const std::filesystem::path& path);
 void eject();
 void reset_controller();
 std::vector<uint8_t> sector(uint32_t number);
 uint8_t read(unsigned port);
 void write(unsigned port,uint8_t value);
 void command(uint8_t value);
 void execute(uint8_t value,const std::deque<uint8_t>& parameters);
 void tick();
 void reply(uint8_t irq,std::vector<uint8_t> bytes);
 bool irq()const{return (flag&enable&31)!=0;}
 uint8_t status()const{return sectors?uint8_t((motor?2:0)|(reading?32:0)):0x10;}
 std::vector<uint8_t> read_file(const std::string& path,unsigned limit=4*1024*1024);
 std::string boot_path();
 std::vector<uint8_t> boot_exe();
 unsigned effective_region()const{return region?region:detected_region;}
};
}
