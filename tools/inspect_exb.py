#!/usr/bin/env python3
"""Read-only EXB structural probe. Reports offsets and raw payloads; no guessed DSP mappings."""
import collections
import pathlib
import struct
import sys

def be32(b, p): return struct.unpack_from(">I", b, p)[0]

def chunks(b, start, end):
    p = start
    while p + 8 <= end:
        tag = b[p:p+4].decode("ascii", "replace")
        size = be32(b, p+4)
        body, stop = p+8, min(p+8+size, end)
        yield tag, p, body, stop, size
        if p+8+size > end: return
        p = p+8+size

def main(path):
    b = pathlib.Path(path).read_bytes()
    print("FILE", path, "BYTES", len(b), "FORM", b[:12].hex(" "))
    toc = next((x for x in chunks(b, 12, len(b)) if x[0] == "TOC2"), None)
    if not toc: raise SystemExit("TOC2 missing")
    _, _, tb, te, _ = toc
    counts = collections.Counter()
    pointers = []
    for p in range(tb, te-77, 78):
        e = b[p:p+78]
        tag = e[:4].decode("ascii", "replace")
        counts[tag] += 1
        if tag == "E5P1" and len(pointers) < 12:
            ptr = be32(e, 8)
            idx = struct.unpack_from(">H", e, 12)[0]
            pointers.append((idx, ptr, e[:14].hex(" "), [(q, b[q:q+24].hex(" ")) for q in (ptr-8, ptr, ptr+8, ptr+10) if 0 <= q < len(b)]))
    print("TOC COUNTS", dict(counts))
    print("FIRST PRESET POINTERS")
    for x in pointers: print(x)

    tags = collections.Counter()
    examples = collections.defaultdict(list)
    for p in range(0, len(b)-8):
        tag = b[p:p+4]
        if tag not in (b"E5P1", b"LIST", b"E5VL", b"E5V1", b"E5ZL", b"Zhdr", b"E5Fl", b"E5Am", b"E5Ev", b"E5LF", b"EFGn", b"E5Cd"):
            continue
        size = be32(b, p+4)
        if size > len(b)-p-8: continue
        name = tag.decode("ascii")
        tags[name] += 1
        if len(examples[name]) < 12:
            examples[name].append((p, size, b[p+8:min(p+8+size,p+8+80)].hex(" ")))
    print("GLOBAL TAG COUNTS", dict(tags))
    for name, vals in examples.items():
        print("TAG", name)
        for val in vals: print(val)

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "Xtreme Lead-1.exb")
