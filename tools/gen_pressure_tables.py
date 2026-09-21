#!/usr/bin/env python3
"""Generate (or check) the pressure-shape golden tables.

Plain text, so the C++ guards need no JSON parser.

Outputs under test/fixtures/pressure/:
  segment_types.txt   band<TAB>comma-separated demand types
  roster_ais.txt      every AI that appears as a roster core (one per line)
  emission_sites.txt  enclosing function<TAB>comma-separated literal MissileIDs it emits

Sources of truth (scanned, never hand-copied):
  assets/txtdata/monsters/level_rosters.tsv   roster cores per level
  mods/hf/txtdata/monsters/monstdat.tsv       monster -> ai
  Source/monster.cpp                          AiProc, GetMissileType cases, direct emitters
  mods/hf/txtdata/missiles/misdat.tsv         missile -> damage type flags

Damage types live on missiles while monsters only name an AI, so an AI's types are the union of
the generic table and whatever its AI function emits directly (BatAi fires Lightning, MegaAi
fires InfernoControl, CounselorAi fires its four-missile array).

    python3 tools/gen_pressure_tables.py           # write the tables
    python3 tools/gen_pressure_tables.py --check   # fail if the committed tables are stale
"""
import csv
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
ROSTER = "assets/txtdata/monsters/level_rosters.tsv"
MONSTDAT = "mods/hf/txtdata/monsters/monstdat.tsv"
MISDAT = "mods/hf/txtdata/missiles/misdat.tsv"
MONSTER_CPP = "Source/monster.cpp"
OUT = pathlib.Path("test/fixtures/pressure")
TYPES = ("Fire", "Lightning", "Magic", "Acid", "Physical")


def band(level: int) -> str:
    if level <= 8:
        return "L1-8"
    if level <= 12:
        return "L9-12"
    if level <= 16:
        return "L13-16"
    return "L17-24"


def missile_flags() -> dict[str, str]:
    flags = {}
    for row in list(csv.reader(open(ROOT / MISDAT, encoding="utf-8"), delimiter="\t"))[1:]:
        if len(row) > 6 and row[0]:
            flags[row[0]] = row[6]
    return flags


def generic_table(src: str) -> dict[str, str]:
    block = re.search(r"MissileID GetMissileType\(MonsterAIID ai\)\s*\{(.*?)\n\}", src, re.S)
    table, pending = {}, []
    for line in (block.group(1).splitlines() if block else []):
        case = re.search(r"case MonsterAIID::(\w+)", line)
        if case:
            pending.append(case.group(1))
            continue
        ret = re.search(r"return MissileID::(\w+)", line)
        if ret:
            for ai in pending:
                table[ai] = ret.group(1)
            pending = []
    return table


def ai_functions(src: str) -> dict[str, str]:
    mapping = {}
    for ai, fn in re.findall(r"/\*MonsterAIID::(\w+)\s*\*/\s*&(\w+)", src):
        mapping[ai] = fn
    return mapping


def counselor_array(src: str) -> list[str]:
    block = re.search(r"MissileID MissileTypes\[4\]\s*=\s*\{([^}]*)\}", src, re.S)
    return re.findall(r"MissileID::(\w+)", block.group(1)) if block else []


def emission_sites(src: str) -> dict[str, set[str]]:
    current, sites = None, {}
    for line in src.splitlines():
        fn = re.match(r"void (\w+)\(Monster &monster\)", line)
        if fn:
            current = fn.group(1)
        if current is None:
            continue
        for call in re.finditer(r"(?:AddMissile|StartRangedAttack|StartRangedSpecialAttack)\([^;]*?MissileID::(\w+)", line):
            sites.setdefault(current, set()).add(call.group(1))
    return sites


def build_tables() -> dict[str, list[str]]:
    src = (ROOT / MONSTER_CPP).read_text(encoding="utf-8", errors="replace")
    flags = missile_flags()
    generic = generic_table(src)
    counselor = counselor_array(src)
    sites = emission_sites(src)
    ai_to_fn = ai_functions(src)

    mh, *mrows = list(csv.reader(open(ROOT / MONSTDAT, encoding="utf-8"), delimiter="\t"))
    mi = {c: i for i, c in enumerate(mh)}
    ai_of = {r[mi["_monster_id"]]: r[mi["ai"]] for r in mrows if len(r) >= len(mh)}

    rh, *rrows = list(csv.reader(open(ROOT / ROSTER, encoding="utf-8"), delimiter="\t"))
    ri = {c: i for i, c in enumerate(rh)}

    per_band, roster_ais = {}, set()
    for row in rrows:
        if len(row) <= ri["role"] or row[ri["role"]] != "core":
            continue
        ai = ai_of.get(row[ri["monster_id"]])
        if not ai:
            continue
        roster_ais.add(ai)
        missiles = {generic[ai]} if ai in generic else set()
        missiles |= sites.get(ai_to_fn.get(ai, ""), set())
        if ai == "Counselor":
            missiles |= set(counselor)
        found = set()
        for missile in missiles:
            for t in TYPES:
                if t in flags.get(missile, ""):
                    found.add(t)
        if not found:
            found.add("Physical")  # melee: answered by armour (ACP)
        per_band.setdefault(band(int(row[ri["level"]])), set()).update(found)

    return {
        "segment_types.txt": [f"{b}\t{','.join(sorted(per_band[b]))}" for b in sorted(per_band)],
        "roster_ais.txt": sorted(roster_ais),
        "emission_sites.txt": [f"{fn}\t{','.join(sorted(ids))}" for fn, ids in sorted(sites.items())],
    }


def payload(lines: list[str]) -> bytes:
    # CRLF: this suffix is not in the .editorconfig LF exception list.
    return ("\n".join(lines) + "\n").replace("\r\n", "\n").replace("\n", "\r\n").encode()


def main(argv: list[str]) -> int:
    tables = build_tables()
    if "--check" in argv:
        stale = 0
        for name, lines in tables.items():
            path = OUT / name
            actual = path.read_bytes() if path.exists() else b""
            if actual != payload(lines):
                print(f"STALE: {path} differs from a fresh scan; run tools/gen_pressure_tables.py")
                stale += 1
        print("pressure tables: " + ("OK" if stale == 0 else f"{stale} stale"))
        return 1 if stale else 0
    OUT.mkdir(parents=True, exist_ok=True)
    for name, lines in tables.items():
        (OUT / name).write_bytes(payload(lines))
        print(f"wrote {OUT / name} ({len(lines)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
