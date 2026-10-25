#include "core.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iomanip>
namespace ps1 {
static std::runtime_error bus_error(const char* operation,uint32_t a){std::ostringstream s;s<<operation<<" @ 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<a;return std::runtime_error(s.str());}
static uint32_t physical(uint32_t a) { return a & 0x1fffffff; }
static bool timer_address(uint32_t a){a=physical(a);return a>=0x1f801100&&a<0x1f801130&&((a&15)==0||(a&15)==4||(a&15)==8);}
void Bus::sample_irq_lines(){
 uint16_t lines=(gpu.status&(1u<<24))?2:0;
 if(cd.irq())lines|=4;
 for(unsigned i=0;i<2;i++)if(sio[i].irq)lines|=uint16_t(1u<<(7+i));
 irq_status|=uint16_t(lines&~previous_irq_lines);previous_irq_lines=lines;
}
void Bus::tick_timers(){if(gpu.tick_video())irq_status|=1;sample_irq_lines();cd.tick();if(dma[3].control&(1u<<24))run_dma(3);sample_irq_lines();for(unsigned i=0;i<3;i++)if(timers[i].tick(i))irq_status|=uint16_t(1u<<(4+i));}

uint8_t Bus::read8(uint32_t a) const {
    if(physical(a)>=0x1f801040&&physical(a)<0x1f801060)return sio[(physical(a)-0x1f801040)/16].read(a&15);
    if(physical(a)>=0x1f801800&&physical(a)<=0x1f801803)return cd.read(physical(a)-0x1f801800);
    uint8_t v=0;if(boot_io_read(a,v))return v;
    const uint32_t original=a;a=physical(a);
    if(a<0x800000) return ram[a&0x1fffff];
    if(a>=0x1f800000 && a<0x1f800400) return scratch[a-0x1f800000];
    if(a>=0x1fc00000 && a<0x1fc80000) return bios[a-0x1fc00000];
    throw bus_error("Unsupported bus read",original);
}
uint16_t Bus::read16(uint32_t a)const{
 if(timer_address(a))return timers[(physical(a)-0x1f801100)/16].read(a&15);
 if(a&1)throw bus_error("Unaligned halfword read",a);
 uint32_t p=physical(a);if(p>=0x1f801c00&&p<0x1f801e00)return spu.read(p-0x1f801c00);
 return uint16_t(read8(a)|(uint16_t(read8(a+1))<<8));
}
void Bus::write16(uint32_t a,uint16_t v){
 if(timer_address(a)){timers[(physical(a)-0x1f801100)/16].write(a&15,v);return;}
 if(a&1)throw bus_error("Unaligned halfword write",a);
 uint32_t p=physical(a);if(p>=0x1f801c00&&p<0x1f801e00){spu.write(p-0x1f801c00,v);return;}
 write8(a,uint8_t(v));write8(a+1,uint8_t(v>>8));
}
uint32_t Bus::read32(uint32_t a) const {
    // CPU instruction fetches and ordinary RAM loads avoid four MMIO decodes.
    const uint32_t p=physical(a);
    if(!(a&3)&&p<0x800000){const auto i=p&0x1fffff;return uint32_t(ram[i])|(uint32_t(ram[i+1])<<8)|(uint32_t(ram[i+2])<<16)|(uint32_t(ram[i+3])<<24);}
    if(!(a&3)&&p>=0x1fc00000&&p<0x1fc80000){const auto i=p-0x1fc00000;return uint32_t(bios[i])|(uint32_t(bios[i+1])<<8)|(uint32_t(bios[i+2])<<16)|(uint32_t(bios[i+3])<<24);}
    if(timer_address(a))return read16(a);
    if(!(a&3)&&physical(a)>=0x1f801c00&&physical(a)<0x1f801e00)return uint32_t(read16(a))|(uint32_t(read16(a+2))<<16);
    uint32_t dma_value=0;if(dma_read(physical(a),dma_value))return dma_value;
    if(physical(a)==0x1f801810)return gpu.read_gp0();
    if(physical(a)==0x1f801814)return gpu.status;
    if(a&3) throw std::runtime_error("Unaligned word read");
    uint32_t v=0; for(unsigned i=0;i<4;i++) v|=uint32_t(read8(a+i))<<(8*i); return v;
}
void Bus::write8(uint32_t a,uint8_t v) {
    if(physical(a)>=0x1f801040&&physical(a)<0x1f801060){sio[(physical(a)-0x1f801040)/16].write(a&15,v);return;}
    if(physical(a)>=0x1f801800&&physical(a)<=0x1f801803){cd.write(physical(a)-0x1f801800,v);return;}
    if(boot_io_write(a,v))return;
    const uint32_t original=a;a=physical(a);
    if(a<0x800000) {ram[a&0x1fffff]=v;return;}
    if(a>=0x1f800000 && a<0x1f800400) {scratch[a-0x1f800000]=v;return;}
    throw bus_error("Unsupported/read-only bus write",original);
}
void Bus::write32(uint32_t a,uint32_t v) {
    const uint32_t p=physical(a);
    if(!(a&3)&&p<0x800000){const auto i=p&0x1fffff;for(unsigned byte=0;byte<4;byte++)ram[i+byte]=uint8_t(v>>(byte*8));return;}
    if(timer_address(a)){write16(a,uint16_t(v));return;}
    if(!(a&3)&&physical(a)>=0x1f801c00&&physical(a)<0x1f801e00){write16(a,uint16_t(v));write16(a+2,uint16_t(v>>16));return;}
    if(dma_write(physical(a),v))return;
    if(physical(a)==0x1f801810){gpu.gp0(v);return;}
    if(physical(a)==0x1f801814){gpu.gp1(v);return;}
    if(a&3) throw std::runtime_error("Unaligned word write");
    for(unsigned i=0;i<4;i++) write8(a+i,uint8_t(v>>(8*i)));
}
void Bus::load_bios(const std::string& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f || f.tellg()!=std::streamoff(bios.size())) throw std::runtime_error("BIOS must be exactly 512 KiB");
    f.seekg(0); if(!f.read(reinterpret_cast<char*>(bios.data()),bios.size())) throw std::runtime_error("BIOS read failed");
}
static int64_t signed32(uint32_t v) { return v<0x80000000u ? int64_t(v) : int64_t(v)-0x100000000ll; }
static uint32_t sar(uint32_t v,unsigned n) {
    n&=31; if(!n) return v;
    return (v>>n) | ((v&0x80000000u) ? (~uint32_t(0)<<(32-n)) : 0);
}
void Cpu::reset(uint32_t entry) {
    instructions=interrupts=0;trace.fill({});r.fill(0);cop0.fill(0);cop0[12]=0x00400000;cop0[15]=2;
    hi=lo=0;pc=entry;next_pc=entry+4;pending_reg=0;pending_value=0;delay_slot=false;
}
void Cpu::step() {
    const uint32_t ip=pc;
    const bool in_delay=delay_slot;
    auto exception=[&](unsigned code) {
        cop0[13]=(cop0[13]&0x0000ff00u)|(code<<2)|(in_delay?0x80000000u:0);
        cop0[14]=in_delay?ip-4:ip;
        cop0[12]=(cop0[12]&~0x3fu)|((cop0[12]<<2)&0x3f);
        pc=(cop0[12]&0x400000)?0xbfc00180:0x80000080;next_pc=pc+4;delay_slot=false;
    };
    cop0[13]=(cop0[13]&~0x400u)|((bus.irq_status&bus.irq_mask)?0x400u:0);
    if((cop0[12]&1)&&(cop0[12]&cop0[13]&0xff00)){
        if(pending_reg)r[pending_reg]=pending_value;
        pending_reg=0;exception(0);interrupts++;return;
    }
    if(ip&3){ if(pending_reg)r[pending_reg]=pending_value;pending_reg=0;cop0[8]=ip;exception(4);return; }
    const uint32_t ins=bus.read32(ip);
    trace[instructions%trace.size()]={ip,ins};instructions++;
    // Observe BIOS putchar entry without replacing or skipping its instructions.
    if(((ip&0x1fffffff)==0xa0 && r[9]==0x3c)||((ip&0x1fffffff)==0xb0 && r[9]==0x3d))bus.append_tty(char(r[4]&255));
    const unsigned op=ins>>26,s=(ins>>21)&31,t=(ins>>16)&31,d=(ins>>11)&31,sh=(ins>>6)&31;
    const uint32_t a=r[s],b=r[t],imm=uint32_t(int32_t(int16_t(ins&65535)));
    const unsigned old_pending=pending_reg;const uint32_t old_value=pending_value;
    if(pending_reg)r[pending_reg]=pending_value;
    pending_reg=0;pc=next_pc;next_pc=pc+4;delay_slot=false;
    auto put=[&](unsigned reg,uint32_t value){if(reg)r[reg]=value;};
    auto load=[&](uint32_t value){pending_reg=t;pending_value=value;};
    auto branch=[&](bool take){delay_slot=true;if(take)next_pc=ip+4+(imm<<2);};
    auto arithmetic=[&](unsigned reg,int64_t value){if(value>2147483647ll||value< -2147483648ll)exception(12);else put(reg,uint32_t(value));};
    const uint32_t address=a+imm;
    auto aligned=[&](unsigned mask,bool store){if(address&mask){cop0[8]=address;exception(store?5:4);return false;}return true;};
    auto half=[&](){return uint32_t(bus.read16(address));};
    switch(op) {
    case 0:
        switch(ins&63) {
        case 0:put(d,b<<sh);break;case 2:put(d,b>>sh);break;case 3:put(d,sar(b,sh));break;
        case 4:put(d,b<<(a&31));break;case 6:put(d,b>>(a&31));break;case 7:put(d,sar(b,a));break;
        case 8:delay_slot=true;next_pc=a;break;
        case 9:put(d,ip+8);delay_slot=true;next_pc=a;break;
        case 12:exception(8);break;case 13:exception(9);break;
        case 16:put(d,hi);break;case 17:hi=a;break;case 18:put(d,lo);break;case 19:lo=a;break;
        case 24:{uint64_t v=uint64_t(signed32(a)*signed32(b));lo=uint32_t(v);hi=uint32_t(v>>32);break;}
        case 25:{uint64_t v=uint64_t(a)*b;lo=uint32_t(v);hi=uint32_t(v>>32);break;}
        case 26:if(!b){lo=signed32(a)<0?1:0xffffffff;hi=a;}else{lo=uint32_t(signed32(a)/signed32(b));hi=uint32_t(signed32(a)%signed32(b));}break;
        case 27:if(!b){lo=0xffffffff;hi=a;}else{lo=a/b;hi=a%b;}break;
        case 32:arithmetic(d,signed32(a)+signed32(b));break;
        case 33:put(d,a+b);break;case 34:arithmetic(d,signed32(a)-signed32(b));break;case 35:put(d,a-b);break;
        case 36:put(d,a&b);break;case 37:put(d,a|b);break;case 38:put(d,a^b);break;case 39:put(d,~(a|b));break;
        case 42:put(d,signed32(a)<signed32(b));break;case 43:put(d,a<b);break;
        default:exception(10);break;
        }break;
    case 1:
        if(t==0||t==1||t==16||t==17){if(t&16)put(31,ip+8);branch((t&1)?signed32(a)>=0:signed32(a)<0);}else exception(10);break;
    case 2:case 3:if(op==3)put(31,ip+8);delay_slot=true;next_pc=((ip+4)&0xf0000000)|((ins&0x3ffffff)<<2);break;
    case 4:branch(a==b);break;case 5:branch(a!=b);break;
    case 6:branch(signed32(a)<=0);break;case 7:branch(signed32(a)>0);break;
    case 8:arithmetic(t,signed32(a)+signed32(imm));break;case 9:put(t,a+imm);break;
    case 10:put(t,signed32(a)<signed32(imm));break;case 11:put(t,a<imm);break;
    case 12:put(t,a&(ins&65535));break;case 13:put(t,a|(ins&65535));break;case 14:put(t,a^(ins&65535));break;case 15:put(t,(ins&65535)<<16);break;
    case 16:
        if(s==0)load(cop0[d]);
        else if(s==4){
            if(d==12||d==14||d==3||d==5||d==6||d==7||d==9||d==11)cop0[d]=b;
            else if(d==13)cop0[13]=(cop0[13]&~0x300u)|(b&0x300);
        }else if(s==16&&(ins&63)==16)cop0[12]=(cop0[12]&~15u)|((cop0[12]>>2)&15);
        else exception(10);
        break;
    case 32:load(uint32_t(int32_t(int8_t(bus.read8(address)))));break;
    case 33:if(aligned(1,false))load(uint32_t(int32_t(int16_t(half()))));break;
    case 35:if(aligned(3,false))load(bus.read32(address));break;
    case 36:load(bus.read8(address));break;case 37:if(aligned(1,false))load(half());break;
    case 34:case 38:{
        const uint32_t word=bus.read32(address&~3u),previous=(old_pending==t&&t)?old_value:b;
        const unsigned n=address&3;
        if(op==34){unsigned shift=(3-n)*8;load((previous&(0x00ffffffu>>(n*8)))|(word<<shift));}
        else {unsigned shift=n*8;uint32_t mask=n?0xffffffffu<<(32-shift):0;load((previous&mask)|(word>>shift));}break;}
    case 40:if(!(cop0[12]&0x10000))bus.write8(address,uint8_t(b));break;
    case 41:if(aligned(1,true)&&!(cop0[12]&0x10000)){bus.write16(address,uint16_t(b));}break;
    case 43:if(aligned(3,true)&&!(cop0[12]&0x10000))bus.write32(address,b);break;
    case 42:case 46:if(!(cop0[12]&0x10000)){
        uint32_t word=bus.read32(address&~3u);unsigned n=address&3;
        if(op==42){unsigned shift=(3-n)*8;uint32_t mask=n==3?0:0xffffffffu<<((n+1)*8);word=(word&mask)|(b>>shift);}
        else {unsigned shift=n*8;uint32_t mask=n?(1u<<shift)-1:0;word=(word&mask)|(b<<shift);}
        bus.write32(address&~3u,word);
    }break;
    default:exception(10);break;
    }
    r[0]=0;bus.tick_timers();
}
std::string Cpu::diagnostic()const{
 std::ostringstream out;out<<"PS1 Lab BIOS diagnostic\nInstructions: "<<instructions<<"\n";
 out<<std::hex<<std::setfill('0');
 out<<"PC="<<std::setw(8)<<pc<<" Next="<<std::setw(8)<<next_pc<<" SR="<<std::setw(8)<<cop0[12]<<" Cause="<<std::setw(8)<<cop0[13]<<" EPC="<<std::setw(8)<<cop0[14]<<"\n";
 out<<"POST="<<unsigned(bus.post)<<" RAM control="<<bus.ram_control<<" Cache control="<<bus.cache_control<<"\n";
 for(unsigned i=0;i<32;i++)out<<"R"<<std::dec<<i<<"="<<std::hex<<std::setw(8)<<r[i]<<((i%4==3)?"\n":" ");
 out<<"Recent fetched instructions (oldest first):\n";
 uint64_t start=instructions>trace.size()?instructions-trace.size():0;
 for(uint64_t i=start;i<instructions;i++){const auto& t=trace[i%trace.size()];out<<std::setw(8)<<t.pc<<": "<<std::setw(8)<<t.opcode<<"\n";}
 for(unsigned i=0;i<3;i++)out<<"Timer"<<i<<" count="<<bus.timers[i].count<<" mode="<<bus.timers[i].mode<<" target="<<bus.timers[i].target<<"\n";
 out<<"CPU IRQ exceptions="<<std::dec<<interrupts<<"\n";
 out<<"DPCR="<<std::hex<<bus.dpcr<<" DICR="<<bus.dicr<<" I_STAT="<<bus.irq_status<<" I_MASK="<<bus.irq_mask<<"\n";
 out<<"CD sectors="<<std::dec<<bus.cd.sectors<<" lba="<<bus.cd.lba<<" commands="<<bus.cd.commands<<" sectors_read="<<bus.cd.sectors_read<<" irq="<<unsigned(bus.cd.flag)<<"\n";
 out<<"Disc layout: "<<bus.cd.disc.description()<<"\n";
 out<<"CD mode="<<std::hex<<unsigned(bus.cd.mode)<<" last_command="<<unsigned(bus.cd.last_command)<<" error="<<unsigned(bus.cd.last_error)<<" data_fifo="<<std::dec<<bus.cd.data.size()<<" pending="<<bus.cd.pending.size()<<" region_emulated="<<bus.cd.effective_region()<<" boot="<<bus.cd.boot_name<<"\n";
 out<<"Pad connected="<<bus.sio[0].connected<<" buttons="<<std::hex<<bus.sio[0].buttons<<" phase="<<std::dec<<bus.sio[0].phase<<"\n";
 out<<"GPUSTAT="<<std::hex<<bus.gpu.status<<" DrawMode="<<bus.gpu.draw_mode<<" TextureWindow="<<bus.gpu.texture_window<<" PacketWords="<<std::dec<<bus.gpu.count<<"\n";
 out<<"GPU words="<<bus.gpu.words<<" polygons="<<bus.gpu.polygons<<" rectangles="<<bus.gpu.rectangles<<" lines="<<bus.gpu.line_segments<<"\n";
 out<<"Video line="<<std::dec<<bus.gpu.video_line<<" dot="<<bus.gpu.video_dot<<" field="<<bus.gpu.video_field<<" vblank="<<bus.gpu.video_vblank<<" frames="<<bus.gpu.video_frames<<" vblank_events="<<bus.gpu.vblank_events<<"\n";
 out<<"GPU display registers:";for(auto word:bus.gpu.display)out<<" "<<std::hex<<word;out<<"\n";
 out<<"Recent GP0 words (oldest first):\n";
 const auto first=bus.gpu.words>bus.gpu.history.size()?bus.gpu.words-bus.gpu.history.size():0;
 for(uint64_t i=first;i<bus.gpu.words;i++)out<<std::hex<<std::setw(8)<<bus.gpu.history[i%bus.gpu.history.size()]<<((i%8==7)?"\n":" ");
 out<<"\n";
 out<<"TTY (observed BIOS putchar entries):\n"<<bus.tty<<"\n";return out.str();
}
void install_demo(Bus& bus) {
    // addiu t0,zero,7; addiu t1,zero,5; addu t2,t0,t1; sw t2,256(zero)
    const uint32_t code[]={0x24080007,0x24090005,0x01095021,0xac0a0100};
    for(unsigned i=0;i<4;i++) bus.write32(i*4,code[i]);
}
}
