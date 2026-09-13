#!/usr/bin/env python3
"""Render the initial and converged plates side by side for the report.

    ./make_plate_figure.py plate0.fit plate1.fit results/plate.png
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from astropy.io import fits

a, b, out = sys.argv[1], sys.argv[2], sys.argv[3]
d0 = fits.getdata(a).astype(float)
d1 = fits.getdata(b).astype(float)
ny, nx = d1.shape

vmin = min(d0.min(), d1.min())
vmax = max(d0.max(), d1.max())

fig, axes = plt.subplots(1, 2, figsize=(12.5, 5.4))
for ax, d, title in ((axes[0], d0, "Initial state: fixed boundaries and\nprinted circuit components"),
                     (axes[1], d1, "Converged heat distribution")):
    im = ax.imshow(d, origin="lower", cmap="inferno", vmin=vmin, vmax=vmax)
    ax.set_title(f"{title}\n{nx}x{ny} plate", fontsize=11)
    ax.set_xlabel("x"); ax.set_ylabel("y")
    fig.colorbar(im, ax=ax, label="temperature (K)", fraction=0.046, pad=0.03)

fig.tight_layout()
Path(out).parent.mkdir(parents=True, exist_ok=True)
fig.savefig(out, dpi=130)
print(f"wrote {out}")
for name, d in ((a, d0), (b, d1)):
    print(f"  {name}: min={d.min():.2f} K  max={d.max():.2f} K  mean={d.mean():.2f} K")
