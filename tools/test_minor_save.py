#!/usr/bin/env python3
"""Synthetic THMIN001 v3 save-format regression (no player saves)."""
import struct
from inspect_minor_save import read_save, summary

width, height = 32, 24
count = width*height//2
header = bytearray(0xAA4)
struct.pack_into('<ii', header, 0, width, height)
tiles = bytearray(count*44)
abstract_x, abstract_y = (width+4)//5, (height+4)//5
abstract_size = abstract_y*((abstract_x+1)//2)

# Singletons, two-way combinations, and a three-way combination.
cases = {(10,8): [(3,1)],
         (11,9): [(1,120)],
         (12,8): [(2,7)],
         (14,8): [(1,7), (3,2)],
         (16,8): [(1,0), (2,1), (3,8)]}
records = bytearray()
for idx in range(count):
    y = idx//(width//2)
    x = 2*(idx%(width//2)) + (y & 1)
    features = cases.get((x,y), [])
    records.append(len(features))
    for typ, code in features:
        records.extend((typ, code))
    tiles[44*idx] = (3<<5) | (2<<3)
# Force an independent+native-landmark combination; it is representable.
i = 8*(width//2) + 16//2
struct.pack_into('<I', tiles, 44*i+12, 0x00000008)
raw = (b'TERRAN-demo\0' + header + tiles + bytes(abstract_size)
       + b'THMIN001' + struct.pack('<II',3,count) + records
       + b'post-map extra data')

w,h,v,parsed = read_save(raw)
assert (w,h,v)==(width,height,3)
assert parsed[(10,8)]['features']=={3:1}
assert parsed[(11,9)]['features']=={1:120}
assert parsed[(12,8)]['features']=={2:7}
assert parsed[(14,8)]['features']=={1:7, 3:2}
assert parsed[(16,8)]['features']=={1:0, 2:1, 3:8}
assert parsed[(18,8)]['features']=={}
assert parsed[(16,8)]['native_landmarks']==8
report=summary(w,h,v,parsed)
assert 'Titanium: 3 tiles' in report
assert 'Stacked independent tiles: 2' in report
assert 'Minor tiles overlapping native landmarks: 1' in report
assert 'Invalid type/code/ocean entries: 0' in report

# Reject truncated records, duplicate/out-of-order entries, invalid types/codes,
# and obsolete experimental v2, rather than silently corrupting game state.
offset = raw.index(b'THMIN001') + 16
def invalid(data, reason):
    try:
        read_save(data)
    except ValueError:
        return
    raise AssertionError(reason)

invalid(raw[:offset+1], 'truncation must fail')
invalid(raw[:offset-12] + struct.pack('<II', 2, count) + raw[offset:],
        'v2 must fail')
first = offset+8*(width//2)+10//2
# The first feature count in record array may shift; locate the target via a scan.
p = offset
for idx in range(count):
    x = 2*(idx%(width//2))+(idx//(width//2)&1)
    y = idx//(width//2)
    if (x,y) == (14,8):
        break
    n = raw[p]
    p += 1+2*n
dup = bytearray(raw)
dup[p+3] = 1
invalid(bytes(dup), 'duplicate type must fail')
bad = bytearray(raw)
bad[p+2] = 250
invalid(bytes(bad), 'bad code must fail')
print('Independent minor THMIN001 v3 parser: stacking, codes, native overlaps and malformed records passed')
