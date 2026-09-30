#!/usr/bin/env python3
# scan.py <image> [start_offset] [end_offset]
import mmap, re, sys

path  = sys.argv[1]
start = int(sys.argv[2]) if len(sys.argv) > 2 else 0

with open(path, "rb") as f:
    m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    end = int(sys.argv[3]) if len(sys.argv) > 3 else len(m)

    events = []
    for h in re.finditer(rb"(?=\xff\xd8\xff(.))", m, re.DOTALL):
        events.append((h.start(), "HEADER", h.group(1)[0]))
    for t in re.finditer(rb"(?=\xff\xd9)", m):
        events.append((t.start(), "FOOTER", None))

    events.sort()
    for off, kind, b4 in events:
        if not (start <= off < end):
            continue
        extra = f"FF D8 FF {b4:02X}" if kind == "HEADER" else "FF D9"
        aligned = "aligned512" if off % 512 == 0 else ""
        print(f"{off:>12}  {kind}  {extra:<12} {aligned}")