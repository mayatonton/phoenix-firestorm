#!/usr/bin/env python3
"""Generate the "busy store" benchmark region as an OpenSim OAR.

The scene mirrors the load of the "busy store" demo bench of a wgpu-based
viewer's macOS GPU investigation: a static mirror facing the plaza, 64 local
lights, 12 lace veils with 12 glowing strings, and a crowd of 40 avatars
each wearing eight 7-prim linksets. The avatar crowd is made of
OpenSim NPCs created in-world by busy_store_crowd.lsl, which the OAR places
in a "bench crowd controller" prim together with the 7-prim linkset the
avatars wear; see docs/guides/ayastorm-r42-opensim-bench.ja.md.

Everything is derived from fixed constants and uuid5 names, so the same
command always produces the same region. Only the Python standard library
and OpenJPEG's opj_compress are needed.

Layout (region coordinates, metres). The bench view stands at START and
looks north (+Y):

  * plaza:   96 x 96 m paved slab centred on the region, top at FLOOR_Z
  * veils:   12 lace curtains 14 x 8 m, one every metre from 2.5 m ahead;
             three in four hang across the view, the rest along it or at
             a slant; 12 glowing strings (80 % opaque) hang among them
  * mirror:  3 x 2 m static mirror (reflection probe, mirror flag) 12 m
             ahead, its centre 2.5 m above the floor, facing the view
  * lights:  64 glowing 6 cm spheres at head height on a 1.6 m grid from
             9 m ahead, each a local light of radius 3 m
  * crowd:   40 NPCs on an 8 x 5 grid from 14 m ahead, facing the view,
             each wearing eight 7-prim linksets (created by the script)

Usage:
  scripts/bench/generate_busy_store_oar.py --out ~/OpenSimBench/busy_store.oar
  scripts/bench/generate_busy_store_oar.py --out no_veils.oar --omit veils
then in the OpenSim console:  load oar <path>
"""

import argparse
import base64
import gzip
import hashlib
import io
import math
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import uuid
import zlib
from pathlib import Path
from xml.sax.saxutils import escape

REGION_SIZE = 256
TERRAIN_Z = 21.5
FLOOR_Z = 22.0
START = (128.0, 96.0)  # bench view position; it looks along +Y
PLAZA_SIZE = 96.0      # centred on the region: the view stands well inside it

NAMESPACE = uuid.UUID("6f1c2a3e-8a7b-4f0e-9c55-0a5b1e7e2d42")
BLANK_TEXTURE = "5748decc-f629-461c-9a36-a35a221fe21f"
OWNER = "00000000-0000-0000-0000-000000000000"  # load oar reassigns ownership

VEIL_COUNT = 12
VEIL_STRINGS = 12
VEIL_GAP = 1.0
VEIL_SIZE = (14.0, 8.0)
LIGHT_COUNT = 64
LIGHT_SPACING = 1.6
LIGHT_RADIUS = 3.0
MIRROR_SIZE = (3.0, 2.0)
MIRROR_AHEAD = 12.0
MIRROR_HEIGHT = 2.5

# Reflection probe extra parameter (llprimitive.h: LLReflectionProbeParams)
EP_LIGHT = 0x20
EP_REFLECTION_PROBE = 0x90
PROBE_FLAG_MIRROR = 0x04

TE_FULLBRIGHT = 0x20


def uid(name):
    return str(uuid.uuid5(NAMESPACE, name))


def asset_uid(data):
    """Asset ids follow the content: OpenSim keeps the first asset stored
    under an id and skips later uploads of the same id, so a changed script
    or texture must get a new id."""
    return uid("asset/" + hashlib.sha256(data).hexdigest())


# ---------------------------------------------------------------- textures

