#!/usr/bin/env python3
"""One-time non-destructive migration: 19 project-owned GLBs -> global Prawko library."""
import hashlib
import json
import shutil
from pathlib import Path

root = Path(__file__).resolve().parents[1]
assert root == Path("F:/PrawkoRoadStudio"), "Use only the canonical workspace"
global_manifest_path = root / "assets/library/manifest.json"
legacy_manifest_path = root / "projects/Prawko-Scenariusze/assets/library/manifest.json"
global_manifest = json.loads(global_manifest_path.read_text(encoding="utf-8-sig"))
legacy_manifest = json.loads(legacy_manifest_path.read_text(encoding="utf-8-sig"))
legacy_props = [item for item in legacy_manifest["items"] if item.get("create",{}).get("model_file")]
assert len(legacy_props) == 19, f"expected exactly 19 existing GLB entries, got {len(legacy_props)}"
global_keys = {item["key"] for item in global_manifest["items"]}
assert all(x["key"] not in global_keys for x in legacy_props), "refuse duplicate or half migration"
assert len(global_manifest["items"]) == 12
new_global = []
operations = []
for item in legacy_props:
    new = json.loads(json.dumps(item))
    old_model = root / "projects/Prawko-Scenariusze" / item["create"]["model_file"]
    old_thumb = root / "projects/Prawko-Scenariusze" / item["thumbnail"]
    new_model = root / "assets/props" / old_model.name
    new_thumb = root / "assets/library/thumbnails" / old_thumb.name
    for old, dest in ((old_model,new_model),(old_thumb,new_thumb)):
        assert old.is_file(), f"missing source {old}"
        if dest.exists():
            assert hashlib.sha256(old.read_bytes()).digest() == hashlib.sha256(dest.read_bytes()).digest(), f"conflicting {dest}"
        operations.append((old,dest))
    new["create"]["model_file"] = str(new_model.relative_to(root)).replace("\\","/")
    new["thumbnail"] = str(new_thumb.relative_to(root)).replace("\\","/")
    new_global.append(new)
global_manifest["items"].extend(new_global)
global_manifest["generated"] = "2026-11-17"
legacy_manifest["items"] = [x for x in legacy_manifest["items"] if x not in legacy_props]
for src,dest in operations:
    dest.parent.mkdir(parents=True,exist_ok=True)
    if not dest.exists():
        shutil.copy2(src,dest)
    assert src.read_bytes()==dest.read_bytes()
global_manifest_path.write_text(json.dumps(global_manifest,ensure_ascii=False,indent=2)+"\n", encoding="utf-8")
legacy_manifest_path.write_text(json.dumps(legacy_manifest,ensure_ascii=False,indent=2)+"\n", encoding="utf-8")
print("GLOBAL_ASSET_MIGRATION_PASS: global 31 total = 12 road + 19 GLB; project overlay 0 GLB")
