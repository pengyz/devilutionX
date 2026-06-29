"""
po2gmo.py - Convert GNU gettext .po file to GNU .mo/.gmo binary format.
The format matches what DevilutionX's language.cpp expects:
  MoHead: magic(4) + revision(2+2) + nbMappings(4) + srcOffset(4) + dstOffset(4) = 16 bytes
  Then nbMappings x MoEntry (8 bytes each: length(4) + offset(4)) for original strings
  Then nbMappings x MoEntry for translated strings
  Then actual string data (originals then translations, contiguous)
Entries are sorted by original string content for binary search.
The first entry (index 0) is always the metadata header (empty msgid).

Usage: python po2gmo.py Translations/zh_CN.po build_debug/zh_CN.gmo
"""
import struct, sys

def parse_po(path):
    entries = []
    msgctxt = None
    msgid = None
    msgstr = None
    in_msgid = False
    in_msgstr = False
    in_msgctxt = False

    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            stripped = line.strip()

            if stripped.startswith('#~'):
                msgctxt = None; msgid = None; msgstr = None
                in_msgid = False; in_msgstr = False; in_msgctxt = False
                continue
            if stripped.startswith('#') or stripped == '':
                if stripped == '' and msgid is not None:
                    full_msgid = msgid
                    if msgctxt:
                        full_msgid = msgctxt + '\x04' + msgid
                    if msgstr is not None:
                        entries.append((full_msgid, msgstr))
                    msgctxt = None; msgid = None; msgstr = None
                    in_msgid = False; in_msgstr = False; in_msgctxt = False
                continue

            if stripped.startswith('msgctxt "'):
                msgctxt = stripped[9:-1]
                in_msgctxt = True; in_msgid = False; in_msgstr = False
            elif stripped.startswith('msgid "'):
                msgid = stripped[7:-1]
                in_msgid = True; in_msgstr = False; in_msgctxt = False
            elif stripped.startswith('msgstr "'):
                msgstr = stripped[8:-1]
                in_msgstr = True; in_msgid = False; in_msgctxt = False
            elif stripped.startswith('"') and in_msgid:
                msgid += stripped[1:-1]
            elif stripped.startswith('"') and in_msgstr:
                msgstr += stripped[1:-1]
            elif stripped.startswith('"') and in_msgctxt:
                msgctxt += stripped[1:-1]

    if msgid is not None and msgstr is not None:
        full_msgid = msgid
        if msgctxt:
            full_msgid = msgctxt + '\x04' + msgid
        entries.append((full_msgid, msgstr))
    return entries


def build_gmo(entries, outpath):
    all_entries = entries[:]
    sorted_entries = sorted(all_entries, key=lambda e: e[0])
    num = len(sorted_entries)

    header_size = 16
    entry_size = 8
    src_table_size = num * entry_size
    dst_table_size = num * entry_size

    orig_strings = [e[0].encode('utf-8') for e in sorted_entries]
    trans_strings = [e[1].encode('utf-8') for e in sorted_entries]

    src_table_offset = header_size
    dst_table_offset = src_table_offset + src_table_size
    data_offset = dst_table_offset + dst_table_size

    src_entries = []
    off = data_offset
    for s in orig_strings:
        src_entries.append((len(s), off))
        off += len(s)

    dst_entries = []
    for s in trans_strings:
        dst_entries.append((len(s), off))
        off += len(s)

    with open(outpath, 'wb') as f:
        f.write(struct.pack('<I', 0x950412de))
        f.write(struct.pack('<H', 0))
        f.write(struct.pack('<H', 0))
        f.write(struct.pack('<I', num))
        f.write(struct.pack('<I', src_table_offset))
        f.write(struct.pack('<I', dst_table_offset))

        for length, offset in src_entries:
            f.write(struct.pack('<I', length))
            f.write(struct.pack('<I', offset))

        for length, offset in dst_entries:
            f.write(struct.pack('<I', length))
            f.write(struct.pack('<I', offset))

        for s in orig_strings:
            f.write(s)
        for s in trans_strings:
            f.write(s)

    import os
    print(f'{outpath}: {num} strings, {os.path.getsize(outpath)} bytes')


if __name__ == '__main__':
    po_path = sys.argv[1]
    out_path = sys.argv[2]
    entries = parse_po(po_path)
    print(f'Parsed {len(entries)} entries')
    build_gmo(entries, out_path)
