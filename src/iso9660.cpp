#include "cdrom.hpp"
#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>
namespace ps1 {
namespace {
uint32_t le(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
std::string upper(std::string s){for(char& c:s)c=char(std::toupper(static_cast<unsigned char>(c)));return s;}
std::string trim(std::string s){const auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos)return {};return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
struct Entry{uint32_t start=0,size=0;bool directory=false;};
std::string key(std::string s){s=upper(s);const auto semicolon=s.find(';');if(semicolon!=std::string::npos)s.resize(semicolon);return s;}
}
std::vector<uint8_t> Cdrom::read_file(const std::string& filename,unsigned limit){
 if(!sectors)throw std::runtime_error("No disc mounted");
 uint32_t base=0;bool found_data=false;
 for(const auto& t:disc.tracks)if(t.mode!=DiscImage::Mode::Audio){base=t.start;found_data=true;break;}
 if(!found_data)throw std::runtime_error("Audio disc has no ISO9660 filesystem");
 auto fetch=[&](uint32_t start,uint32_t length,unsigned cap){
  if(length>cap||uint64_t(base)+start+(uint64_t(length)+2047)/2048>sectors)throw std::runtime_error("ISO extent exceeds file/directory/image bounds");
  std::vector<uint8_t> bytes;bytes.reserve(length);
  for(uint32_t i=0;i<(uint64_t(length)+2047)/2048;i++){auto block=sector(base+start+i);const auto n=std::min<size_t>(block.size(),length-bytes.size());bytes.insert(bytes.end(),block.begin(),block.begin()+n);}
  return bytes;
 };
 std::vector<uint8_t> volume;
 for(unsigned n=16;n<32&&base+n<sectors;n++){
  auto p=sector(base+n);if(std::string(p.begin()+1,p.begin()+6)!="CD001"||p[6]!=1)throw std::runtime_error("ISO9660 descriptor invalid");
  if(p[0]==1){volume=std::move(p);break;}if(p[0]==255)break;
 }
 if(volume.empty()||volume[128]!=0||volume[129]!=8||volume[156]<34)throw std::runtime_error("Need primary ISO9660 volume with 2048-byte blocks");
 Entry current{le(&volume[158]),le(&volume[166]),true};
 std::string path=filename;std::replace(path.begin(),path.end(),'\\','/');const auto colon=path.find(':');
 if(colon!=std::string::npos){if(upper(path.substr(0,colon))!="CDROM"&&upper(path.substr(0,colon))!="CDROM0")throw std::runtime_error("Only cdrom: boot paths supported");path.erase(0,colon+1);}
 std::istringstream parts(path);std::string component;unsigned depth=0;
 while(std::getline(parts,component,'/')){
  if(component.empty())continue;
  if(component=="."||component==".."||++depth>16)throw std::runtime_error("Invalid ISO path depth/component");
  if(!current.directory)throw std::runtime_error("ISO path enters a regular file");
  auto directory=fetch(current.start,current.size,1024*1024);std::optional<Entry> next;
  for(size_t i=0;i<directory.size();){
   const unsigned len=directory[i];if(!len){i=(i/2048+1)*2048;continue;}
   if(len<34||i+len>directory.size()||i%2048+len>2048||33u+directory[i+32]>len)throw std::runtime_error("Malformed ISO directory record");
   const std::string name(reinterpret_cast<char*>(directory.data()+i+33),directory[i+32]);
   if(key(name)==key(component)){
    if(directory[i+25]&0x80)throw std::runtime_error("ISO multi-extent files unsupported");
    next=Entry{le(directory.data()+i+2),le(directory.data()+i+10),(directory[i+25]&2)!=0};break;
   }
   i+=len;
  }
  if(!next)throw std::runtime_error("ISO file not found: "+component);
  current=*next;
 }
 if(!depth||current.directory)throw std::runtime_error("ISO path does not identify a file");
 return fetch(current.start,current.size,limit);
}
std::string Cdrom::boot_path(){
 std::vector<uint8_t> config;
 try{config=read_file("SYSTEM.CNF;1",64*1024);}
 catch(const std::runtime_error& e){if(std::string(e.what()).find("ISO file not found: SYSTEM.CNF")!=0)throw;return "PSX.EXE;1";}
 std::istringstream input(std::string(config.begin(),config.end()));std::string line;
 while(std::getline(input,line)){
  const auto eq=line.find('=');if(eq==std::string::npos||upper(trim(line.substr(0,eq)))!="BOOT")continue;
  auto value=trim(line.substr(eq+1));if(value.empty())break;
  if(value.front()=='\"'){const auto end=value.find('\"',1);if(end==std::string::npos)throw std::runtime_error("SYSTEM.CNF BOOT quote missing");return value.substr(1,end-1);}
  const auto end=value.find_first_of(" \t\r\n\0");return value.substr(0,end);
 }
 throw std::runtime_error("SYSTEM.CNF does not specify BOOT");
}
std::vector<uint8_t> Cdrom::boot_exe(){
 auto path=boot_path();auto bytes=read_file(path);boot_name=path;return bytes;
}
}
