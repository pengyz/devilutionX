#!/usr/bin/env python3
"""Compile a .po translation file to .gmo format using polib."""
import polib
import sys

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} <input.po> <output.gmo>", file=sys.stderr)
    sys.exit(1)

po = polib.pofile(sys.argv[1])
po.save_as_mofile(sys.argv[2])