def png_bytes(width, height, rows, alpha):
    """Encode rows (lists of RGB or RGBA tuples) as a PNG."""
    channels = 4 if alpha else 3
    raw = bytearray()
    for row in rows:
        raw.append(0)
        for px in row:
            raw.extend(px[:channels])

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6 if alpha else 2, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def plaza_texture(size=512):
    """Light stone pavers with darker joints, 4 x 4 tiles per texture."""
    tile = size // 4
    rows = []
    for y in range(size):
        row = []
        for x in range(size):
            tx, ty = x // tile, y // tile
            joint = (x % tile) < 3 or (y % tile) < 3
            if joint:
                v = 92
            else:
                # deterministic per-tile tone and fine grain
                v = 178 + ((tx * 37 + ty * 59) % 23) - 11 + ((x * 7 + y * 13) % 9) - 4
            row.append((v, v - 4, v - 12))
        rows.append(row)
    return png_bytes(size, size, rows, alpha=False)


def lace_texture(size=512):
    """Lace: opaque threads, a net about 38 % opaque, clear holes."""
    cell = size // 16
    rows = []
    for y in range(size):
        row = []
        for x in range(size):
            cx, cy = (x % cell) - cell / 2 + 0.5, (y % cell) - cell / 2 + 0.5
            r = math.hypot(cx, cy) / (cell / 2)
            thread = (x % cell) < 2 or (y % cell) < 2 or abs(r - 0.78) < 0.06
            if thread:
                a = 255
            elif r < 0.55:
                a = 0          # hole
            else:
                a = 97         # net, 38 %
            row.append((255, 255, 255, a))
        rows.append(row)
    return png_bytes(size, size, rows, alpha=True)


def encode_j2k(png, opj, workdir, name):
    src = workdir / f"{name}.png"
    dst = workdir / f"{name}.j2k"
    src.write_bytes(png)
    subprocess.run([opj, "-i", str(src), "-o", str(dst), "-n", "6"],
                   check=True, stdout=subprocess.DEVNULL)
    return dst.read_bytes()


# ------------------------------------------------------------- primitives

def texture_entry(texture, color=(1, 1, 1, 1), repeats=(1.0, 1.0),
                  fullbright=False, glow=0.0):
    """LLTextureEntry wire format with one default for all faces."""
    out = bytearray()
    out += uuid.UUID(texture).bytes + b"\x00"
    out += bytes(255 - int(round(c * 255)) for c in color) + b"\x00"
    out += struct.pack("<f", repeats[0]) + b"\x00"
    out += struct.pack("<f", repeats[1]) + b"\x00"
    out += struct.pack("<h", 0) + b"\x00"  # offset S
    out += struct.pack("<h", 0) + b"\x00"  # offset T
    out += struct.pack("<h", 0) + b"\x00"  # rotation
    out += bytes([TE_FULLBRIGHT if fullbright else 0]) + b"\x00"
    out += b"\x00" + b"\x00"  # media / texgen
    out += bytes([int(round(glow * 255))]) + b"\x00"
    out += uuid.UUID(int=0).bytes + b"\x00"  # material id
    return bytes(out)


def extra_params(light=None, probe_flags=None):
    params = []
    if light is not None:
        (r, g, b), intensity, radius, falloff = light
        data = (bytes(int(round(c * 255)) for c in (r, g, b, intensity))
                + struct.pack("<fff", radius, 0.0, falloff))
        params.append((EP_LIGHT, data))
    if probe_flags is not None:
        params.append((EP_REFLECTION_PROBE, struct.pack("<ffB", 0.0, 0.0, probe_flags)))
    out = bytearray([len(params)])
    for kind, data in params:
        out += struct.pack("<HI", kind, len(data)) + data
    return bytes(out)


SHAPES = {
    # pcode 9 volumes: (ProfileCurve, PathCurve, ProfileShape name)
    "box": (1, 16, "Square"),
    "cylinder": (0, 16, "Circle"),
    "sphere": (5, 32, "HalfCircle"),
}


def quat_axis(axis, degrees):
    h = math.radians(degrees) / 2
    s = math.sin(h)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(h))


