#!/usr/bin/env python3
"""Assemble .raw + .pal into a PNG image."""
import sys, struct
from PIL import Image

def assemble(raw_path, pal_path, out_path, width, height):
    pixels = open(raw_path, 'rb').read()
    pal_data = open(pal_path, 'rb').read()
    palette = [(pal_data[i], pal_data[i+1], pal_data[i+2]) for i in range(0, len(pal_data), 3)]
    img = Image.new('RGB', (width, height))
    px = img.load()
    for y in range(height):
        for x in range(width):
            idx = pixels[y * width + x]
            px[x, y] = palette[idx] if idx < len(palette) else (255, 0, 255)
    img.save(out_path)
    print(f'{out_path}: {width}x{height}')

if __name__ == '__main__':
    if len(sys.argv) < 5:
        print(f'Usage: {sys.argv[0]} <input.raw> <input.pal> <output.png> <width> <height>')
        sys.exit(1)
    assemble(sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
