"""由 empty / sample 的透射 CSV 构建快中子透射图像。

I0 (空场), I (样品), T = I/I0, A = -ln(T), primary/scatter 分量。
信号定义: 每 pixel 内到达探测器的初级中子 (is_primary_neutron) 的 unique event 计数。
"""
from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

import common as C


def primary_count_image(df, nx, ny):
    """每 pixel 的初级中子 unique event 数。"""
    if df.empty:
        return np.zeros((nx, ny))
    prim = df[(df.is_primary_neutron == 1)]
    if prim.empty:
        return np.zeros((nx, ny))
    # 每 (pixel, event) 计一次
    uniq = prim[["event_id", "pixel_x", "pixel_y"]].drop_duplicates()
    img = np.zeros((nx, ny))
    for px, py in zip(uniq.pixel_x.values, uniq.pixel_y.values):
        if 0 <= px < nx and 0 <= py < ny:
            img[px, py] += 1
    return img


def edep_image(df, nx, ny):
    if df.empty:
        return np.zeros((nx, ny))
    img = np.zeros((nx, ny))
    for px, py, e in zip(df.pixel_x, df.pixel_y, df.edep_keV):
        if 0 <= px < nx and 0 <= py < ny:
            img[px, py] += e
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--empty", type=Path, default=C.RAW, help="空场 raw 目录")
    ap.add_argument("--sample", type=Path, default=C.RAW, help="样品 raw 目录")
    ap.add_argument("--empty-run", type=int, default=0)
    ap.add_argument("--sample-run", type=int, default=1)
    ap.add_argument("--nx", type=int, default=C.PIXELS_X)
    ap.add_argument("--ny", type=int, default=C.PIXELS_Y)
    ap.add_argument("--tag", default="sample")
    ap.add_argument("--out", type=Path, default=C.IMAGES)
    args = ap.parse_args()

    nx, ny = args.nx, args.ny
    d0 = C.read_transmission(args.empty, args.empty_run)
    d1 = C.read_transmission(args.sample, args.sample_run)

    I0 = primary_count_image(d0, nx, ny)
    Ip = primary_count_image(d1, nx, ny)          # primary (透射)
    Iscat = edep_image(
        d1[(d1.is_scattered_neutron == 1)] if not d1.empty else d1.iloc[0:0], nx, ny)
    Itot = edep_image(d1, nx, ny)

    eps = 1.0
    T = np.clip(Ip / np.maximum(I0, eps), 0, None)
    A = -np.log(np.clip(T, 1e-6, None))
    A[I0 < eps] = 0.0
    spr = np.divide(Iscat, np.maximum(Itot, eps), out=np.zeros_like(Iscat), where=Itot > 0)

    np.save(args.out / f"I0_{args.tag}.npy", I0)
    np.save(args.out / f"I_primary_{args.tag}.npy", Ip)
    np.save(args.out / f"transmission_{args.tag}.npy", T)
    np.save(args.out / f"attenuation_{args.tag}.npy", A)
    np.save(args.out / f"scatter_to_primary_{args.tag}.npy", spr)

    # CSV 汇总
    np.savetxt(args.out / f"attenuation_{args.tag}.csv", A, delimiter=",")

    fig, axes = plt.subplots(2, 2, figsize=(10, 9))
    for ax, (im, title, cmap) in zip(
        axes.ravel(),
        [(I0, "I0 (empty)", "viridis"),
         (Ip, "I (sample, primary)", "viridis"),
         (T, "T = I/I0", "gray"),
         (A, "A = -ln(I/I0)", "magma")],
    ):
        ax.imshow(im, origin="lower", cmap=cmap, aspect="equal")
        ax.set_title(title)
        ax.set_xlabel("pixel_y"); ax.set_ylabel("pixel_x")
    fig.tight_layout()
    fig.savefig(C.FIGURES / f"fig3_transmission_{args.tag}.png", dpi=130)
    print(f"[transmission] I0_sum={I0.sum():.0f} I_sum={Ip.sum():.0f} "
          f"<T>={T[I0>eps].mean():.3f} <A>={A[I0>eps].mean():.3f}")
    print(f"  saved -> {args.out}/attenuation_{args.tag}.npy ; figure fig3_transmission_{args.tag}.png")


if __name__ == "__main__":
    main()
