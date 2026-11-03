#!/usr/bin/env python3
# Verify actual ELF PT_LOAD span and required exported callbacks, not just section totals.
import sys, struct, hashlib, json

if len(sys.argv) < 2:
    print("Usage: python check_unit.py <package> [manifest.json]", file=sys.stderr)
    sys.exit(1)

package_path = sys.argv[1]
manifest_path = sys.argv[2] if len(sys.argv) > 2 else None

with open(package_path, "rb") as f:
    b = f.read()

u16 = lambda o: struct.unpack_from("<H", b, o)[0]
u32 = lambda o: struct.unpack_from("<I", b, o)[0]

if len(b) < 52 or b[:4] != b"\x7fELF" or b[4] != 1 or b[5] != 1 or u16(18) != 40:
    raise ValueError("Expected little-endian ELF32 ARM")

segments = []
for i in range(u16(44)):
    p = u32(28) + i * u16(42)
    if p + 32 > len(b):
        raise ValueError("Invalid program header")
    if u32(p) == 1:
        offset, base, files, memory = u32(p + 4), u32(p + 8), u32(p + 16), u32(p + 20)
        if offset + files > len(b) or files > memory:
            raise ValueError("Invalid load segment")
        segments.append((base, memory))

if not segments:
    raise ValueError("No loadable segments")

load_bytes = max(s[0] + s[1] for s in segments) - min(s[0] for s in segments)
if load_bytes > 49152 or len(b) > 49152:
    raise ValueError(f"48 KiB limit exceeded: load={load_bytes} package={len(b)}")

sections = []
for i in range(u16(48)):
    s = u32(32) + i * u16(46)
    sections.append({
        "type": u32(s + 4),
        "offset": u32(s + 16),
        "size": u32(s + 20),
        "link": u32(s + 24),
        "entry": u32(s + 36)
    })

callback_exports = set()
for s in [sec for sec in sections if sec["type"] == 11]:
    strings = sections[s["link"]]
    if not s["entry"]:
        raise ValueError("Invalid symbol table")
    for p in range(s["offset"], s["offset"] + s["size"], s["entry"]):
        shndx = u16(p + 14)
        info = b[p + 12]
        other = b[p + 13]
        if shndx != 0 and (other & 3) == 0 and (info >> 4) != 0:
            start = strings["offset"] + u32(p)
            end = b.find(b"\0", start)
            callback_exports.add(b[start:end].decode("utf-8", errors="replace"))

required = [
    "unit_init", "unit_teardown", "unit_reset", "unit_resume", "unit_suspend",
    "unit_render", "unit_set_param_value", "unit_get_param_value",
    "unit_get_param_str_value", "unit_note_on", "unit_note_off",
    "unit_all_note_off", "unit_set_tempo", "unit_tempo_4ppqn_tick",
    "unit_pitch_bend", "unit_channel_pressure", "unit_aftertouch"
]

for name in required:
    if name not in callback_exports:
        raise ValueError("Missing callback export: " + name)

result = {
    "package": package_path,
    "packageBytes": len(b),
    "loadBytes": load_bytes,
    "headroomBytes": 49152 - load_bytes,
    "sha256": hashlib.sha256(b).hexdigest(),
    "callbacks": len(required)
}

if manifest_path:
    with open(manifest_path, "w") as f:
        json.dump(result, f, indent=2)
        f.write("\n")

print(json.dumps(result))
