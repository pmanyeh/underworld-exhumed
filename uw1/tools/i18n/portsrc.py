"""Where a port source is: UW1's src/port, else Exhume's runtime/port, which holds the
localization and HD text modules both games share (utf8, layout, localization, hd_font, ...)."""
from pathlib import Path

UW1 = Path(__file__).resolve().parents[2]
RUNTIME = UW1.parent / 'exhume' / 'runtime' / 'port'


def port_src(rel):
    p = UW1 / 'src' / 'port' / rel
    return p if p.exists() else RUNTIME / rel
