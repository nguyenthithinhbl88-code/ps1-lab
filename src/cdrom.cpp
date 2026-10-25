#include "cdrom.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
namespace ps1 {
namespace {
uint8_t bcd(unsigned v){return uint8_t((v/10)*16+v%10);}
unsigned dec(uint8_t v){return(v>>4)*10+(v&15);}
bool valid_bcd(uint8_t v){return(v&15)<10&&(v>>4)<10;}
std::array<uint8_t,3> msf(unsigned frame){return {bcd(frame/(60*75)),bcd((frame/75)%60),bcd(frame%75)};}
}
void Cdrom::reset_controller(){
 lba=last_lba=0;index=enable=flag=mode=filter_file=filter_channel=0;reading=requested=muted=header_valid=false;motor=true;
 params.clear();response.clear();data.clear();events.clear();pending.clear();commands=sectors_read=0;last_command=last_error=0;last_header.fill(0);volume.fill(0);
}
void Cdrom::eject(){disc=DiscImage{};sectors=0;boot_name.clear();detected_region=4;reset_controller();}
void Cdrom::mount(const std::filesystem::path& path){
 Cdrom fresh;fresh.disc.open(path);fresh.sectors=fresh.disc.sectors;
 // GetID region is emulated metadata, not a claim that a BIN contains SCEx wobble.
 try{
  fresh.boot_name=fresh.boot_path();auto name=fresh.boot_name;
  for(char& c:name)c=char(std::toupper(static_cast<unsigned char>(c)));
  const auto slash=name.find_last_of("/\\:");if(slash!=std::string::npos)name.erase(0,slash+1);
  if(name.rfind("SLUS",0)==0||name.rfind("SCUS",0)==0)fresh.detected_region=1;
  else if(name.rfind("SLES",0)==0||name.rfind("SCES",0)==0||name.rfind("SLED",0)==0||name.rfind("SCED",0)==0)fresh.detected_region=2;
  else if(name.rfind("SLPS",0)==0||name.rfind("SLPM",0)==0||name.rfind("SCPS",0)==0)fresh.detected_region=3;
 }catch(const std::exception&){} // Mounting a valid raw disc does not require ISO files.
 *this=std::move(fresh);
}
std::vector<uint8_t> Cdrom::sector(uint32_t n){return disc.user(n);}
void Cdrom::reply(uint8_t irq,std::vector<uint8_t> bytes){if(events.size()>=8)throw std::runtime_error("CD response queue full");events.push_back({irq,std::move(bytes)});}
void Cdrom::tick(){
 if(flag||!response.empty())return;
 if(!events.empty()){flag=events.front().irq;for(auto b:events.front().bytes)response.push_back(b);events.pop_front();return;}
 if(!pending.empty()){
  auto command=std::move(pending.front());pending.pop_front();execute(command.command,command.params);return;
 }
 if(reading&&data.empty()){
  try{
   if(disc.track(lba).mode==DiscImage::Mode::Audio)throw std::runtime_error("CDDA playback not implemented");
   const auto raw=disc.raw(lba);last_lba=lba++;std::copy_n(raw.begin()+12,8,last_header.begin());header_valid=true;
   std::vector<uint8_t> bytes;
   if(mode&32)bytes.assign(raw.begin()+12,raw.end());else bytes=sector(last_lba);
   for(auto b:bytes)data.push_back(b);
   sectors_read++;flag=1;response.push_back(status());
  }catch(const std::exception&){reading=false;last_error=0x80;flag=5;response.push_back(status()|1);response.push_back(last_error);}
 }
}
uint8_t Cdrom::read(unsigned p){
 if(p==0)return uint8_t(index|(params.empty()?8:0)|(params.size()<16?16:0)|(!response.empty()?32:0)|((requested&&!data.empty())?64:0));
 if(p==1){if(response.empty())return 0;auto v=response.front();response.pop_front();return v;}
 if(p==2){if(!requested||data.empty())return 0;auto v=data.front();data.pop_front();return v;}
 return uint8_t(0xe0|((index&1)?flag:enable));
}
void Cdrom::write(unsigned p,uint8_t v){
 if(p==0){index=v&3;return;}
 if(p==1&&index==0){command(v);return;}
 if(p==2&&index==0){if(params.size()>=16)throw std::runtime_error("CD parameter FIFO full");params.push_back(v);return;}
 if(p==2&&index==1){enable=v&31;return;}
 if(p==3&&index==0){requested=v&0x80;if(!requested)data.clear();return;}
 if(p==3&&index==1){flag&=uint8_t(~(v&31));if(v&0x40)params.clear();return;}
 if(index==2&&(p==2||p==3)){volume[p-2]=v;return;}
 if(index==3&&(p==1||p==2)){volume[p+1]=v;return;}
 if(index==3&&p==3){if(v&1)muted=true;return;}
 // Sound map writes are latched, but no XA/CDDA audio is emitted.
 if(index==1&&p==1)return;
 throw std::runtime_error("Unsupported CD register bank/write");
}
void Cdrom::command(uint8_t v){
 auto p=params;params.clear();
 if(flag||!events.empty()||!response.empty()||!pending.empty()){
  if(pending.size()<4)pending.push_back({v,std::move(p)});else last_error=0x20;
  return;
 }
 execute(v,p);
}
void Cdrom::execute(uint8_t v,const std::deque<uint8_t>& p){
 commands++;last_command=v;last_error=0;
 auto error=[&](uint8_t code){last_error=code;reply(5,{uint8_t(status()|1),code});};
 auto ack=[&](){reply(3,{status()});};
 auto count=[&](unsigned n){if(p.size()==n)return true;error(0x20);return false;};
 auto media=[&](){if(sectors)return true;error(0x80);return false;};
 switch(v){
 case 1:if(count(0))ack();break;
 case 2:{
  if(!count(3))break;
  if(!valid_bcd(p[0])||!valid_bcd(p[1])||!valid_bcd(p[2])||dec(p[1])>=60||dec(p[2])>=75){error(0x10);break;}
  const int address=int((dec(p[0])*60+dec(p[1]))*75+dec(p[2]))-150;
  if(address<0||uint32_t(address)>=sectors){error(0x10);break;}lba=uint32_t(address);ack();break;}
 case 3:error(0x40);break; // CDDA Play requires a mixer/drive playback implementation.
 case 6:case 0x1b:if(count(0)&&media()){motor=true;reading=true;data.clear();ack();}break;
 case 7:case 8:case 9:case 0xa:
  if(!count(0))break;
  reading=false;data.clear();motor=v!=8;
  if(v==0xa){mode=filter_file=filter_channel=0;header_valid=false;}
  ack();reply(2,{status()});break;
 case 0xb:case 0xc:if(count(0)){muted=v==0xb;ack();}break;
 case 0xd:if(count(2)){filter_file=p[0];filter_channel=p[1];ack();}break;
 case 0xe:if(count(1)){if(p[0]&16){error(0x10);break;}mode=p[0];ack();}break;
 case 0xf:if(count(0))reply(3,{status(),mode,0,filter_file,filter_channel});break;
 case 0x10:if(count(0)){if(!header_valid){error(0x80);break;}reply(3,std::vector<uint8_t>(last_header.begin(),last_header.end()));}break;
 case 0x11:if(count(0)&&media()){
  const auto& t=disc.track(header_valid?last_lba:lba);const auto at=header_valid?last_lba:lba;
  const auto relative=msf(at>=t.start?at-t.start:t.start-at),absolute=msf(at+150);
  reply(3,{bcd(t.number),uint8_t(at>=t.start?1:0),relative[0],relative[1],relative[2],absolute[0],absolute[1],absolute[2]});
 }break;
 case 0x12:if(count(1)&&media()){if(p[0]!=1){error(0x10);break;}ack();reply(2,{status()});}break;
 case 0x13:if(count(0)&&media())reply(3,{status(),bcd(disc.tracks.front().number),bcd(disc.tracks.back().number)});break;
 case 0x14:if(count(1)&&media()){
  if(!valid_bcd(p[0])||dec(p[0])>disc.tracks.size()){error(0x10);break;}
  const auto n=dec(p[0]);const auto at=msf((n?disc.tracks[n-1].start:sectors)+150);reply(3,{status(),at[0],at[1]});
 }break;
 case 0x15:case 0x16:if(count(0)&&media()){reading=false;data.clear();ack();reply(2,{status()});}break;
 case 0x19:if(count(1)){if(p[0]==0x20)reply(3,{0x95,0x07,0x24,0xc1});else error(0x10);}break;
 case 0x1a:if(count(0)){
  ack();const auto r=effective_region();
  if(sectors&&r>=1&&r<=3)reply(2,{status(),0,0x20,0,'S','C','E',uint8_t(r==1?'A':r==2?'E':'I')});
  else reply(5,{uint8_t(status()|8),sectors?uint8_t(0x80):uint8_t(0x40),0,0,0,0,0,0});
 }break;
 case 0x1e:if(count(0)&&media()){ack();reply(2,{status()});}break;
 default:error(0x40);break;
 }
}
}
