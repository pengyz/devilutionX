#!/usr/bin/env python3
"""Compile all .po translation files to .gmo after build.

Usage: python tools/compile_translations.py <build_assets_dir>

Example (from project root after cmake --build):
  python tools/compile_translations.py build_vs18/assets
"""
import polib
import sys
import os
from pathlib import Path

def main():
    if len(sys.argv) < 2:
        print("Usage: python tools/compile_translations.py <build_assets_dir>", file=sys.stderr)
        sys.exit(1)

    build_assets = Path(sys.argv[1])
    translations_dir = Path(__file__).resolve().parent.parent / "Translations"

    if not translations_dir.is_dir():
        print(f"Error: Translations directory not found: {translations_dir}", file=sys.stderr)
        sys.exit(1)

    build_assets.mkdir(parents=True, exist_ok=True)

    compiled = 0
    for po_file in sorted(translations_dir.glob("*.po")):
        lang = po_file.stem  # e.g., "zh_CN"
        gmo_file = build_assets / f"{lang}.gmo"
        try:
            po = polib.pofile(str(po_file))
            po.save_as_mofile(str(gmo_file))
            print(f"  {lang}.po -> {gmo_file.name} ({len(po)} entries)")
            compiled += 1
        except Exception as e:
            print(f"  Error compiling {po_file.name}: {e}", file=sys.stderr)

    print(f"Compiled {compiled} translation file(s) to {build_assets}")

if __name__ == "__main__":
    main()
