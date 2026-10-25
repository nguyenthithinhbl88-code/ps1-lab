"""Generate our own BIOS-free PS-X EXE; no compiler/SDK needed."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parents[1]
base=0x80010000
code=[]
def emit(v):code.append(v)
def li(reg,value):
 emit(0x3c000000|(reg<<16)|(value>>16));emit(0x34000000|(reg<<21)|(reg<<16)|(value&65535))
def sw(reg,addr):li(8,addr);emit(0xac000000|(8<<21)|(reg<<16))
# GPU list at 0x11000: clear and draw a triangle + a quad.
commands=[0x02201008,0,(240<<16)|320,0xe3000000,0xe4000000|(239<<10)|319,
          0x200080ff,(25<<16)|160,(210<<16)|25,(210<<16)|295,
          0x28ff8030,(110<<16)|110,(110<<16)|210,(170<<16)|110,(170<<16)|210]
li(9,0x07654b21);sw(9,0x1f8010f0)
li(9,0x11000);sw(9,0x1f8010a0)
li(9,0x01000401);sw(9,0x1f8010a8)
loop=base+len(code)*4;emit(0x08000000|((loop>>2)&0x3ffffff));emit(0)
payload=bytearray(0x1000+4+len(commands)*4)
for i,w in enumerate(code):struct.pack_into('<I',payload,i*4,w)
struct.pack_into('<I',payload,0x1000,(len(commands)<<24)|0xffffff)
for i,w in enumerate(commands):struct.pack_into('<I',payload,0x1004+i*4,w)
header=bytearray(0x800);header[:8]=b'PS-X EXE'
for offset,value in [(0x10,base),(0x18,base),(0x1c,len(payload)),(0x30,0x801fff00)]:struct.pack_into('<I',header,offset,value)
(root/'examples/dma_polygon.exe').write_bytes(header+payload)
print('Generated examples/dma_polygon.exe')
