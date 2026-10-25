#include "core.hpp"
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
int main(int argc,char** argv){
 try{
  ps1::Bus bus;ps1::Cpu cpu(bus);
  if(argc==1||(argc==2&&std::string(argv[1])=="--demo")){
   ps1::install_demo(bus);cpu.reset(0);for(int i=0;i<4;i++)cpu.step();std::cout<<"MIPS demo: 7 + 5 = "<<bus.read32(256)<<"\n";return 0;
  }
  std::string bios,exe,disc;uint64_t steps=1000000;
  for(int i=1;i<argc;i++){
   const std::string arg=argv[i];if(i+1>=argc)throw std::runtime_error("Missing option value");
   const std::string value=argv[++i];
   if(arg=="--bios")bios=value;else if(arg=="--exe")exe=value;else if(arg=="--disc")disc=value;
   else if(arg=="--steps"){size_t used=0;steps=std::stoull(value,&used);if(used!=value.size()||steps>200000000)throw std::runtime_error("Step limit is 200000000");}
   else throw std::runtime_error("Unknown CLI option");
  }
  if((bios.empty()&&exe.empty()&&disc.empty())||(!exe.empty()&&(!bios.empty()||!disc.empty())))throw std::runtime_error("Choose EXE, disc direct boot, or BIOS with optional disc");
  if(!bios.empty()){bus.load_bios(bios);cpu.reset();}
  if(!disc.empty()){bus.cd.mount(disc);ps1::boot_disc(bus,cpu,bios.empty());}
  if(!exe.empty()){
   std::ifstream file(exe,std::ios::binary|std::ios::ate);if(!file||file.tellg()>4*1024*1024)throw std::runtime_error("EXE unavailable or exceeds 4 MiB");
   file.seekg(0);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});ps1::load_exe(bus,cpu,bytes);
  }
  bus.sio[0].connected=true;
  for(uint64_t i=0;i<steps;i++)cpu.step();
  std::cout<<cpu.diagnostic();return 0;
 }catch(const std::exception& e){std::cerr<<"Stopped: "<<e.what()<<"\nUsage: ps1_lab [--demo | --exe file --steps N | --disc file.cue --steps N | --bios bios.bin [--disc file.cue] --steps N]\n";return 1;}
}
