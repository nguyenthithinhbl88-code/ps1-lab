"""Original MIPS homebrew game + ISO/Mode2 BIN/CUE, no Sony code or assets.
The fixture's raw EDC/ECC area is zero: for emulator testing, not disc burning.
"""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
BASE=0x80010000
code=[];labels={};fixups=[]
def emit(w):code.append(w&0xffffffff)
def label(name):labels[name]=len(code)
def li(r,v):emit((15<<26)|(r<<16)|(v>>16));emit((13<<26)|(r<<21)|(r<<16)|(v&65535))
def addi(t,s,i):emit((9<<26)|(s<<21)|(t<<16)|(i&65535))
def op(f,d,s,t):emit((s<<21)|(t<<16)|(d<<11)|f)
def mem(o,t,s,i=0):emit((o<<26)|(s<<21)|(t<<16)|(i&65535))
def branch(o,s,t,target):fixups.append((len(code),target));emit((o<<26)|(s<<21)|(t<<16));emit(0)
def jump(target):fixups.append((len(code),target,'j'));emit(2<<26);emit(0)
def andi(t,s,i):emit((12<<26)|(s<<21)|(t<<16)|i)
def shift(d,t,n):emit((t<<16)|(d<<11)|(n<<6))
def send(word,port=16):li(8,word);mem(43,8,port)
def sb(value,port,offset):addi(8,0,value);mem(40,8,port,offset)
# s0 GPU, s1 pad, s2 IRQ, s3 state; t6 is CD register base.
li(16,0x1f801810);li(17,0x1f801040);li(18,0x1f801070);li(19,0x2000);li(14,0x1f801800)
# Setloc PVD -> ReadN -> IRQ1 -> DMA3 -> Pause. Commands run via actual MMIO.
def cd_ack(tag):
 sb(1,14,0);label(tag+'_wait');mem(36,9,14,3);emit(0);andi(9,9,7);branch(4,9,0,tag+'_wait')
 label(tag+'_drain');mem(36,10,14,0);emit(0);andi(10,10,32);branch(4,10,0,tag+'_clear');mem(36,10,14,1);jump(tag+'_drain')
 label(tag+'_clear');sb(31,14,3);sb(0,14,0)
sb(0,14,0)
for value in (0,2,0x16):sb(value,14,2)
sb(2,14,1);cd_ack('loc');sb(6,14,1);cd_ack('read');cd_ack('sector')
sb(0x80,14,3)
for address,value in ((0x1f8010f0,0x8000),(0x1f8010b0,0x3000),(0x1f8010b4,512),(0x1f8010b8,0x11000000)):
 li(9,address);li(8,value);mem(43,8,9)
sb(9,14,1)
label('pause_more');cd_ack('pause');addi(10,0,2);branch(5,9,10,'pause_more')
# GPU setup and display source. No BIOS service calls.
send(0,16) # GP0 NOP
li(15,0x1f801814)
for word in (0,0x08000001,0x05000000,0x06c60260,0x07040010,0x03000000):send(word,15)
for word in (0xe3000000,0xe403bd3f,0xe5000000,0xe6000000):send(word)
# MODE and CTRL for a digital pad, synchronous receive.
addi(8,0,13);mem(41,8,17,8)
addi(20,0,144);addi(21,0,144);addi(22,0,16);addi(23,0,0);addi(26,0,0);addi(27,0,0)
label('frame')
label('vsync');mem(35,8,18);emit(0);andi(8,8,1);branch(4,8,0,'vsync');addi(8,0,-2);mem(43,8,18)
# Deselect/select starts a new 01,42,00,00,00 poll; buttons are active low.
addi(8,0,1);mem(41,8,17,10);addi(8,0,3);mem(41,8,17,10)
for byte in (1,0x42,0):sb(byte,17,0);mem(36,8,17);emit(0)
sb(0,17,0);mem(36,9,17);emit(0);sb(0,17,0);mem(36,8,17);emit(0)
andi(8,9,128);branch(5,8,0,'not_left');addi(20,20,-4);branch(1,20,1,'not_left');addi(20,0,0)
label('not_left');andi(8,9,32);branch(5,8,0,'not_right');addi(20,20,4)
addi(8,0,296);op(42,9,8,20);branch(4,9,0,'not_right');addi(20,0,296)
label('not_right');addi(22,22,3);addi(27,27,1);addi(8,0,208);op(42,9,22,8);branch(5,9,0,'draw')
addi(8,20,-8);op(42,9,21,8);branch(5,9,0,'miss');addi(8,20,24);op(42,9,21,8);branch(4,9,0,'miss')
addi(23,23,1);jump('respawn')
label('miss');addi(26,26,1);addi(8,0,3);branch(5,26,8,'respawn');addi(23,0,0);addi(26,0,0)
label('respawn');addi(22,0,16);addi(21,21,73);addi(8,0,304);op(42,9,8,21);branch(4,9,0,'draw');addi(21,21,-304)
label('draw')
# Clear background then draw paddle, falling star, ground, score and lives.
for word in (0x02201810,0,(240<<16)|320):send(word)
def rect(color,xreg,yreg,width,height):
 send(0x60000000|color);shift(8,yreg,16);op(37,8,8,xreg);mem(43,8,16);send((height<<16)|width)
