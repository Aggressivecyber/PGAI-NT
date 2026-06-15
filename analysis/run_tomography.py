"""中子层析 (tomography): 多角度投影采集 + FBP 重建。

样品台旋转 (源+探测器固定), 采集 0-180° 投影 -> sinogram -> 滤波反投影重建切片。

用法:
    python analysis/run_tomography.py --angles 12 --events 2000
    python analysis/run_tomography.py --skip-sim            # 复用已采投影
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from numpy.fft import fft, ifft, fftfreq

import common as C

ROOT = C.ROOT
NT = ROOT / "build" / "NT"
TOMO_RAW = C.RAW / "tomo"


def gen_macro(path, mode, angle_deg, events, material=None):
    lines = ["/pgai/source/spotSize 55 mm", f"/pgai/phantom/mode {mode}"]  # 匹配 FOV60
    if material:
        lines += [f"/pgai/phantom/singleMaterial {material}",
                  "/pgai/phantom/singleThickness 20 mm"]
    lines += [f"/pgai/run/angle {angle_deg} deg", "/run/initialize", f"/run/beamOn {events}"]
    path.write_text("\n".join(lines) + "\n")


def run_sim(tag, mode, angle_deg, events, material=None):
    work = TOMO_RAW / tag
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    gen_macro(work / "run.mac", mode, angle_deg, events, material)
    with open(work / "log.txt", "w") as lf:
        subprocess.run([str(NT), "run.mac"], cwd=work, stdout=lf, stderr=subprocess.STDOUT, check=True)


def primary_image(raw_dir, nx=128, ny=128):
    df = C.read_transmission(raw_dir, 0)
    if df.empty:
        return np.zeros((nx, ny))
    p = df[df.is_primary_neutron == 1][["event_id", "pixel_x", "pixel_y"]].drop_duplicates()
    im = np.zeros((nx, ny))
    for px, py in zip(p.pixel_x.values, p.pixel_y.values):
        if 0 <= px < nx and 0 <= py < ny:
            im[px, py] += 1
    return im


def fbp_reconstruct(sinogram, angles_deg, n_out=128, det_size_mm=40.0,
                    smooth_sigma=4.0):
    """抗噪 FBP: Hann 加窗 ramp filter + sinogram 高斯平滑 + 线性插值反投影。

    Ram-Lak 对泊松噪声极敏感 (放大高频), 实测 σ=0.1 噪声即令相关从 0.9 崩到 0.2。
    Hann 窗 + detector 方向平滑有效抑制, 解析+噪声下相关恢复到 0.84+。
    """
    from scipy.ndimage import gaussian_filter1d
    n_ang, n_det = sinogram.shape
    # 1. sinogram 沿 detector 方向高斯平滑 (降噪)
    s = gaussian_filter1d(sinogram, sigma=smooth_sigma, axis=1) if smooth_sigma > 0 else sinogram
    # 2. Hann 加窗 ramp filter (×2 修正 Ram-Lak 归一化, 恢复 μ 绝对幅度)
    freq = fftfreq(n_det)
    hann = 0.5 * (1 + np.cos(2 * np.pi * freq))
    ramp = 2.0 * np.abs(freq) * hann
    filtered = np.real(ifft(fft(s, axis=1) * ramp, axis=1))
    # 3. 反投影 (像素坐标, 中心对齐)
    center = (n_det - 1) / 2.0
    grid = np.arange(n_out) - (n_out - 1) / 2.0
    X, Y = np.meshgrid(grid, grid)
    img = np.zeros((n_out, n_out))
    for i, th in enumerate(np.deg2rad(angles_deg)):
        t = X * np.sin(th) + Y * np.cos(th) + center
        # FOV 外的点贡献 0 (不 clamp 到边缘, 避免边缘波纹)
        valid = (t >= 0) & (t < n_det - 1)
        vals = np.zeros_like(t)
        tv = t[valid]
        t0 = np.floor(tv).astype(int)
        w = tv - t0
        vals[valid] = filtered[i, t0] * (1 - w) + filtered[i, t0 + 1] * w
        img += vals
    img *= np.pi / n_ang
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", default="degeneracy", help="phantom mode (degeneracy/steel/single)")
    ap.add_argument("--material", default=None)
    ap.add_argument("--angles", type=int, default=12, help="投影数 (0-180° 等分)")
    ap.add_argument("--events", type=int, default=2000)
    ap.add_argument("--nx", type=int, default=128)
    ap.add_argument("--smooth", type=float, default=4.0, help="sinogram 平滑 sigma")
    ap.add_argument("--tag", default="tomo", help="输出文件标签")
    ap.add_argument("--skip-sim", action="store_true")
    args = ap.parse_args()

    if not NT.exists():
        raise SystemExit(f"未找到 {NT}, 请先 cmake --build build")

    angs = np.linspace(0, 180, args.angles, endpoint=False)
    nz = args.nx

    if not args.skip_sim:
        # empty (I0, angle 无关, 跑一次)
        print(f"=== empty I0 (一次) ===")
        run_sim("empty", "empty", 0, args.events)
        # 每角度 sample
        for k, a in enumerate(angs):
            print(f"=== sample angle {a:.1f}° ({k+1}/{len(angs)}) ===")
            run_sim(f"sample_{k:03d}", args.mode, a, args.events, args.material)

    I0 = primary_image(TOMO_RAW / "empty", nz, nz)
    # z 中心切片: 合并 ±5 行提高统计 (切片厚度 ~3mm), 屏蔽低计数像素, clip 异常值
    cy = nz // 2
    slab = slice(cy - 10, cy + 11)
    I0c = I0[:, slab].sum(axis=1)   # z 方向 21 行合并 (厚切片提统计)
    sino = np.zeros((len(angs), nz))
    for k in range(len(angs)):
        I = primary_image(TOMO_RAW / f"sample_{k:03d}", nz, nz)
        Ic = I[:, slab].sum(axis=1)
        # 放宽裁剪: 允许小负衰减(ratio>1)存在, 仅屏蔽极低统计, 不硬截断上限
        ratio = Ic / np.maximum(I0c, 1e-9)
        A = -np.log(np.clip(ratio, 1e-6, 10.0))
        A[I0c < 20] = 0.0           # 仅极低统计像素屏蔽
        sino[k] = A

    np.save(C.IMAGES / f"sinogram_{args.tag}.npy", sino)
    recon = fbp_reconstruct(sino, angs, n_out=nz, smooth_sigma=args.smooth)
    np.save(C.IMAGES / f"reconstruction_{args.tag}.npy", recon)

    fig, axes = plt.subplots(1, 3, figsize=(14, 4))
    axes[0].imshow(I0, origin="lower", cmap="viridis"); axes[0].set_title("I0 (empty)")
    im1 = axes[1].imshow(sino, aspect="auto", cmap="gray",
                         extent=[0, nz, angs[-1], angs[0]])
    axes[1].set_title("Sinogram"); axes[1].set_xlabel("detector pixel_x"); axes[1].set_ylabel("angle (deg)")
    axes[2].imshow(recon, origin="lower", cmap="magma"); axes[2].set_title("FBP reconstruction")
    fig.tight_layout(); fig.savefig(C.FIGURES / f"fig_tomography_{args.tag}.png", dpi=130)
    print(f"[tomo] tag={args.tag} angles={len(angs)} -> sinogram/reconstruction/fig_tomography_{args.tag}")


if __name__ == "__main__":
    main()
