#!/usr/bin/env python3
"""Read-only structural report for Emulator X EXB preset/voice/zone chunks."""
import collections
import pathlib
import struct
import sys

def be16(b, p): return struct.unpack_from(">H", b, p)[0]
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

def find_list(b, start, end, list_type):
    for tag, _, body, stop, _ in chunks(b, start, end):
        if tag == "LIST" and stop-body >= 4 and b[body:body+4] == list_type:
            return body+4, stop
    return None

def main(path):
    b = pathlib.Path(path).read_bytes()
    if b[:4] != b"FORM" or b[8:12] != b"E5B0":
        raise SystemExit("Not an Emulator X E5B0 bank")
    toc = next((x for x in chunks(b, 12, len(b)) if x[0] == "TOC2"), None)
    if not toc: raise SystemExit("TOC2 missing")
    _, _, toc_body, toc_end, _ = toc

    toc_counts = collections.Counter()
    voice_counts = collections.Counter()
    list_counts = collections.Counter()
    chunk_counts = collections.Counter()
    sample_ids = collections.Counter()
    sample_links = []
    preset_examples = []
    raw_examples = collections.defaultdict(list)

    for p in range(toc_body, toc_end-77, 78):
        e = b[p:p+78]
        tag = e[:4].decode("ascii", "replace")
        toc_counts[tag] += 1
        if tag == "E5SL":
            link_offset = be32(e, 8)
            if link_offset + 14 <= len(b) and b[link_offset:link_offset+4] == b"E5SL":
                body = link_offset + 8
                sample_links.append((be16(b, body), be32(b, body+2)))
            continue
        if tag != "E5P1": continue
        name = e[14:78].decode("utf-16le", "replace").split("\0")[0]
        preset_offset = be32(e, 8)
        if preset_offset + 8 > len(b) or b[preset_offset:preset_offset+4] != b"E5P1":
            continue
        preset_size = be32(b, preset_offset+4)
        preset_end = min(preset_offset+8+preset_size, len(b))
        # The TOC pointer lands on E5P1; its child chunks start after the
        # 8-byte chunk header and the two-byte preset prefix.
        voice_list = find_list(b, preset_offset+10, preset_end, b"E5VL")
        if voice_list is None: continue
        vstart, vend = voice_list
        this_preset_zones = 0
        this_preset_voices = 0
        for vtag, _, vbody, vstop, _ in chunks(b, vstart, vend):
            if vtag != "E5V1": continue
            this_preset_voices += 1
            voice_counts["E5V1"] += 1
            for ctag, _, cbody, cstop, csize in chunks(b, vbody, vstop):
                chunk_counts[ctag] += 1
                if ctag == "E5Fl" and len(raw_examples["E5Fl"]) < 16:
                    raw_examples["E5Fl"].append(b[cbody:cstop].hex(" "))
                if ctag == "E5Am" and len(raw_examples["E5Am"]) < 12:
                    raw_examples["E5Am"].append(b[cbody:cstop].hex(" "))
                if ctag != "LIST" or cstop-cbody < 4: continue
                list_type = b[cbody:cbody+4].decode("ascii", "replace")
                list_counts[list_type] += 1
                child_start = cbody+4
                for child_tag, _, child_body, child_stop, child_size in chunks(b, child_start, cstop):
                    if child_tag == "Zhdr" and child_size >= 12:
                        sid = be16(b, child_body+4)
                        sample_ids[sid] += 1
                        this_preset_zones += 1
                    elif child_tag in ("E5Ev", "E5LF", "EFGn", "E5Cd"):
                        key = child_tag + "_size"
                        voice_counts[f"{key}:{child_size}"] += 1
                        if len(raw_examples[child_tag]) < 10:
                            raw_examples[child_tag].append(b[child_body:child_stop].hex(" "))
        voice_counts["presets_with_voice"] += 1 if this_preset_voices else 0
        voice_counts["zones_total"] += this_preset_zones
        if len(preset_examples) < 30:
            preset_examples.append((name, this_preset_voices, this_preset_zones))

    print("FILE", path, "BYTES", len(b))
    print("TOC", dict(toc_counts))
    print("VOICE_AND_ZONE_COUNTS", dict(voice_counts))
    print("VOICE_CHILD_CHUNKS", dict(chunk_counts))
    print("VOICE_LIST_TYPES", dict(list_counts))
    print("UNIQUE_SAMPLE_IDS_IN_ZONES", len(sample_ids))
    print("PRESET EXAMPLES (name, voices, zones)")
    for row in preset_examples: print(row)
    for tag, values in raw_examples.items():
        print("RAW", tag, "examples", len(values))
        for value in values: print(value)
    print("FIRST SAMPLE IDS", sample_ids.most_common(40))
    print("FIRST E5SL LINKS (slot, sample_index)", sample_links[:60])
    print("E5SL SLOT RANGE", (min((x[0] for x in sample_links), default=None), max((x[0] for x in sample_links), default=None)))
    print("E5SL SAMPLE INDEX RANGE", (min((x[1] for x in sample_links), default=None), max((x[1] for x in sample_links), default=None)))
    print("ZONE IDS MATCH E5SL SLOTS", sum(1 for sid in sample_ids if sid in {x[0] for x in sample_links}), "of", len(sample_ids))
    print("ZONE IDS MATCH E5SL SAMPLE INDICES", sum(1 for sid in sample_ids if sid in {x[1] for x in sample_links}), "of", len(sample_ids))

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "Xtreme Lead-1.exb")
