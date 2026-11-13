"""Regenerate PrawkoRoadStudio's 16 m seamless PBR surface atlases.

Original Poly Haven CC0 512 px source textures are preserved verbatim.
Usage: python scripts/build_pbr_atlas.py
Requires Pillow and NumPy. The deterministic 4x4 corner-offset blend uses
the same coordinate transforms for albedo, OpenGL Y+ normal and roughness.
"""
from pathlib import Path
from PIL import Image, ImageFilter
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "textures"
TARGET = SOURCE / "prawko_atlas"
CELLS = 4
SIDE = 512
SEED = 0x5123A7


def corner_offset(x: int, y: int, salt: int) -> tuple[int, int]:
    """Stable spatial hash; integer wrapping also makes atlas edges tile."""
    x %= CELLS
    y %= CELLS
    n = ((x + 17) * 73856093) ^ ((y + 37) * 19349663) ^ (salt * 83492791) ^ SEED
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    n ^= n >> 16
    return int(n % SIDE), int((n >> 9) % SIDE)


def prepare(image: Image.Image, surface: str, channel: str) -> np.ndarray:
    rgb = np.asarray(image.convert("RGB"), dtype=np.float32)
    if channel == "diff" and surface == "asphalt":
        # Break up the very recognizable deep crack without erasing grain.
        # Lift only a small portion of the darkest *locally contrasting* pixels.
        lightness = rgb.mean(axis=2)
        local = np.asarray(
            Image.fromarray(np.uint8(np.clip(lightness, 0, 255)), "L")
            .filter(ImageFilter.GaussianBlur(9)),
            dtype=np.float32,
        )
        dark = np.clip((local - lightness - 14.0) / 49.0, 0.0, 1.0)
        rgb = np.clip(rgb + dark[..., None] * 24.0, 0, 255)
        rgb *= np.array([0.96, 0.97, 0.98], dtype=np.float32)
    elif channel == "diff" and surface == "concrete":
        # Cooler, neutral grey pavement instead of strongly beige/brown.
        grey = rgb.mean(axis=2, keepdims=True)
        rgb = np.clip(rgb * 0.58 + grey * 0.42 + 9.0, 0, 255)
    elif channel == "nor_gl":
        # Slightly soften the micro-relief of concrete, avoid plastic-looking
        # brushed ridges on the same surface as tiny vertical curb faces.
        strength = 0.75 if surface == "concrete" else 0.88
        tangent_xy = (rgb[..., 0:2] / 127.5 - 1.0) * strength
        z = np.sqrt(np.maximum(1.0 - np.sum(tangent_xy**2, axis=2), 0.02))
        n = np.dstack((tangent_xy, z))
        n /= np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-6)
        rgb = (n + 1.0) * 127.5
    return rgb


def atlas(source: np.ndarray, salt: int, normal: bool) -> Image.Image:
    assert source.shape == (SIDE, SIDE, 3)
    output = np.empty((CELLS * SIDE, CELLS * SIDE, 3), dtype=np.uint8)
    row, col = np.mgrid[0:SIDE, 0:SIDE]
    # Bilinear smoothstep weights converge with zero slope at cell edges.
    tx = (col + 0.5) / SIDE
    ty = (row + 0.5) / SIDE
    tx = (tx * tx * (3.0 - 2.0 * tx))[..., None]
    ty = (ty * ty * (3.0 - 2.0 * ty))[..., None]
    for cy in range(CELLS):
        for cx in range(CELLS):
            tile = np.zeros((SIDE, SIDE, 3), dtype=np.float32)
            for dy in (0, 1):
                for dx in (0, 1):
                    ox, oy = corner_offset(cx + dx, cy + dy, salt)
                    sample = source[(row + oy) % SIDE, (col + ox) % SIDE, :]
                    wx = tx if dx else (1.0 - tx)
                    wy = ty if dy else (1.0 - ty)
                    tile += sample * (wx * wy)
            if normal:
                n = tile / 127.5 - 1.0
                n /= np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-6)
                tile = (n + 1.0) * 127.5
            output[cy * SIDE:(cy + 1) * SIDE, cx * SIDE:(cx + 1) * SIDE] = (
                np.clip(tile, 0, 255).astype(np.uint8)
            )
    return Image.fromarray(output, "RGB")


def generate() -> None:
    TARGET.mkdir(exist_ok=True)
    inputs = {
        "asphalt": (
            "asphalt/asphalt_02_diff_512.jpg",
            "asphalt/asphalt_02_nor_gl_512.png",
            "asphalt/asphalt_02_rough_512.jpg",
        ),
        "concrete": (
            "concrete/brushed_concrete_diff_512.jpg",
            "concrete/brushed_concrete_nor_gl_512.png",
            "concrete/brushed_concrete_rough_512.jpg",
        ),
    }
    for surface, paths in inputs.items():
        for ch, src in zip(("diff", "nor_gl", "rough"), paths):
            image = Image.open(SOURCE / src)
            if image.size != (SIDE, SIDE):
                raise ValueError(f"Expected 512x512: {src}")
            data = prepare(image, surface, ch)
            finished = atlas(data, 41 if surface == "asphalt" else 73, ch == "nor_gl")
            # Preserve 2048 albedo grain; normal and roughness can use 1024
            # because these fine channels are filtered at driving distances.
            # Saving only the latter at half resolution reduces decode,
            # VRAM and GLB size without altering 16 m UV periodicity.
            resolution = 2048 if ch == "diff" else 1024
            if resolution == 1024:
                finished = finished.resize((resolution, resolution), Image.Resampling.LANCZOS)
                if ch == "nor_gl":
                    n = np.asarray(finished, dtype=np.float32) / 127.5 - 1.0
                    n /= np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-6)
                    finished = Image.fromarray(
                        np.uint8(np.clip((n + 1.0) * 127.5, 0, 255)), "RGB"
                    )
            suffix = "png" if ch == "nor_gl" else "jpg"
            out = TARGET / f"{surface}_{ch}_{resolution}.{suffix}"
            if suffix == "png":
                finished.save(out, optimize=True)
            else:
                finished.save(out, quality=88, subsampling=0)
            print(f"PBR_ATLAS_BUILT {out.relative_to(ROOT)} {finished.size}")
    print("PBR_ATLAS_PASS")


if __name__ == "__main__":
    generate()
