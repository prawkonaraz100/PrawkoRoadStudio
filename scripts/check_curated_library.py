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
BUNDLED = LIB / "bundled-defaults.json"
QRC = ROOT / "editor/resources/resources.qrc"
ALLOWED = {
    "Prawko | Gotowe drogi": "road.prawko.",
    "Prawko | Krawężniki i chodniki": "style.prawko.",
}


def main() -> int:
    errors: list[str] = []
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    bundled = json.loads(BUNDLED.read_text(encoding="utf-8"))
    items = bundled.get("items", [])
    global_items = manifest.get("items", [])
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
    if "bundled-defaults.json" not in QRC.read_text(encoding="utf-8"):
        errors.append("qrc must bundle the 12 static road presets, not the user/global GLB manifest")
    global_keys = [item.get("key", "") for item in global_items]
    if len(global_keys) != len(set(global_keys)):
        errors.append("duplicate keys in the shared global catalog")
    if any(item != next((g for g in global_items if g.get("key") == item["key"]), None)
           for item in items):
        errors.append("shared catalog no longer contains the 12 unchanged road presets")
    imported = [item for item in global_items if item.get("create", {}).get("model_file")]
    if len(imported) < 19:
        errors.append(f"expected at least 19 shared 3D models, found {len(imported)}")
    global_thumbnails = set(thumbnails)
    for item in global_items:
        picture = item.get("thumbnail", "")
        if picture:
            global_thumbnails.add(Path(picture).name)
    for item in imported:
        key = item["key"]
        path = ROOT / item["create"]["model_file"]
        thumb = ROOT / item.get("thumbnail", "")
        if not path.is_file() or path.suffix.lower() != ".glb":
            errors.append(f"shared GLB missing for {key}: {path}")
        if not thumb.is_file():
            errors.append(f"shared thumbnail missing for {key}: {thumb}")
    if disk != global_thumbnails:
        errors.append(f"global thumbnails drift: missing={global_thumbnails-disk}, stale={disk-global_thumbnails}")
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
    print(f"[curated-library] PASS: 6 Prawko roads, 6 pavement styles, "
          f"{len(imported)} shared GLBs with real thumbnails; "
          "6 scenario actors from code; no hidden legacy assets or scenes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
