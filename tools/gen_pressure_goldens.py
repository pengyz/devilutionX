#!/usr/bin/env python3
"""Generate test/fixtures/pressure/immutable_tables.fnv64.

The pressure leg must not change the monster numbers, the drop tables or the prices. This list
covers every table that carries one of those, verified against the code paths that read them:
  monstdat / unique_monstdat      -> monster numbers            (tables/monstdat.cpp, monster.cpp)
  itemdat / unique_itemdat        -> drops and prices           (items.cpp: dropRate, UIValue)
  item_prefixes / item_suffixes   -> affix multipliers -> price (items.cpp: multVal -> CalcItemValue)

The digest is FNV-1a over the raw bytes: a change detector for accidental edits, not a
cryptographic digest. Regenerate consciously:

    python3 tools/gen_pressure_goldens.py
"""
import pathlib

TABLES = [
    "assets/txtdata/monsters/monstdat.tsv",
    "mods/hf/txtdata/monsters/monstdat.tsv",
    "assets/txtdata/monsters/unique_monstdat.tsv",
    "assets/txtdata/items/itemdat.tsv",
    "assets/txtdata/items/unique_itemdat.tsv",
    "mods/hf/txtdata/items/unique_itemdat.tsv",
    "assets/txtdata/items/item_prefixes.tsv",
    "assets/txtdata/items/item_suffixes.tsv",
    "mods/hf/txtdata/items/item_prefixes.tsv",
    "mods/hf/txtdata/items/item_suffixes.tsv",
]


def fnv1a64(data: bytes) -> str:
    h = 0xCBF29CE484222325
    for byte in data:
        h ^= byte
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return f"{h:016x}"


def main() -> int:
    lines = []
    for table in TABLES:
        path = pathlib.Path(table)
        if not path.exists():
            print(f"missing table: {table}")
            return 1
        lines.append(f"{fnv1a64(path.read_bytes())}  {table}")
    out = pathlib.Path("test/fixtures/pressure/immutable_tables.fnv64")
    out.parent.mkdir(parents=True, exist_ok=True)
    # CRLF: this suffix is not in the .editorconfig LF exception list.
    out.write_bytes(("\n".join(lines) + "\n").replace("\r\n", "\n").replace("\n", "\r\n").encode())
    print(f"wrote {out} with {len(lines)} tables")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
