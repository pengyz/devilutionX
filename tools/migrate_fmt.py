#!/usr/bin/env python3
"""Rewrite fmt::format(fmt::runtime(X), ...) as FormatRuntime(X, ...).

Paren-balanced, so format strings containing calls such as
ngettext("a", "b", n) are handled correctly. Also rewrites the plain
fmt::format(...) -> std::format(...) case.
"""
import sys

NEEDLE = 'fmt::format(fmt::runtime('


def close_paren(text: str, open_idx: int) -> int:
    """Index of the ')' matching the '(' at open_idx, skipping string literals."""
    depth = 0
    i = open_idx
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"':
            i += 1
            while i < n:
                if text[i] == '\\':
                    i += 2
                    continue
                if text[i] == '"':
                    break
                i += 1
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError(f'unbalanced parens starting at {open_idx}')


def migrate_runtime(text: str) -> tuple[str, int]:
    count = 0
    while True:
        at = text.find(NEEDLE)
        if at < 0:
            return text, count
        runtime_open = at + len('fmt::format(fmt::runtime')
        runtime_close = close_paren(text, runtime_open)
        inner = text[runtime_open + 1:runtime_close]
        rest = text[runtime_close + 1:]
        text = text[:at] + 'FormatRuntime(' + inner + rest
        count += 1


def migrate_plain(text: str) -> tuple[str, int]:
    count = text.count('fmt::format(')
    return text.replace('fmt::format(', 'std::format('), count


def main(paths: list[str]) -> int:
    total_rt = total_plain = 0
    for path in paths:
        with open(path, 'rb') as f:
            raw = f.read().decode('utf-8')
        crlf = '\r\n' in raw
        text = raw.replace('\r\n', '\n') if crlf else raw

        text, n_rt = migrate_runtime(text)
        text, n_plain = migrate_plain(text)

        if n_rt or n_plain:
            out = text.replace('\n', '\r\n') if crlf else text
            with open(path, 'wb') as f:
                f.write(out.encode('utf-8'))
            print(f'{path}: runtime={n_rt} plain={n_plain}')
        total_rt += n_rt
        total_plain += n_plain
    print(f'total: runtime={total_rt} plain={total_plain}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
