#!/usr/bin/env python3
"""Guard the deliberately curated Prawko library against stale assets.

Run from any cwd: python scripts/check_curated_library.py
The six traffic participants are supplied by osc::actor_catalog() in C++.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIB = ROOT / "assets/library"
MANIFEST = LIB / "manifest.json"
QRC = ROOT / "editor/resources/resources.qrc"
ALLOWED = {
    "Prawko | Gotowe drogi": "road.prawko.",
    "Prawko | Krawężniki i chodniki": "style.prawko.",
}


def main() -> int:
    errors: list[str] = []
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    items = manifest.get("items", [])
    if len(items) != 12:
        errors.append(f"expected the 12 current Prawko built-ins, found {len(items)}")
    keys: set[str] = set()
    thumbnails: set[str] = set()
    counts = {category: 0 for category in ALLOWED}
    for item in items:
        category = item.get("category", "")
        key = item.get("key", "")
        if category not in ALLOWED:
            errors.append(f"forbidden legacy category {category!r}: {key}")
            continue
        counts[category] += 1
        if not key.startswith(ALLOWED[category]):
            errors.append(f"unexpected non-Prawko key {key!r}")
        if key in keys:
            errors.append(f"duplicate key {key!r}")
        keys.add(key)
        picture = item.get("thumbnail", "")
        if not picture:
            errors.append(f"missing thumbnail for {key}")
        else:
            thumbnails.add(Path(picture).name)
    if any(count != 6 for count in counts.values()):
        errors.append(f"expected 6 road templates + 6 road styles, found {counts!r}")
    embedded = {
        item for item in re.findall(r'alias="thumbnails/([^"]+)"', QRC.read_text(encoding="utf-8"))
    }
    disk = {p.name for p in (LIB / "thumbnails").glob("*.png")}
    if embedded != thumbnails:
        errors.append(f"Qt thumbnails drift: missing={thumbnails-embedded}, stale={embedded-thumbnails}")
    if disk != thumbnails:
        errors.append(f"on-disk thumbnails drift: missing={thumbnails-disk}, stale={disk-thumbnails}")
    for folder in (ROOT / "assets/samples", ROOT / "assets/library/props", ROOT / "assets/signs/us"):
        if folder.exists() and any(p.is_file() for p in folder.rglob("*")):
            errors.append(f"legacy source/data reappeared in active assets: {folder}")
    fixtures = ROOT / "core/tests/fixtures/legacy_scenes"
    if not fixtures.is_dir() or not (fixtures / "t_junction.xodr").exists():
        errors.append("internal legacy regression fixtures are missing")
    for error in errors:
        print(f"[curated-library] ERROR: {error}")
    if errors:
        return 1
    print("[curated-library] PASS: 6 Prawko roads, 6 Prawko pavement styles; "
          "6 scenario actors added by code; no hidden legacy assets or scenes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
