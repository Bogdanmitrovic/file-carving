#!/usr/bin/env python3
# walk.py <image> <jpeg_start_offset>
import mmap, sys

with open(sys.argv[1], "rb") as f:
    m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
start = int(sys.argv[2])
pos = start + 2
print(f"{start:>10}  SOI")
while pos + 4 <= len(m):
    if m[pos] != 0xFF:
        print(f"{pos:>10}  INVALID (no marker) - stop"); break
    mk = m[pos + 1]
    if mk == 0xFF:                        # fill byte
        pos += 1; continue
    if mk == 0xD9:
        print(f"{pos:>10}  EOI  -> file ends at {pos+2} ({pos+2-start} bytes)"); break
    if mk == 0x01 or 0xD0 <= mk <= 0xD8:  # no length field
        pos += 2; continue
    ln = int.from_bytes(m[pos+2:pos+4], "big")
    print(f"{pos:>10}  FF{mk:02X}  len={ln}  ends at {pos+2+ln}")
    pos += 2 + ln
    if mk == 0xDA:                        # entropy-coded data follows
        while True:
            i = m.find(b"\xff", pos)
            if i < 0 or i + 1 >= len(m):
                print("  ran off the end"); sys.exit()
            n = m[i + 1]
            if n == 0 or 0xD0 <= n <= 0xD7 or n == 0xFF:
                pos = i + (1 if n == 0xFF else 2); continue
            pos = i; break