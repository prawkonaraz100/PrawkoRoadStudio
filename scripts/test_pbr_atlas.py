"""Quick PBR atlas artifact checks (optional Pillow+NumPy dependency)."""
from pathlib import Path
from PIL import Image
import numpy as np

root = Path(__file__).resolve().parents[1] / "assets" / "textures" / "prawko_atlas"
for surface in ("asphalt", "concrete"):
    for channel, ext in (("diff", "jpg"), ("nor_gl", "png"), ("rough", "jpg")):
        path = root / f"{surface}_{channel}_2048.{ext}"
        rgb = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)
        assert rgb.shape == (2048, 2048, 3), path
        assert np.isfinite(rgb).all(), path
        # Rotation-free offsets vary the pattern: the four adjacent original
        # 4 m tiles must no longer be pixel identical.
        a = rgb[512:1024, 512:1024]
        b = rgb[512:1024, 1024:1536]
        mismatch = np.abs(a - b).mean()
        assert mismatch > 0.4, (path, mismatch)
        # Atlas edges are close to each other, given seamless source textures.
        x_seam = np.abs(rgb[:, 0] - rgb[:, -1]).mean()
        y_seam = np.abs(rgb[0] - rgb[-1]).mean()
        print(surface,channel,"tile_difference",round(float(mismatch),2),
              "x_seam",round(float(x_seam),2),"y_seam",round(float(y_seam),2))
        assert max(x_seam, y_seam) < 25, (path, x_seam, y_seam)
print("PBR_ATLAS_QA_PASS")
