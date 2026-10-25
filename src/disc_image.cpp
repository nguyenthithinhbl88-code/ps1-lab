#include "disc_image.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
namespace ps1 {
namespace {
std::string upper(std::string s){for(char& c:s)c=char(std::toupper(static_cast<unsigned char>(c)));return s;}
unsigned msf(const std::string& text){
 std::istringstream in(text);unsigned m=0,s=0,f=0;char a=0,b=0,extra=0;
 if(!(in>>m>>a>>s>>b>>f)||a!=':'||b!=':'||m>99||s>59||f>74||(in>>extra))throw std::runtime_error("Invalid CUE MSF timestamp");
 return (m*60+s)*75+f;
}
uint8_t bcd(unsigned v){return uint8_t((v/10)*16+v%10);}
}
void DiscImage::open(const std::filesystem::path& filename){
 DiscImage d;d.path=filename;
 auto add_file=[&](const std::filesystem::path& p){
  if(d.files.size()>=99)throw std::runtime_error("CUE exceeds 99 track files");
  File f;f.path=p;f.stream.open(p,std::ios::binary|std::ios::ate);
  if(!f.stream||f.stream.tellg()<=0)throw std::runtime_error("Disc track file missing or empty: "+p.u8string());
  f.bytes=uint64_t(f.stream.tellg());if(f.bytes>uint64_t(450000)*2352)throw std::runtime_error("Disc file exceeds supported CD size");
  d.files.push_back(std::move(f));
 };
 const bool cue=upper(filename.extension().u8string())==".CUE";
 if(cue){
  std::ifstream input(filename);if(!input)throw std::runtime_error("Cannot open CUE");
  input.seekg(0,std::ios::end);if(input.tellg()>1024*1024)throw std::runtime_error("CUE exceeds 1 MiB");input.seekg(0);
  std::string line;unsigned line_number=0;
  while(std::getline(input,line)){
   if(++line_number==1&&line.compare(0,3,"\xef\xbb\xbf")==0)line.erase(0,3);
   std::istringstream fields(line);std::string key;fields>>key;key=upper(key);if(key.empty())continue;
   if(key=="FILE"){
    std::string rest;std::getline(fields,rest);const auto start=rest.find_first_not_of(" \t");if(start==std::string::npos)throw std::runtime_error("CUE FILE needs filename");rest.erase(0,start);
    std::string name,type;
    if(rest[0]=='\"'){const auto end=rest.find('\"',1);if(end==std::string::npos)throw std::runtime_error("CUE FILE quote missing");name=rest.substr(1,end-1);std::istringstream(rest.substr(end+1))>>type;}
    else {std::istringstream r(rest);r>>name>>type;}
    if(upper(type)!="BINARY")throw std::runtime_error("CUE supports BINARY files only (no WAV/MP3/ECM)");
    std::replace(name.begin(),name.end(),'\\','/');add_file(filename.parent_path()/std::filesystem::u8path(name));
   }else if(key=="TRACK"){
    Track t;std::string mode;fields>>t.number>>mode;mode=upper(mode);
    if(d.files.empty()||t.number!=d.tracks.size()+1||t.number>99)throw std::runtime_error("CUE tracks must be numbered 01..99 in order");
    t.file=unsigned(d.files.size()-1);
    if(mode=="MODE1/2048"){t.mode=Mode::Mode1;t.stride=2048;}
    else if(mode=="MODE1/2352"){t.mode=Mode::Mode1;t.stride=2352;}
    else if(mode=="MODE2/2352"){t.mode=Mode::Mode2;t.stride=2352;}
    else if(mode=="MODE2/2336"){t.mode=Mode::Mode2;t.stride=2336;}
    else if(mode=="MODE2/2048"){t.mode=Mode::Mode2;t.stride=2048;}
    else if(mode=="AUDIO"){t.mode=Mode::Audio;t.stride=2352;}
    else throw std::runtime_error("Unsupported CUE track mode: "+mode);
    d.tracks.push_back(t);
   }else if(key=="INDEX"||key=="PREGAP"){
    if(d.tracks.empty()||d.tracks.back().file!=d.files.size()-1)throw std::runtime_error("CUE INDEX/PREGAP requires current track");
    auto& t=d.tracks.back();std::string stamp;
    if(key=="PREGAP"){fields>>stamp;if(t.pregap)throw std::runtime_error("Duplicate CUE PREGAP");t.pregap=msf(stamp);}
    else {unsigned index=99;fields>>index>>stamp;const auto frame=msf(stamp);
     if(index==0){if(t.has0||t.has1)throw std::runtime_error("Invalid CUE INDEX00 order");t.index0=frame;t.has0=true;}
     else if(index==1){if(t.has1)throw std::runtime_error("Duplicate INDEX01");t.index1=frame;t.has1=true;}
     else throw std::runtime_error("Only CUE INDEX00/01 supported");
    }
   }else if(key!="REM"&&key!="TITLE"&&key!="PERFORMER"&&key!="SONGWRITER"&&key!="CATALOG"&&key!="ISRC"&&key!="FLAGS"&&key!="CDTEXTFILE")throw std::runtime_error("Unsupported CUE directive: "+key);
  }
 }else{
  add_file(filename);auto& f=d.files[0];Track t;t.number=1;t.file=0;t.has1=true;t.mode=Mode::Mode1;t.stride=2048;
  std::array<uint8_t,16> header{};f.stream.seekg(0);f.stream.read(reinterpret_cast<char*>(header.data()),header.size());
  bool sync=header[0]==0&&header[11]==0;for(unsigned i=1;i<11;i++)sync=sync&&header[i]==255;
  if(sync&&f.bytes%2352==0&&(header[15]==1||header[15]==2)){t.stride=2352;t.mode=header[15]==1?Mode::Mode1:Mode::Mode2;}
  else if(upper(filename.extension().u8string())!=".ISO")throw std::runtime_error("BIN needs raw Mode1/2 2352 sectors; use CUE for track layout");
  d.tracks.push_back(t);
 }
 if(d.tracks.empty())throw std::runtime_error("CUE has no tracks");
 uint64_t disc_end=0;
 for(size_t i=0;i<d.tracks.size();i++){
  auto& t=d.tracks[i];const auto& f=d.files[t.file];
  if(!t.has1||f.bytes%t.stride||(t.has0&&t.index0>t.index1))throw std::runtime_error("CUE INDEX01 or sector alignment invalid");
  if(i&&t.file==d.tracks[i-1].file&&t.stride!=d.tracks[i-1].stride)throw std::runtime_error("Mixed sector sizes within a CUE file unsupported");
  uint64_t file_end=f.bytes/t.stride;
  if(i+1<d.tracks.size()&&d.tracks[i+1].file==t.file){const auto& next=d.tracks[i+1];file_end=next.has0?next.index0:next.index1;}
  if(t.index1>=file_end||file_end>f.bytes/t.stride)throw std::runtime_error("CUE index outside track file or empty track");
  t.gap_start=uint32_t(disc_end);
  const uint64_t gap=i?(t.pregap+(t.has0?t.index1-t.index0:0)):0;
  t.start=uint32_t(disc_end+gap);disc_end=disc_end+gap+file_end-t.index1;
  if(disc_end>450000)throw std::runtime_error("CUE exceeds supported CD length");
  t.end=uint32_t(disc_end);
 }
 d.sectors=uint32_t(disc_end);
 if(!cue&&d.tracks[0].stride==2048){
  if(d.sectors<17)throw std::runtime_error("ISO too small");
  auto p=d.user(16);
  if(std::string(p.begin()+1,p.begin()+6)!="CD001")throw std::runtime_error("ISO9660 volume descriptor missing");
 }
 *this=std::move(d);
}
const DiscImage::Track& DiscImage::track(uint32_t lba)const{
 if(lba>=sectors)throw std::runtime_error("CD sector outside image");
 for(const auto& t:tracks)if(lba>=t.gap_start&&lba<t.end)return t;
 throw std::runtime_error("CD sector not mapped");
}
std::array<uint8_t,2352> DiscImage::raw(uint32_t lba){
 const auto& t=track(lba);std::array<uint8_t,2352> out{};
 const auto stamp=lba+150;out[12]=bcd(stamp/(60*75));out[13]=bcd((stamp/75)%60);out[14]=bcd(stamp%75);out[15]=t.mode==Mode::Mode1?1:2;
 std::fill(out.begin()+1,out.begin()+11,255);
 uint32_t position=0;
 if(lba<t.start){
  const unsigned stored=t.has0?t.index1-t.index0:0;
  if(!stored||lba<t.start-stored){if(t.mode==Mode::Audio)out.fill(0);return out;}
  position=t.index0+lba-(t.start-stored);
 }else position=t.index1+lba-t.start;
 auto& f=files[t.file];f.stream.clear();f.stream.seekg(uint64_t(position)*t.stride);
 unsigned offset=0;if(t.stride==2336)offset=16;else if(t.stride==2048)offset=t.mode==Mode::Mode1?16:24;
 if(!f.stream.read(reinterpret_cast<char*>(out.data()+offset),t.stride))throw std::runtime_error("CD track read failed");
 if(t.stride==2352&&t.mode!=Mode::Audio){
  bool valid=out[0]==0&&out[11]==0&&out[15]==(t.mode==Mode::Mode1?1:2);
  for(unsigned i=1;i<11;i++)valid=valid&&out[i]==255;
  if(!valid)throw std::runtime_error("Raw data-sector sync/mode does not match CUE");
 }
 return out;
}
std::vector<uint8_t> DiscImage::user(uint32_t lba){
 const auto& t=track(lba);if(t.mode==Mode::Audio||lba<t.start)throw std::runtime_error("No ISO user data in audio/pregap sector");
 const auto bytes=raw(lba);unsigned offset=16;
 if(t.mode==Mode::Mode2){if(bytes[18]&32)throw std::runtime_error("Mode2 Form2 is not a 2048-byte data sector");offset=24;}
 return std::vector<uint8_t>(bytes.begin()+offset,bytes.begin()+offset+2048);
}
std::string DiscImage::description()const{
 std::ostringstream out;out<<tracks.size()<<" tracks, "<<sectors<<" sectors";
 for(const auto& t:tracks)out<<" | T"<<t.number<<" "<<(t.mode==Mode::Audio?"AUDIO":t.mode==Mode::Mode1?"MODE1":"MODE2")<<"/"<<t.stride<<" LBA "<<t.start<<".."<<t.end-1;
 return out.str();
}
}
