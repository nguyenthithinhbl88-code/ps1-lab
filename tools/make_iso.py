"""Original ISO9660 fixture containing our own BIOS-free demo."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parents[1]
exe=(root/'examples/dma_polygon.exe').read_bytes()
count=21+(len(exe)+2047)//2048
iso=bytearray(count*2048)
def both32(b,o,v):struct.pack_into('<I',b,o,v);struct.pack_into('>I',b,o+4,v)
def both16(b,o,v):struct.pack_into('<H',b,o,v);struct.pack_into('>H',b,o+2,v)
def record(name,lba,size,flags):
 b=bytearray(33+len(name)+(len(name)%2==0));b[0]=len(b);both32(b,2,lba);both32(b,10,size);b[25]=flags;both16(b,28,1);b[32]=len(name);b[33:33+len(name)]=name;return b
p=bytearray(2048);p[0]=1;p[1:6]=b'CD001';p[6]=1;p[40:72]=b'PS1_LAB_HOMEBREW'.ljust(32);both32(p,80,count);both16(p,120,1);both16(p,124,1);both16(p,128,2048);both32(p,132,10);struct.pack_into('<I',p,140,18);struct.pack_into('>I',p,148,19);p[156:190]=record(b'\0',20,2048,2);p[881]=1;iso[16*2048:17*2048]=p
iso[17*2048:17*2048+7]=bytes([255])+b'CD001'+bytes([1])
for sector,endian in [(18,'<'),(19,'>')]:
 t=bytearray(10);t[0]=1;struct.pack_into(endian+'I',t,2,20);struct.pack_into(endian+'H',t,6,1);iso[sector*2048:sector*2048+10]=t
records=record(b'\0',20,2048,2)+record(b'\1',20,2048,2)+record(b'PSX.EXE;1',21,len(exe),0)
iso[20*2048:20*2048+len(records)]=records;iso[21*2048:21*2048+len(exe)]=exe
(root/'examples/homebrew.iso').write_bytes(iso)
print('Generated examples/homebrew.iso (original demo only)')
