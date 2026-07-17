#!/usr/bin/env python3
import struct
MAGIC=0x32524256; VERSION=1; HEADER=26; TAG=4; KEY=0x59a6c31d

def tag(data,key):
    h=(2166136261 ^ key)&0xffffffff
    for b in data:
        h ^= b; h=(h*16777619)&0xffffffff
    return (h ^ (((key<<13)&0xffffffff)|(key>>19)))&0xffffffff

def frame(op=17,generation=9,counter=1,capability=7,request=b'v_request1',payload=b'[1,2,3]'):
    b=bytearray(HEADER+len(request)+len(payload)+TAG)
    struct.pack_into('<IHHIIIHI',b,0,MAGIC,VERSION,op,generation,counter,capability,len(request),len(payload))
    b[HEADER:HEADER+len(request)]=request; b[HEADER+len(request):-TAG]=payload
    struct.pack_into('<I',b,len(b)-TAG,tag(b[:-TAG],KEY)); return bytes(b)

def valid(raw,generation=9):
    if len(raw)<HEADER+TAG: return False
    magic,version,op,gen,counter,cap,rl,pl=struct.unpack_from('<IHHIIIHI',raw,0)
    if magic!=MAGIC or version!=VERSION or gen!=generation or counter==0: return False
    if HEADER+rl+pl+TAG!=len(raw): return False
    return struct.unpack_from('<I',raw,len(raw)-TAG)[0]==tag(raw[:-TAG],KEY)
base=frame(); assert valid(base)
mutations=[]
for offset in (0,4,8,12,16,20,22,HEADER,len(base)-TAG-1,len(base)-1):
    m=bytearray(base); m[offset]^=0x01; mutations.append(bytes(m))
mutations += [base[:-1], b'\x00'*len(base), frame(generation=10), frame(counter=0)]
accepted=[i for i,m in enumerate(mutations) if valid(m)]
if accepted: raise SystemExit(f'mutated bridge frames accepted: {accepted}')
print(f'bridge frame mutation smoke: PASS ({len(mutations)} corruptions rejected)')
