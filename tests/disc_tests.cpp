#include "core.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
namespace {
void ck(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::vector<uint8_t> bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("fixture missing");return {std::istreambuf_iterator<char>(f),{}};}
void write(const std::filesystem::path& p,const std::vector<uint8_t>& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));}
void text(const std::filesystem::path& p,const std::string& s){std::ofstream(p)<<s;}
}
int main(int argc,char** argv){try{
 ck(argc==2,"need examples directory");const auto examples=std::filesystem::path(argv[1]);
 const auto temp=std::filesystem::temp_directory_path()/"ps1_lab_disc_tests_v16";std::filesystem::create_directories(temp);
 const auto iso=bytes(examples/"catch_stars.iso"),raw=bytes(examples/"catch_stars.bin");
 ps1::DiscImage image;image.open(examples/"catch_stars.cue");ck(image.sectors==iso.size()/2048&&image.tracks[0].stride==2352,"CUE mount");
 const auto expected=std::vector<uint8_t>(iso.begin()+16*2048,iso.begin()+17*2048);
 ck(image.user(16)==expected,"Mode2/2352 user extraction");image.open(examples/"catch_stars.bin");ck(image.user(16)==expected,"standalone BIN detection");
 image.open(examples/"catch_stars.iso");ck(image.user(16)==expected,"ISO user extraction");
 // Mode1/2352 and Mode2/2336 use different user-data offsets.
 std::vector<uint8_t> mode1,mode2336;
 for(size_t i=0;i<iso.size()/2048;i++){
  std::vector<uint8_t> block(raw.begin()+i*2352,raw.begin()+(i+1)*2352);block[15]=1;
  std::copy_n(iso.begin()+i*2048,2048,block.begin()+16);mode1.insert(mode1.end(),block.begin(),block.end());
  mode2336.insert(mode2336.end(),raw.begin()+i*2352+16,raw.begin()+(i+1)*2352);
 }
 write(temp/"mode1.bin",mode1);image.open(temp/"mode1.bin");ck(image.user(16)==expected,"Mode1 raw offset");
 write(temp/"xa.bin",mode2336);text(temp/"xa.cue","FILE \"xa.bin\" BINARY\nTRACK 01 MODE2/2336\nINDEX 01 00:00:00\n");image.open(temp/"xa.cue");ck(image.user(16)==expected,"2336-byte offset");
 // INDEX00 stored pregap and PREGAP synthetic sectors are separate from data.
 std::vector<uint8_t> mixed=raw;mixed.insert(mixed.end(),2*2352,0xaa);mixed.insert(mixed.end(),5*2352,0x55);write(temp/"mixed.bin",mixed);
 text(temp/"mixed.cue","REM mixed fixture\nFILE \"mixed.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\nTRACK 02 AUDIO\nPREGAP 00:00:03\nINDEX 00 00:00:25\nINDEX 01 00:00:27\n");
 image.open(temp/"mixed.cue");ck(image.tracks[0].end==25&&image.tracks[1].start==30&&image.sectors==35,"same-file TOC/gaps");
 ck(image.raw(25)[0]==0&&image.raw(28)[0]==0xaa&&image.raw(30)[0]==0x55,"synthetic/stored/audio mapping");
 bool rejected=false;try{image.user(30);}catch(const std::exception&){rejected=true;}ck(rejected,"audio is not ISO data");
 write(temp/"data track.bin",raw);write(temp/"audio.bin",std::vector<uint8_t>(4*2352,0x66));
 text(temp/"multi.cue","FILE \"data track.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\nFILE \"audio.bin\" BINARY\nTRACK 02 AUDIO\nPREGAP 00:02:00\nINDEX 01 00:00:00\n");
 image.open(temp/"multi.cue");ck(image.tracks[1].start==175&&image.sectors==179&&image.raw(175)[0]==0x66,"multi-file CUE TOC");
 std::vector<uint8_t> leading(150*2352);leading.insert(leading.end(),raw.begin(),raw.end());write(temp/"leading.bin",leading);
 text(temp/"leading.cue","FILE \"leading.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 00 00:00:00\nINDEX 01 00:02:00\n");image.open(temp/"leading.cue");ck(image.sectors==25&&image.user(16)==expected,"first track stored lead-in excluded from LBA0");
 // Failed mount is atomic and diagnostics remain usable.
 text(temp/"bad.cue","FILE \"missing.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n");
 rejected=false;try{image.open(temp/"bad.cue");}catch(const std::exception&){rejected=true;}ck(rejected&&image.user(16)==expected,"failed CUE mount preserves image");
 const std::string invalid[]={"FILE \"data track.bin\" WAVE\n","FILE \"data track.bin\" BINARY\nTRACK 01 MODE2/2352\n","FILE \"data track.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 90:00:00\n","FILE \"data track.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\nPOSTGAP 00:02:00\n"};
 for(const auto& s:invalid){text(temp/"bad.cue",s);rejected=false;try{image.open(temp/"bad.cue");}catch(const std::exception&){rejected=true;}ck(rejected,"malformed/unsupported CUE rejected");}
 // Form2 bytes can be read raw, but must never be misidentified as Form1 ISO data.
 auto form2=raw;form2[16*2352+18]|=32;write(temp/"form2.bin",form2);image.open(temp/"form2.bin");
 rejected=false;try{image.user(16);}catch(const std::exception&){rejected=true;}ck(rejected&&image.raw(16)[18]&32,"Form2 bounds");
 ps1::Bus b;ps1::Cpu cpu(b);b.cd.mount(examples/"catch_stars.cue");
 ck(b.cd.boot_path()=="cdrom:\\GAME\\MAIN.EXE;1","SYSTEM.CNF nested BOOT");
 ck(b.cd.boot_exe()==bytes(examples/"catch_stars.exe"),"nested EXE load");
 ck(b.cd.read_file("cdrom:/game/main.exe")==bytes(examples/"catch_stars.exe"),"case/version/path normalization");
 rejected=false;try{b.cd.read_file("../MAIN.EXE");}catch(const std::exception&){rejected=true;}ck(rejected,"ISO path validation");
 b.cd.mount(examples/"homebrew.iso");ck(b.cd.boot_path()=="PSX.EXE;1"&&b.cd.boot_exe()==bytes(examples/"dma_polygon.exe"),"legacy PSX.EXE fallback");
 b.cd.mount(temp/"multi.cue");
 auto w=[&](unsigned p,uint8_t v){b.write8(0x1f801800+p,v);};auto r=[&](unsigned p){return b.read8(0x1f801800+p);};
 auto ack=[&](){while(r(0)&32)r(1);w(0,1);w(3,31);w(0,0);};
 auto wait=[&](){for(unsigned i=0;i<32;i++){b.tick_timers();if(b.cd.flag)return b.cd.flag;}throw std::runtime_error("CD IRQ timeout in test");};
 auto command=[&](uint8_t cmd,std::initializer_list<uint8_t> p){for(auto v:p)w(2,v);w(1,cmd);return wait();};
 auto response=[&](){std::vector<uint8_t> v;while(r(0)&32)v.push_back(r(1));return v;};
 w(0,1);w(2,31);w(0,0);
 ck(command(0x13,{})==3&&response()==std::vector<uint8_t>({2,1,2}),"GetTN first/last");ack();
 ck(command(0x14,{2})==3&&response()==std::vector<uint8_t>({2,0,4}),"GetTD track start MS");ack();
 ck(command(0x14,{0x99})==5,"bad track produces INT5");ack();
 ck(command(0x19,{0x20})==3&&response().size()==4,"Test drive version");ack();
 ck(command(0xd,{7,9})==3,"Setfilter");ack();ck(command(0xf,{})==3&&response()==std::vector<uint8_t>({2,0,0,7,9}),"Getparam filter");ack();
 ck(command(0xe,{0x20})==3,"raw2340 mode");ack();ck(command(2,{0,2,0x16})==3,"Setloc");ack();ck(command(6,{})==3,"ReadN");ack();
 ck(wait()==1&&b.cd.data.size()==2340&&(b.irq_status&4),"raw sector INT1/IRQ2");ack();w(3,0x80);
 b.write32(0x1f8010f0,0x8000);b.write32(0x1f8010b0,0x10000);b.write32(0x1f8010b4,585);b.write32(0x1f8010b8,0x11000000);
 for(unsigned i=0;i<2340;i++)ck(b.ram[0x10000+i]==raw[16*2352+12+i],"raw DMA3 payload");
 ck(command(9,{})==3,"Pause ack");ack();ck(wait()==2,"Pause completion");ack();
 ck(command(0x10,{})==3&&response()==std::vector<uint8_t>({0,2,0x16,2,0,0,8,0}),"GetlocL raw header");ack();
 ck(command(0x11,{})==3&&response()==std::vector<uint8_t>({1,1,0,0,0x16,0,2,0x16}),"GetlocP synthesized SubQ");ack();
 ck(command(0x15,{})==3,"SeekL ack");ack();ck(wait()==2,"SeekL complete");ack();
 ck(command(0x1e,{})==3,"ReadTOC ack");ack();ck(wait()==2,"ReadTOC complete");ack();
 ck(command(0x1a,{})==3,"GetID ack");ack();ck(wait()==5,"auto homebrew unlicensed");ack();
 b.cd.region=1;ck(command(0x1a,{})==3,"GetID override ack");ack();ck(wait()==2&&response()==std::vector<uint8_t>({2,0,0x20,0,'S','C','E','A'}),"GetID explicitly emulated region");ack();
 // Commands submitted under a pending IRQ are deferred, not host exceptions.
 ck(command(1,{})==3,"Getstat");w(1,0x13);ck(b.cd.pending.size()==1,"queued command");ack();ck(wait()==3&&response().size()==3,"queued GetTN response");ack();
 const auto n=b.cd.sectors;rejected=false;try{b.cd.mount(temp/"bad.cue");}catch(const std::exception&){rejected=true;}ck(rejected&&b.cd.sectors==n,"atomic controller mount");
 b.cd.eject();ck(b.cd.sectors==0&&b.cd.disc.tracks.empty()&&!b.cd.reading&&b.cd.pending.empty(),"eject resets drive");
 // DMA3 can start before CD data exists and span two sector-ready events.
 b.cd.mount(examples/"catch_stars.cue");
 ck(command(2,{0,2,0x16})==3,"async DMA Setloc");ack();ck(command(6,{})==3,"async DMA ReadN");
 w(3,0x80);b.write32(0x1f8010f0,0x8000);b.write32(0x1f8010f4,0x00880000);
 b.write32(0x1f8010b0,0x18000);b.write32(0x1f8010b4,1024);b.write32(0x1f8010b8,0x11000000);
 ck((b.dma[3].control&(1u<<24))&&b.dma[3].block==1024,"DMA waits for DRQ");ack();
 ck(wait()==1&&b.dma[3].block==512&&(b.dma[3].control&(1u<<24)),"first sector partial DMA");ack();
 ck(wait()==1&&!(b.dma[3].control&(1u<<24))&&(b.dicr&0x88000000u)==0x88000000u,"second sector DMA complete");
 for(unsigned i=0;i<4096;i++){ck(b.ram[0x18000+i]==iso[16*2048+i],"multi-sector DMA bytes");}ack();b.cd.eject();
 // Digital pad bytes, active-low buttons, transaction snapshot and ACK IRQ7.
 auto& pad=b.sio[0];pad.connected=true;pad.buttons=uint16_t(0xffff&~(1u<<5)&~(1u<<14));b.write16(0x1f80104a,0x1003);
 const uint8_t tx[]={1,0x42,0,0,0},rx[]={255,0x41,0x5a,0xdf,0xbf};
 for(unsigned i=0;i<5;i++){b.write8(0x1f801040,tx[i]);ck(b.read8(0x1f801040)==rx[i],"digital pad byte exchange");if(i==1)pad.buttons=0xffff;}
 b.tick_timers();ck(b.irq_status&128,"pad ACK IRQ7");ck(!pad.ack,"final pad byte has no ACK");
 b.write16(0x1f80104a,1);b.write16(0x1f80104a,3);b.write8(0x1f801040,0x81);ck(b.read8(0x1f801040)==255&&pad.phase==0,"memory card stays disconnected");
 // End-to-end original game: CUE -> SYSTEM.CNF -> CPU CD commands -> DMA3 -> GPU.
 b.cd.mount(examples/"catch_stars.cue");ps1::boot_disc(b,cpu,true);
 auto frame=[&](uint32_t target){for(unsigned i=0;i<1500000&&b.read32(0x2004)<target;i++)cpu.step();ck(b.read32(0x2004)>=target,"game frame deadline");for(unsigned i=0;i<32;i++)cpu.step();};
 frame(1);ck(b.read32(0x2000)==144&&b.cd.commands>=3&&b.cd.sectors_read>=1&&b.dma_words>=512,"game CD/DMA pipeline");ck(b.read32(0x3000)==0x30444301,"game fetched ISO PVD through MMIO DMA");
 ck(b.gpu.rgb(145,221)==0x42e742,"game paddle render");
 b.sio[0].buttons=uint16_t(0xffff&~(1u<<5));frame(2);ck(b.read32(0x2000)==148,"game right input through pad");
 b.sio[0].buttons=uint16_t(0xffff&~(1u<<7));frame(3);ck(b.read32(0x2000)==144,"game left input through pad");
 b.sio[0].buttons=0xffff;cpu.r[22]=207;cpu.r[21]=144;frame(4);ck(b.read32(0x2008)==1&&b.read32(0x200c)==16,"game catch score/respawn");
 // BIOS boot resets core while retaining disc and user-provided BIOS bytes.
 b.bios[0]=0x12;ps1::boot_disc(b,cpu,false);ck(cpu.pc==0xbfc00000&&b.bios[0]==0x12&&b.cd.sectors==25&&b.read32(0x2004)==0,"BIOS+disc restart retains media");
 ck(cpu.diagnostic().find("Disc layout:")!=std::string::npos,"CD diagnostic");
 std::filesystem::remove_all(temp);
 std::cout<<"PASS: ISO/BIN/CUE modes and multi-file/track/pregaps, ISO SYSTEM.CNF paths, CD commands/raw DMA3/IRQ queue/GetID, digital pad, playable MIPS CD game and BIOS-media restart\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
