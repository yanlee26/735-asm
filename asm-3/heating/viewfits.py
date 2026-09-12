#!/usr/bin/env python3
"""View FITS images produced by the heat-transfer programs.

    ./viewfits.py plate0.fit                 # open a window
    ./viewfits.py plate0.fit plate1.fit      # compare side by side
    ./viewfits.py plate0.fit --png           # write plate0.png and open it
"""
import argparse
import subprocess
import sys
from pathlib import Path

from astropy.io import fits
import matplotlib
import matplotlib.pyplot as plt

p = argparse.ArgumentParser()
p.add_argument("files", nargs="+", help="FITS file(s) to view")
p.add_argument("--png", action="store_true", help="save a PNG instead of opening a window")
p.add_argument("--cmap", default="inferno", help="matplotlib colormap (default: inferno)")
args = p.parse_args()

if args.png:
    matplotlib.use("Agg")

images = []
for name in args.files:
    path = Path(name)
    if not path.exists():
        sys.exit(f"no such file: {path}")
    data = fits.getdata(path)
    images.append((path, data))
    print(f"{path}  shape={data.shape}  min={data.min():g}  max={data.max():g}  mean={data.mean():g}")

# Share one colour scale so the panels are directly comparable.
vmin = min(d.min() for _, d in images)
vmax = max(d.max() for _, d in images)

fig, axes = plt.subplots(1, len(images), figsize=(5.5 * len(images), 5), squeeze=False)
for ax, (path, data) in zip(axes[0], images):
    im = ax.imshow(data, origin="lower", cmap=args.cmap, vmin=vmin, vmax=vmax)
    ax.set_title(f"{path.name}  ({data.shape[1]}x{data.shape[0]})")
    fig.colorbar(im, ax=ax, label="temperature")
fig.tight_layout()

if args.png:
    out = Path(args.files[0]).with_suffix(".png")
    fig.savefig(out, dpi=110)
    print(f"wrote {out}")
    subprocess.run(["open", str(out)], check=False)
else:
    plt.show()