def vec(tag, v):
    return f"<{tag}><X>{v[0]:.4f}</X><Y>{v[1]:.4f}</Y><Z>{v[2]:.4f}</Z></{tag}>"


PERM_ALL = 2147483647
INV_TYPES = {"object": 6, "script": 10}  # asset type == inventory type for these


def task_inventory_xml(part_id, items):
    """items: (name, asset_id, kind) placed in the part's contents."""
    if not items:
        return ""
    out = ["<TaskInventory>"]
    for name, asset_id, kind in items:
        item_id = uid(f"item/{part_id}/{name}")
        t = INV_TYPES[kind]
        out.append(
            f"<TaskInventoryItem><AssetID><UUID>{asset_id}</UUID></AssetID>"
            f"<BasePermissions>{PERM_ALL}</BasePermissions><CreationDate>0</CreationDate>"
            f"<CreatorID><UUID>{OWNER}</UUID></CreatorID><Description />"
            f"<EveryonePermissions>0</EveryonePermissions><Flags>0</Flags>"
            f"<GroupID><UUID>{OWNER}</UUID></GroupID><GroupPermissions>0</GroupPermissions>"
            f"<InvType>{t}</InvType><ItemID><UUID>{item_id}</UUID></ItemID>"
            f"<OldItemID><UUID>{OWNER}</UUID></OldItemID>"
            f"<LastOwnerID><UUID>{OWNER}</UUID></LastOwnerID><Name>{escape(name)}</Name>"
            f"<NextPermissions>{PERM_ALL}</NextPermissions><OwnerID><UUID>{OWNER}</UUID></OwnerID>"
            f"<CurrentPermissions>{PERM_ALL}</CurrentPermissions>"
            f"<ParentID><UUID>{part_id}</UUID></ParentID>"
            f"<ParentPartID><UUID>{part_id}</UUID></ParentPartID>"
            f"<PermsGranter><UUID>{OWNER}</UUID></PermsGranter><PermsMask>0</PermsMask>"
            f"<Type>{t}</Type><OwnerChanged>false</OwnerChanged></TaskInventoryItem>")
    out.append("</TaskInventory>")
    return "".join(out)