addi(10,0,220);rect(0x40e040,20,10,24,8);rect(0x00e0ff,21,22,8,8)
addi(9,0,0);addi(10,0,232);rect(0xa06030,9,10,320,8)
# Score bar length; clamp to 30 to stay in visible area.
addi(8,0,30);op(42,9,8,23);branch(4,9,0,'score_ok');addi(23,0,0)
label('score_ok');send(0x60ffb040);send((8<<16)|8);shift(8,23,3);li(9,4<<16);op(37,8,8,9);mem(43,8,16)
send(0x604040ff);send((8<<16)|260);addi(8,0,3);op(35,8,8,26);shift(9,8,3);shift(8,8,2);op(33,8,8,9);li(9,4<<16);op(37,8,8,9);mem(43,8,16)
# Observable game state for tests/debugger, not an emulator-specific input API.
for r,offset in ((20,0),(27,4),(23,8),(22,12),(26,16)):mem(43,r,19,offset)
jump('frame')
for item in fixups:
 i,name,*kind=item;target=labels[name]
 if kind:code[i]|=((BASE+target*4)>>2)&0x3ffffff
 else:code[i]|=(target-i-1)&65535
payload=b''.join(struct.pack('<I',w) for w in code)
payload=payload.ljust((len(payload)+2047)//2048*2048,b'\0')
header=bytearray(2048);header[:8]=b'PS-X EXE'
for offset,value in ((0x10,BASE),(0x18,BASE),(0x1c,len(payload)),(0x30,0x801fff00)):struct.pack_into('<I',header,offset,value)
exe=bytes(header)+payload
(ROOT/'examples/catch_stars.exe').write_bytes(exe)
# Root SYSTEM.CNF and nested GAME/MAIN.EXE;1 exercise real ISO path traversal.
def both32(b,o,v):struct.pack_into('<I',b,o,v);struct.pack_into('>I',b,o+4,v)
def both16(b,o,v):struct.pack_into('<H',b,o,v);struct.pack_into('>H',b,o+2,v)
def record(name,lba,size,flags=0):
 b=bytearray(33+len(name)+(len(name)%2==0));b[0]=len(b);both32(b,2,lba);both32(b,10,size);b[25]=flags;both16(b,28,1);b[32]=len(name);b[33:33+len(name)]=name;return b
config=b'BOOT = cdrom:\\GAME\\MAIN.EXE;1\r\nTCB = 4\r\nEVENT = 10\r\nSTACK = 801FFF00\r\n'
count=23+len(exe)//2048;iso=bytearray(count*2048)
p=bytearray(2048);p[0]=1;p[1:6]=b'CD001';p[6]=1;p[40:72]=b'PS1_LAB_CATCH_STARS'.ljust(32);both32(p,80,count);both16(p,120,1);both16(p,124,1);both16(p,128,2048);both32(p,132,22);struct.pack_into('<I',p,140,18);struct.pack_into('>I',p,148,19);p[156:190]=record(b'\0',20,2048,2);p[881]=1;iso[16*2048:17*2048]=p
iso[17*2048:17*2048+7]=bytes([255])+b'CD001'+bytes([1])
for sector,endian in ((18,'<'),(19,'>')):
 table=bytearray(22);table[0]=1;struct.pack_into(endian+'I',table,2,20);struct.pack_into(endian+'H',table,6,1);table[10]=4;struct.pack_into(endian+'I',table,12,21);struct.pack_into(endian+'H',table,16,1);table[18:22]=b'GAME';iso[sector*2048:sector*2048+22]=table
root=record(b'\0',20,2048,2)+record(b'\1',20,2048,2)+record(b'SYSTEM.CNF;1',22,len(config))+record(b'GAME',21,2048,2)
sub=record(b'\0',21,2048,2)+record(b'\1',20,2048,2)+record(b'MAIN.EXE;1',23,len(exe))
iso[20*2048:20*2048+len(root)]=root;iso[21*2048:21*2048+len(sub)]=sub;iso[22*2048:22*2048+len(config)]=config;iso[23*2048:]=exe
(ROOT/'examples/catch_stars.iso').write_bytes(iso)
def bcd(v):return(v//10)*16+v%10
raw=bytearray()
for n in range(count):
 b=bytearray(2352);b[1:11]=b'\xff'*10;at=n+150;b[12:16]=bytes((bcd(at//4500),bcd(at//75%60),bcd(at%75),2));b[16:24]=bytes((0,0,8,0))*2;b[24:2072]=iso[n*2048:(n+1)*2048];raw+=b
(ROOT/'examples/catch_stars.bin').write_bytes(raw)
(ROOT/'examples/catch_stars.cue').write_text('FILE "catch_stars.bin" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n')
print(f'Generated original Catch Stars: {len(code)} MIPS instructions, {count} sectors')
