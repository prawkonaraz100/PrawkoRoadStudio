#!/usr/bin/env python3
"""Make an editable asset-showcase scene in the ONE existing Prawko project.

Does not modify previous scenes or create another project. Refuses overwrites.
"""
from __future__ import annotations

import json
from pathlib import Path
import shutil
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "projects" / "Prawko-Scenariusze"
BASE = "06l-r6b2j-dwie-wysokosci-kaluz"
DEMO = "07-pokaz-biblioteki-assetow"
MANIFEST = PROJECT / "assets/library/manifest.json"

# The other fourteen kinds are already in R6b2j scene 06l.
# Object dimensions for existing props are taken from the previous validated scenes.
ADDITIONS = [
    ("r6b-drzewo", 149.0, -10.9, 0.12, 2.344, 7.433),
    ("r6b-latarnia", 171.0, -8.3, 0.0, 1.109, 5.720),
    ("r6b2-samochod", 178.0, -3.2, 0.0, 2.500, 1.750),
    ("r6b2b-sciezka-zwirowa-12m", 153.0, 20.0, 1.5707963267948966, 6.054, 0.230),
    ("prawko_bariera_drogowa", 195.0, -7.0, 0.0, 2.500, 1.500),
]


def main() -> None:
    if ROOT != Path(r"F:\PrawkoRoadStudio"):
        raise RuntimeError("Only the canonical F:\\PrawkoRoadStudio workspace is permitted")
    expected = [PROJECT / (DEMO + ext) for ext in (".xodr", ".xosc", ".rmscene.json")]
    if any(path.exists() for path in expected):
        raise FileExistsError("Refusing to overwrite an existing showcase scene")
    original = {
        ext: (PROJECT / (BASE + ext)).read_text(encoding="utf-8-sig")
        for ext in (".xodr", ".xosc", ".rmscene.json")
    }
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8-sig"))
    existing = {item["create"]["model"] for item in manifest["items"]
                if item.get("create", {}).get("kind") == "tree"}
    missing = {model for model, *_ in ADDITIONS} - existing
    if missing != {"prawko_bariera_drogowa"}:
        raise RuntimeError("Unexpected library contents, review before changing: " + repr(missing))
    source = ROOT / "assets/prawko/blender/prawko_bariera_drogowa.glb"
    target = PROJECT / "assets/props/prawko_bariera_drogowa.glb"
    if not source.is_file() or target.exists():
        raise RuntimeError("Barrier source missing or project copy already exists")
    objects = ET.fromstring(original[".xodr"]).findall(".//objects/object")
    ids = {int(obj.attrib["id"]) for obj in objects}
    new_objects = []
    for model, s, t, heading, radius, height in ADDITIONS:
        oid = max(ids) + 1
        ids.add(oid)
        new_objects.append(
            f'<object type="tree" name="{model}" id="{oid}" '
            f's="{s:g}" t="{t:g}" zOffset="0" hdg="{heading:.10f}" '
            f'orientation="none" radius="{radius:.3f}" height="{height:.3f}" />'
        )
    if original[".xodr"].count("</objects>") != 1:
        raise RuntimeError("OpenDRIVE object collection changed")
    new_road = original[".xodr"].replace(
        "</objects>", "\n" + "\n".join(new_objects) + "\n</objects>", 1
    ).replace(
        'name="Miasto: ruszanie i obserwacja lusterek"',
        'name="Prawko: scena pokazowa biblioteki assetow"', 1
    )
    ET.fromstring(new_road)
    new_scenario = original[".xosc"].replace(BASE + ".xodr", DEMO + ".xodr")
    if new_scenario == original[".xosc"]:
        raise RuntimeError("OpenSCENARIO road reference was not rewritten")
    new_scenario = new_scenario.replace(
        'description="Miasto: ruszanie i obserwacja lusterek"',
        'description="Prawko: scena pokazowa biblioteki assetow"', 1
    )
    ET.fromstring(new_scenario)
    sidecar = json.loads(original[".rmscene.json"])
    sidecar["view"]["target"] = [109.0, 4.0, 1.2]
    sidecar["view"]["distance"] = 63.0
    sidecar["view"]["pitch"] = 0.55
    sidecar["view"]["yaw"] = -1.5707963267948966
    sidecar["time_of_day"] = "day"
    sidecar["weather"] = "rain"  # demonstrate the two planar-reflection levels
    manifest["items"].append({
        "key": "prop.prawko_bariera_drogowa",
        "label": "Bariera drogowa (Prawko GLB)",
        "category": "Prawko | Obiekty 3D",
        "thumbnail": "assets/library/thumbnails/prawko_bariera_drogowa.png",
        "create": {
            "kind": "tree",
            "model": "prawko_bariera_drogowa",
            "model_file": "assets/props/prawko_bariera_drogowa.glb",
            "source": "assets/prawko/blender/prawko_bariera_drogowa.glb",
            "license": "Apache-2.0 / PrawkoNaRaz"
        }
    })
    present = {item["create"]["model"] for item in manifest["items"]
               if item.get("create", {}).get("kind") == "tree"}
    found = {obj.get("name") for obj in ET.fromstring(new_road).findall(".//objects/object")}
    if not present.issubset(found) or len(present) != 19:
        raise RuntimeError("Demo must contain all nineteen library prop kinds")

    shutil.copyfile(source, target)
    (PROJECT / (DEMO + ".xodr")).write_text(new_road, encoding="utf-8")
    (PROJECT / (DEMO + ".xosc")).write_text(new_scenario, encoding="utf-8")
    (PROJECT / (DEMO + ".rmscene.json")).write_text(
        json.dumps(sidecar, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    MANIFEST.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print("PRAWKO_SHOWCASE_GENERATED: 07 / 19 prop kinds, 42 object instances + simulation")
    print("SHOWCASE_FILE:", PROJECT / (DEMO + ".xodr"))


if __name__ == "__main__":
    main()