def part_xml(name, shape, pos, scale, te, rot=(0, 0, 0, 1), flags="None",
             light=None, probe_flags=None, offset=(0, 0, 0), link_num=0,
             inventory=None, part_id=None):
    """One <SceneObjectPart>. pos is the group position; offset is relative
    to the root for linked children."""
    part_id = part_id or uid("part/" + name)
    profile_curve, path_curve, profile_shape = SHAPES[shape]
    ep = extra_params(light, probe_flags)
    light_xml = ""
    if light is not None:
        (r, g, b), intensity, radius, falloff = light
        light_xml = (f"<LightColorR>{r}</LightColorR><LightColorG>{g}</LightColorG>"
                     f"<LightColorB>{b}</LightColorB><LightColorA>1</LightColorA>"
                     f"<LightRadius>{radius}</LightRadius><LightCutoff>0</LightCutoff>"
                     f"<LightFalloff>{falloff}</LightFalloff>"
                     f"<LightIntensity>{intensity}</LightIntensity><LightEntry>true</LightEntry>")
    return f"""<SceneObjectPart xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
<AllowedDrop>false</AllowedDrop><CreatorID><UUID>{OWNER}</UUID></CreatorID>
<FolderID><UUID>{part_id}</UUID></FolderID><InventorySerial>{1 if inventory else 0}</InventorySerial>
{task_inventory_xml(part_id, inventory)}
<UUID><UUID>{part_id}</UUID></UUID><LocalId>0</LocalId><Name>{escape(name)}</Name>
<Material>3</Material><RegionHandle>0</RegionHandle>
{vec("GroupPosition", pos)}{vec("OffsetPosition", offset)}
<RotationOffset><X>{rot[0]:.6f}</X><Y>{rot[1]:.6f}</Y><Z>{rot[2]:.6f}</Z><W>{rot[3]:.6f}</W></RotationOffset>
<Description>AYAstorm busy store bench</Description><LinkNum>{link_num}</LinkNum>
<Shape><ProfileCurve>{profile_curve}</ProfileCurve>
<TextureEntry>{base64.b64encode(te).decode()}</TextureEntry>
<ExtraParams>{base64.b64encode(ep).decode()}</ExtraParams>
<PathBegin>0</PathBegin><PathCurve>{path_curve}</PathCurve><PathEnd>0</PathEnd>
<PathRadiusOffset>0</PathRadiusOffset><PathRevolutions>0</PathRevolutions>
<PathScaleX>100</PathScaleX><PathScaleY>100</PathScaleY><PathShearX>0</PathShearX>
<PathShearY>0</PathShearY><PathSkew>0</PathSkew><PathTaperX>0</PathTaperX>
<PathTaperY>0</PathTaperY><PathTwist>0</PathTwist><PathTwistBegin>0</PathTwistBegin>
<PCode>9</PCode><ProfileBegin>0</ProfileBegin><ProfileEnd>0</ProfileEnd>
<ProfileHollow>0</ProfileHollow><State>0</State><ProfileShape>{profile_shape}</ProfileShape>
<HollowShape>Same</HollowShape><SculptType>0</SculptType>{light_xml}</Shape>
{vec("Scale", scale)}<ParentID>0</ParentID><CreationDate>0</CreationDate>
<GroupID><UUID>{OWNER}</UUID></GroupID><OwnerID><UUID>{OWNER}</UUID></OwnerID>
<LastOwnerID><UUID>{OWNER}</UUID></LastOwnerID>
<BaseMask>2147483647</BaseMask><OwnerMask>2147483647</OwnerMask><GroupMask>0</GroupMask>
<EveryoneMask>0</EveryoneMask><NextOwnerMask>2147483647</NextOwnerMask>
<Flags>{flags}</Flags></SceneObjectPart>"""


def prim_xml(name, shape, pos, scale, te, **kw):
    """A single-prim region object in the OAR's XML2 format: (name, xml)."""
    return name, (f"<SceneObjectGroup><RootPart>{part_xml(name, shape, pos, scale, te, **kw)}"
                  f"</RootPart><OtherParts /></SceneObjectGroup>\n")


def linkset_original_xml(parts):
    """An object asset (inventory item) in the original XML format that
    rezzing and attaching from a prim's contents read
    (SceneObjectSerializer.FromOriginalXmlFormat)."""
    root, children = parts[0], parts[1:]
    others = "".join(f"<Part>{p}</Part>" for p in children)
    return (f"<SceneObjectGroup><RootPart>{root}</RootPart>"
            f"<OtherParts>{others}</OtherParts></SceneObjectGroup>")


# ----------------------------------------------------------------- scene

TINTS = [(1.0, 1.0, 1.0), (0.95, 0.72, 0.80), (0.62, 0.86, 0.90), (0.98, 0.84, 0.55)]
LIGHT_COLORS = [(1.0, 0.85, 0.6), (0.6, 0.8, 1.0), (1.0, 0.5, 0.7), (0.7, 1.0, 0.7)]


CROWD_SCRIPT = Path(__file__).with_name("busy_store_crowd.lsl")
WEARABLE_COLORS = [(0.85, 0.2, 0.25), (0.95, 0.75, 0.2), (0.25, 0.55, 0.9), (0.9, 0.9, 0.95)]


def wearable_parts(lace_tex):
    """The 7-prim linkset each crowd avatar wears eight times."""
    parts = [part_xml("bench wearable", "box", (0, 0, 0), (0.08, 0.08, 0.08),
                      texture_entry(lace_tex, color=WEARABLE_COLORS[0] + (1.0,)),
                      link_num=1, part_id=uid("part/bench wearable/0"))]
    offsets = [(0.07, 0, 0), (-0.07, 0, 0), (0, 0.07, 0), (0, -0.07, 0), (0, 0, 0.07), (0, 0, -0.07)]
    for i, off in enumerate(offsets, start=1):
        parts.append(part_xml(
            f"bench wearable {i}", "sphere", (0, 0, 0), (0.05, 0.05, 0.05),
            texture_entry(BLANK_TEXTURE, color=WEARABLE_COLORS[i % 4] + (1.0,), glow=0.05),
            offset=off, link_num=i + 1, part_id=uid(f"part/bench wearable/{i}")))
    return parts


