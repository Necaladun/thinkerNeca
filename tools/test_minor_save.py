#!/usr/bin/env python3
"""Synthetic save-format regression test; does not commit any player saves."""
import struct
from inspect_minor_save import read_save, summary

width, height = 32, 24
count = width*height//2
header = bytearray(0xAA4)
struct.pack_into('<ii', header, 0, width, height)
tiles = bytearray(count*44)
abstract_x, abstract_y = (width+4)//5, (height+4)//5
abstract_size = abstract_y*((abstract_x+1)//2)
minor = bytearray(count*2)
# (10,8) Titanium, (11,9) Jungle, (12,8) Uranium
for x, y, typ, code in ((10,8,3,1),(11,9,1,120),(12,8,2,7)):
    idx = y*(width//2)+x//2
    minor[2*idx:2*idx+2] = bytes((typ,code))
    tiles[44*idx] = (3<<5) | (2<<3)
raw = (b'TERRAN-demo\0' + header + tiles + bytes(abstract_size)
       + b'THMIN001' + struct.pack('<II',2,count) + minor
       + b'post-map extra data')
w,h,v,parsed = read_save(raw)
assert (w,h,v)==(width,height,2)
assert parsed[(10,8)]['type']==3 and parsed[(10,8)]['code']==1
assert parsed[(11,9)]['type']==1 and parsed[(11,9)]['code']==120
assert parsed[(12,8)]['type']==2 and parsed[(12,8)]['code']==7
assert parsed[(14,8)]['type']==0
report=summary(w,h,v,parsed)
assert 'Titanium: 1 tiles' in report
assert 'Minor tiles overlapping native landmarks: 0' in report
assert 'Invalid type/code/ocean entries: 0' in report
print('Independent minor SAV parser: synthetic header, tile offsets and codes passed')
