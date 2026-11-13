"""Quick PBR atlas artifact checks (optional Pillow+NumPy dependency)."""
from pathlib import Path
from PIL import Image
import numpy as np

root = Path(__file__).resolve().parents[1] / "assets" / "textures" / "prawko_atlas"
for surface in ("asphalt", "concrete"):
    for channel, ext in (("diff", "jpg"), ("nor_gl", "png"), ("rough", "jpg")):
        resolution = 2048 if channel == "diff" else 1024
        path = root / f"{surface}_{channel}_{resolution}.{ext}"
        rgb = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)
        assert rgb.shape == (resolution, resolution, 3), path
        assert np.isfinite(rgb).all(), path
        # Rotation-free offsets vary the pattern: the four adjacent original
        # 4 m tiles must no longer be pixel identical.
        tile = resolution // 4
        a = rgb[tile:2*tile, tile:2*tile]
        b = rgb[tile:2*tile, 2*tile:3*tile]
        mismatch = np.abs(a - b).mean()
        assert mismatch > 0.4, (path, mismatch)
        # Atlas edges are close to each other, given seamless source textures.
        x_seam = np.abs(rgb[:, 0] - rgb[:, -1]).mean()
        y_seam = np.abs(rgb[0] - rgb[-1]).mean()
        print(surface,channel,"tile_difference",round(float(mismatch),2),
              "x_seam",round(float(x_seam),2),"y_seam",round(float(y_seam),2))
        assert max(x_seam, y_seam) < 25, (path, x_seam, y_seam)
        if channel == "nor_gl":
            normals = rgb / 127.5 - 1.0
            length = np.linalg.norm(normals, axis=2)
            assert np.abs(length - 1.0).mean() < 0.012, (path, float(np.abs(length - 1).mean()))
            assert (normals[..., 2] > 0).all(), path
print("PBR_ATLAS_QA_PASS")