def build_scene(plaza_tex, lace_tex):
    """Returns the region objects [(name, xml)] and the extra assets
    {archive path: bytes} that the objects' contents refer to."""
    sx, sy = START
    prims = []
    assets = {}

    prims.append(prim_xml(
        "bench plaza", "box", (128.0, 128.0, FLOOR_Z - 0.25), (PLAZA_SIZE, PLAZA_SIZE, 0.5),
        texture_entry(plaza_tex, repeats=(PLAZA_SIZE / 8.0, PLAZA_SIZE / 8.0))))

    for i in range(VEIL_COUNT):
        y = sy + 2.5 + i * VEIL_GAP
        kind = i % 4
        if kind != 3:
            rot = (0, 0, 0, 1)                       # across the view
        elif (i // 4) % 2 == 0:
            rot = quat_axis((0, 0, 1), 90)           # along the view
        else:
            rot = quat_axis((0, 0, 1), 35)           # at a slant
        prims.append(prim_xml(
            f"bench veil {i:02d}", "box", (sx, y, FLOOR_Z + VEIL_SIZE[1] / 2),
            (VEIL_SIZE[0], 0.01, VEIL_SIZE[1]),
            texture_entry(lace_tex, color=TINTS[i % 4] + (1.0,), repeats=(3.5, 2.0)),
            rot=rot, flags="Phantom"))

    for i in range(VEIL_STRINGS):
        x = sx - VEIL_SIZE[0] / 2 + 0.6 + (i * 5 % VEIL_STRINGS) * (VEIL_SIZE[0] - 1.2) / (VEIL_STRINGS - 1)
        y = sy + 3.0 + i * (VEIL_COUNT * VEIL_GAP - 1.0) / VEIL_STRINGS
        prims.append(prim_xml(
            f"bench string {i:02d}", "cylinder", (x, y, FLOOR_Z + 4.5), (0.03, 0.03, 7.0),
            texture_entry(BLANK_TEXTURE, color=LIGHT_COLORS[i % 4] + (0.8,),
                          fullbright=True, glow=0.25),
            flags="Phantom"))

    # local +Z is the mirror normal (llheroprobemanager.cpp): turn it to -Y
    prims.append(prim_xml(
        "bench mirror", "box", (sx, sy + MIRROR_AHEAD, FLOOR_Z + MIRROR_HEIGHT),
        (MIRROR_SIZE[0], MIRROR_SIZE[1], 0.05),
        texture_entry(BLANK_TEXTURE, color=(0.08, 0.08, 0.09, 1.0)),
        rot=quat_axis((1, 0, 0), 90), probe_flags=PROBE_FLAG_MIRROR))

    side = int(math.isqrt(LIGHT_COUNT))
    for i in range(LIGHT_COUNT):
        gx, gy = i % side, i // side
        x = sx + (gx - (side - 1) / 2) * LIGHT_SPACING
        y = sy + 9.0 + gy * LIGHT_SPACING
        color = LIGHT_COLORS[i % 4]
        prims.append(prim_xml(
            f"bench light {i:02d}", "sphere", (x, y, FLOOR_Z + 1.6), (0.06, 0.06, 0.06),
            texture_entry(BLANK_TEXTURE, color=color + (1.0,), fullbright=True, glow=0.4),
            flags="Phantom", light=(color, 1.0, LIGHT_RADIUS, 0.75)))

    # crowd controller: creates the NPC crowd (busy_store_crowd.lsl), out of
    # the bench view, behind and to the side of the start position
    script = CROWD_SCRIPT.read_bytes()
    wearable = linkset_original_xml(wearable_parts(lace_tex)).encode()
    script_id, wearable_id = asset_uid(script), asset_uid(wearable)
    assets[f"assets/{script_id}_script.lsl"] = script
    assets[f"assets/{wearable_id}_object.xml"] = wearable
    prims.append(prim_xml(
        "bench crowd controller", "box", (sx + 12.0, sy - 8.0, FLOOR_Z + 0.25), (0.5, 0.5, 0.5),
        texture_entry(BLANK_TEXTURE, color=(0.3, 0.3, 0.35, 1.0)),
        inventory=[("bench crowd", script_id, "script"),
                   ("bench wearable", wearable_id, "object")]))
    return prims, assets


# --omit parts and the object name prefixes they remove
PARTS = {
    "veils": ("bench veil", "bench string"),
    "mirror": ("bench mirror",),
    "lights": ("bench light",),
    "crowd": ("bench crowd controller",),
}


def control_file():
    return (f'<?xml version="1.0" encoding="utf-16"?>'
            f'<archive major_version="1" minor_version="0">'
            f'<creation_info><datetime>0</datetime>'
            f'<id>{uid("archive")}</id></creation_info>'
            f'<assets_included>True</assets_included>'
            f'<region_info><is_megaregion>False</is_megaregion>'
            f'<size_in_meters>{REGION_SIZE},{REGION_SIZE}</size_in_meters></region_info>'
            f'</archive>').encode()


def add(tar, path, data):
    info = tarfile.TarInfo(path)
    info.size = len(data)
    info.mtime = 0
    tar.addfile(info, io.BytesIO(data))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", required=True, type=Path, help="OAR file to write")
    ap.add_argument("--opj", default=shutil.which("opj_compress") or "opj_compress",
                    help="path to OpenJPEG's opj_compress")
    ap.add_argument("--omit", type=lambda s: [p for p in s.split(",") if p], default=[],
                    help="comma-separated parts to leave out, for A/B runs: "
                         + ", ".join(PARTS))
    args = ap.parse_args()
    unknown = [p for p in args.omit if p not in PARTS]
    if unknown:
        ap.error(f"unknown --omit part(s): {', '.join(unknown)}")

    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        plaza = encode_j2k(plaza_texture(), args.opj, work, "plaza")
        lace = encode_j2k(lace_texture(), args.opj, work, "lace")
    plaza_tex, lace_tex = asset_uid(plaza), asset_uid(lace)
    textures = {plaza_tex: plaza, lace_tex: lace}

    prims, assets = build_scene(plaza_tex, lace_tex)
    if args.omit:
        prims = [(n, x) for n, x in prims
                 if not any(n.startswith(p) for part in args.omit for p in PARTS[part])]
        if "crowd" in args.omit:
            assets = {}
    terrain = struct.pack(f"<{REGION_SIZE * REGION_SIZE}f",
                          *([TERRAIN_Z] * (REGION_SIZE * REGION_SIZE)))

    args.out.parent.mkdir(parents=True, exist_ok=True)
    # fixed gzip mtime and tar entry times keep the archive byte-identical
    with open(args.out, "wb") as raw, \
            gzip.GzipFile(fileobj=raw, mode="wb", mtime=0, filename="") as gz, \
            tarfile.open(fileobj=gz, mode="w") as tar:
        add(tar, "archive.xml", control_file())
        for tex_id, data in textures.items():
            add(tar, f"assets/{tex_id}_texture.jp2", data)
        for path, data in assets.items():
            add(tar, path, data)
        add(tar, "terrains/BenchRegion.r32", terrain)
        for name, xml in prims:
            add(tar, f"objects/{name.replace(' ', '_')}__{uid('part/' + name)}.xml", xml.encode())
    print(f"wrote {args.out}: {len(prims)} objects, "
          f"{len(textures) + len(assets)} assets")
    return 0


if __name__ == "__main__":
    sys.exit(main())
