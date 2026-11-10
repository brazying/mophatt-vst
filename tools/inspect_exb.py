#!/usr/bin/env python3
"""Read-only structural report for Emulator X EXB voice data.

This deliberately reports raw fields instead of guessing undocumented DSP
parameter mappings. It is used to ground the sampler engine implementation in
the actual factory bank shipped with this repository.
"""
from __future__ import annotations

import collections
import pathlib
import struct
import sys


def u32be(data: bytes, off: int) -> int:
    return struct.unpack_from(">I", data, off)[0]


def chunks(data: bytes, start: int, end: int):
    pos = start
    while pos + 8 <= end:
        tag = data[pos:pos + 4].decode("ascii", "replace")
        size = u32be(data, pos + 4)
        body = pos + 8
        stop = min(body + size, end)
        yield tag, body, stop
        if body + size > end:
            return
        pos = body + size


def list_children(data: bytes, body: int, stop: int):
    if stop - body < 4:
        return "", []
    kind = data[body:body + 4].decode("ascii", "replace")
    return kind, list(chunks(data, body + 4, stop))


def hex_head(data: bytes, start: int, stop: int, count: int = 48) -> str:
    return data[start:min(stop, start + count)].hex(" ")


def main(path: str):
    data = pathlib.Path(path).read_bytes()
    if data[:4] != b"FORM" or data[8:12] != b"E5B0":
        raise SystemExit("Not an Emulator X E5B0 bank")

    toc = None
    for tag, body, stop in chunks(data, 12, len(data)):
        if tag == "TOC2":
            toc = (body, stop)
            break
    if toc is None:
        raise SystemExit("TOC2 not found")

    toc_body, toc_stop = toc
    entry_counts = collections.Counter()
    preset_count = 0
    voice_count = 0
    zone_count = 0
    sample_links = []
    voice_tags = collections.Counter()
    list_tags = collections.Counter()
    parameter_examples = collections.defaultdict(list)
    envelope_sizes = collections.Counter()
    lfo_sizes = collections.Counter()
    fg_sizes = collections.Counter()
    cord_sizes = collections.Counter()

    for off in range(toc_body, toc_stop - 77, 78):
        entry = data[off:off + 78]
        tag = entry[:4].decode("ascii", "replace")
        entry_counts[tag] += 1
        if tag == "E5SL":
            sample_links.append((struct.unpack_from(">H", entry, 12)[0], entry[14:78].decode("utf-16le", "replace").split("\0")[0]))
        if tag != "E5P1":
            continue

        preset_count += 1
        chunk_off = u32be(entry, 8)
        if chunk_off + 8 > len(data):
            continue
        chunk_size = u32be(data, chunk_off + 4)
        preset_end = min(chunk_off + 8 + chunk_size, len(data))
        for ptag, pbody, pstop in chunks(data, chunk_off + 8, preset_end):
            if ptag != "LIST" or data[pbody:pbody + 4] != b"E5VL":
                continue
            for vtag, vbody, vstop in chunks(data, pbody + 4, pstop):
                if vtag != "E5V1":
                    continue
                voice_count += 1
                for ctag, cbody, cstop in chunks(data, vbody, vstop):
                    voice_tags[ctag] += 1
                    if ctag == "LIST":
                        kind, children = list_children(data, cbody, cstop)
                        list_tags[kind] += 1
                        for child_tag, child_body, child_stop in children:
                            size = child_stop - child_body
                            if child_tag == "Zhdr":
                                zone_count += 1
                                if size >= 12:
                                    sample_id = struct.unpack_from(">H", data, child_body + 4)[0]
                                    root = data[child_body + 10]
                                    parameter_examples["Zhdr"].append((sample_id, root, hex_head(data, child_body, child_stop, 24)))
                            elif child_tag == "E5Ev":
                                envelope_sizes[size] += 1
                                if len(parameter_examples["E5Ev"]) < 12:
                                    parameter_examples["E5Ev"].append(hex_head(data, child_body, child_stop, 72))
                            elif child_tag == "E5LF":
                                lfo_sizes[size] += 1
                                if len(parameter_examples["E5LF"]) < 8:
                                    parameter_examples["E5LF"].append(hex_head(data, child_body, child_stop, 80))
                            elif child_tag == "EFGn":
                                fg_sizes[size] += 1
                                if len(parameter_examples["EFGn"]) < 8:
                                    parameter_examples["EFGn"].append(hex_head(data, child_body, child_stop, 80))
                            elif child_tag == "E5Cd":
                                cord_sizes[size] += 1
                                if len(parameter_examples["E5Cd"]) < 8:
                                    parameter_examples["E5Cd"].append(hex_head(data, child_body, child_stop, 32))
                    elif ctag in ("E5Fl", "E5Am"):
                        if len(parameter_examples[ctag]) < 20:
                            parameter_examples[ctag].append(hex_head(data, cbody, cstop, 96))

    print(f"FILE: {path}")
    print(f"BYTES: {len(data)}")
    print(f"TOC ENTRIES: {sum(entry_counts.values())} {dict(entry_counts)}")
    print(f"PRESETS: {preset_count}")
    print(f"VOICES: {voice_count}")
    print(f"ZONES: {zone_count}")
    print(f"SAMPLE LINKS: {len(sample_links)}")
    print(f"VOICE CHUNK TAGS: {dict(voice_tags)}")
    print(f"LIST TYPES: {dict(list_tags)}")
    print(f"ENVELOPE PAYLOAD SIZES: {dict(envelope_sizes)}")
    print(f"LFO PAYLOAD SIZES: {dict(lfo_sizes)}")
    print(f"FUNCTION GENERATOR PAYLOAD SIZES: {dict(fg_sizes)}")
    print(f"CORD PAYLOAD SIZES: {dict(cord_sizes)}")
    for key, values in parameter_examples.items():
        print(f"\n[{key}] examples={len(values)}")
        for value in values:
            print(value if isinstance(value, str) else " | ".join(map(str, value)))
    print("\nFIRST 30 SAMPLE LINKS:")
    for item in sample_links[:30]:
        print(item)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "Xtreme Lead-1.exb")
