"""Original diagnostic ROM, not a Sony BIOS or BIOS replacement."""
from pathlib import Path
import struct
code=[]
def emit(w):code.append(w)
def li(r,v):emit(0x3c000000|(r<<16)|(v>>16));emit(0x34000000|(r<<21)|(r<<16)|(v&65535))
def store(r,a):li(8,a);emit(0xac000000|(8<<21)|(r<<16))
li(9,0x1f000000);store(9,0x1f801000)
li(9,0x1f802000);store(9,0x1f801004)
li(9,0xb88);store(9,0x1f801060)
li(9,0x804);store(9,0xfffe0130)
# Real code at B0: jr ra / nop. No HLE service substitution.
li(9,0x03e00008);store(9,0xb0);li(9,0);store(9,0xb4)
for char in b'BOOT ROM: CPU -> BIOS ROM -> MMIO -> RAM -> B0 OK\n':
 li(9,0x3d);li(4,char);li(10,0xb0);emit(0x0140f809);emit(0)
li(9,0x600d);store(9,0x100)
li(8,0x1f802041);li(9,0x42);emit(0xa1090000)
loop=0xbfc00000+len(code)*4;emit(0x08000000|((loop>>2)&0x3ffffff));emit(0)
rom=bytearray(512*1024)
for i,w in enumerate(code):struct.pack_into('<I',rom,i*4,w)
(Path(__file__).resolve().parents[1]/'examples/boot_test_rom.bin').write_bytes(rom)
print('Generated original diagnostic ROM')
