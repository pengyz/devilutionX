#!/usr/bin/env python3
"""Resolve conflict blocks by keeping the HEAD side, leaving the rest untouched.

Only touches lines between conflict markers, so content git already
auto-merged elsewhere in the file is preserved.
"""
import sys


def resolve(text: str) -> tuple[str, int]:
    out = []
    blocks = 0
    state = 'normal'  # normal | ours | theirs
    for line in text.split('\n'):
        if state == 'normal':
            if line.startswith('<<<<<<< '):
                state = 'ours'
                blocks += 1
                continue
            out.append(line)
        elif state == 'ours':
            if line.startswith('======='):
                state = 'theirs'
                continue
            if line.startswith('>>>>>>> '):
                state = 'normal'
                continue
            out.append(line)
        else:  # theirs -- discard
            if line.startswith('>>>>>>> '):
                state = 'normal'
            continue
    if state != 'normal':
        raise ValueError('unterminated conflict block')
    return '\n'.join(out), blocks


def main(paths: list[str]) -> int:
    for path in paths:
        with open(path, 'rb') as f:
            raw = f.read().decode('utf-8')
        crlf = '\r\n' in raw
        text = raw.replace('\r\n', '\n') if crlf else raw
        text, blocks = resolve(text)
        out = text.replace('\n', '\r\n') if crlf else text
        with open(path, 'wb') as f:
            f.write(out.encode('utf-8'))
        print(f'{path}: kept HEAD side of {blocks} block(s)')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
