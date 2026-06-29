"""
po2gmo.py - Convert GNU gettext .po file to .gmo binary format.
Handles msgid, msgstr, and msgctxt (by prepending context + EOT byte to msgid).
Usage: python po2gmo.py Translations/zh_CN.po build_debug/assets/zh_CN.gmo
"""
import struct, sys

def parse_po(path):
    """Parse a .po file, returning list of (msgid, msgstr) pairs.
    msgctxt is handled by prepending "context\x04" to msgid."""
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
            
            if stripped.startswith('#') or stripped == '':
                if stripped == '' and msgid is not None:
                    # End of an entry
                    full_msgid = msgid
                    if msgctxt:
                        full_msgid = msgctxt + '\x04' + msgid
                    if msgstr is not None and msgstr:
                        entries.append((full_msgid, msgstr))
                    msgctxt = None
                    msgid = None
                    msgstr = None
                    in_msgid = False
                    in_msgstr = False
                    in_msgctxt = False
                continue
            
            if stripped.startswith('msgctxt "'):
                msgctxt = stripped[9:-1]
                in_msgctxt = True
                in_msgid = False
                in_msgstr = False
            elif stripped.startswith('msgid "'):
                msgid = stripped[7:-1]
                in_msgid = True
                in_msgstr = False
                in_msgctxt = False
            elif stripped.startswith('msgstr "'):
                msgstr = stripped[8:-1]
                in_msgstr = True
                in_msgid = False
                in_msgctxt = False
            elif stripped.startswith('"') and in_msgid:
                msgid += stripped[1:-1]
            elif stripped.startswith('"') and in_msgstr:
                msgstr += stripped[1:-1]
            elif stripped.startswith('"') and in_msgctxt:
                msgctxt += stripped[1:-1]
    
    # Don't forget the last entry
    if msgid is not None and msgstr is not None and msgstr:
        full_msgid = msgid
        if msgctxt:
            full_msgid = msgctxt + '\x04' + msgid
        entries.append((full_msgid, msgstr))
    
    return entries


def build_gmo(entries, outpath):
    """Build a .gmo binary file from parsed entries."""
    num = len(entries)
    header_size = 28  # 7 x uint32
    
    # Compute offsets
    orig_offset = header_size
    offset = orig_offset
    orig_data = []
    for msgid, msgstr in entries:
        data = msgid.encode('utf-8')
        orig_data.append(data)
        offset += 4 + len(data)
    
    trans_offset = offset
    trans_data = []
    for msgid, msgstr in entries:
        data = msgstr.encode('utf-8')
        trans_data.append(data)
        offset += 4 + len(data)
    
    with open(outpath, 'wb') as f:
        # Header
        f.write(struct.pack('<I', 0x950412de))  # magic
        f.write(struct.pack('<I', 0))             # revision
        f.write(struct.pack('<I', num))           # num strings
        f.write(struct.pack('<I', orig_offset))   # original table offset
        f.write(struct.pack('<I', trans_offset))  # translation table offset
        f.write(struct.pack('<I', 0))             # hash table size (0 = no hash)
        f.write(struct.pack('<I', 0))             # hash table offset
        
        # Original strings table
        for data in orig_data:
            f.write(struct.pack('<I', len(data)))
            f.write(data)
        
        # Translated strings table  
        for data in trans_data:
            f.write(struct.pack('<I', len(data)))
            f.write(data)
    
    import os
    print(f'Wrote {outpath}: {num} strings, {os.path.getsize(outpath)} bytes')


if __name__ == '__main__':
    po_path = sys.argv[1]
    out_path = sys.argv[2]
    entries = parse_po(po_path)
    print(f'Parsed {len(entries)} translated entries from {po_path}')
    build_gmo(entries, out_path)
