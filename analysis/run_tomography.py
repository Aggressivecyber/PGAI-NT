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


def gen_macro(path, mode, angle_deg, events, material=None, threads=16):
    lines = [
        f"/run/numberOfThreads {threads}",
        "/pgai/source/spotSize 65 mm",
        f"/pgai/phantom/mode {mode}",
    ]  # 匹配 FOV70
    if material:
        lines += [f"/pgai/phantom/singleMaterial {material}",
                  "/pgai/phantom/singleThickness 20 mm"]
    lines += [f"/pgai/run/angle {angle_deg:g} deg", "/run/initialize", f"/run/beamOn {events}"]
    path.write_text("\n".join(lines) + "\n")


def run_sim(tag, mode, angle_deg, events, material=None, raw_root=TOMO_RAW, threads=16):
    work = Path(raw_root) / tag
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    gen_macro(work / "run.mac", mode, angle_deg, events, material, threads=threads)
    with open(work / "log.txt", "w") as lf:
        subprocess.run([str(NT), "run.mac"], cwd=work, stdout=lf, stderr=subprocess.STDOUT, check=True)


def primary_image(raw_dir, nx=128, ny=128):
    """CT 用未碰撞初级中子 (is_uncollided_primary): 满足 Radon 线积分假设。
    若字段缺失则回退到 is_primary_neutron。"""
    df = C.read_transmission(raw_dir, 0)
    if df.empty:
        return np.zeros((nx, ny))
    col = "is_uncollided_primary" if "is_uncollided_primary" in df.columns else "is_primary_neutron"
    p = df[df[col] == 1][["event_id", "pixel_x", "pixel_y"]].drop_duplicates()
    im = np.zeros((nx, ny))
    for px, py in zip(p.pixel_x.values, p.pixel_y.values):
        if 0 <= px < nx and 0 <= py < ny:
            im[px, py] += 1
    return im


def adaptive_min_i0(i0_counts):
    positive = np.asarray(i0_counts, dtype=float)
    positive = positive[positive > 0]
    if positive.size == 0:
        return np.inf
    return max(3.0, min(20.0, 0.25 * float(np.median(positive))))


def attenuation_from_counts(sample_counts, i0_counts, min_i0=None,
                            pseudocount=0.5, max_attenuation=5.0,
                            max_negative=0.5):
    """从 sample/I0 计数生成稳定的 -ln(I/I0) 投影。

    低统计量 CT 调试常见每 bin 只有十几个 I0 命中，固定 20-count 阈值会把
    有效投影清空；Jeffreys 伪计数同时避免 Ic=0 生成支配 FBP 的极端白条。
    """
    i0 = np.asarray(i0_counts, dtype=float)
    sample = np.asarray(sample_counts, dtype=float)
    threshold = adaptive_min_i0(i0) if min_i0 is None else float(min_i0)
    if not np.isfinite(threshold):
        return np.zeros_like(i0, dtype=float)

    ratio = (sample + pseudocount) / np.maximum(i0 + pseudocount, 1e-12)
    ratio = np.clip(ratio, np.exp(-max_attenuation), np.exp(max_negative))
    attenuation = -np.log(ratio)
    attenuation[i0 < threshold] = 0.0
    return attenuation


def default_slab_half_width(mode, nx):
    if mode == "cttest":
        return max(1, min(nx // 2 - 1, int(round(20.0 / C.DETECTOR_SIZE_MM * nx))))
    return max(1, min(nx // 2 - 1, 10))


def remove_angle_invariant_background(sinogram):
    """去掉随角度不变的投影背景，突出旋转小结构的差异信号。"""
    s = np.asarray(sinogram, dtype=float)
    return s - np.median(s, axis=0, keepdims=True)


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


def _sart_projectors(n_out, n_det, angles_deg):
    img_center = (n_out - 1) / 2.0
    det_center = (n_det - 1) / 2.0
    grid = np.arange(n_out) - img_center
    x, y = np.meshgrid(grid, grid, indexing="ij")
    pix_all = np.arange(n_out * n_out).reshape(n_out, n_out)
    blocks = []
    for th in np.deg2rad(angles_deg):
        t = x * np.sin(th) + y * np.cos(th) + det_center
        valid = (t >= 0) & (t < n_det - 1)
        tv = t[valid]
        pix = pix_all[valid].ravel()
        t0 = np.floor(tv).astype(np.int32).ravel()
        w = (tv.ravel() - t0).astype(float)
        rows = np.concatenate([t0, t0 + 1])
        pixels = np.concatenate([pix, pix])
        weights = np.concatenate([1.0 - w, w])

        row_sum = np.zeros(n_det, dtype=float)
        np.add.at(row_sum, rows, weights)
        row_sum[row_sum <= 0] = 1.0

        col_sum = np.zeros(n_out * n_out, dtype=float)
        np.add.at(col_sum, pixels, weights)
        col_sum[col_sum <= 0] = 1.0

        blocks.append((rows, pixels, weights, row_sum, col_sum))
    return blocks


def _tv_flow_denoise(image, weight=0.02, iterations=6, step=0.18):
    if weight <= 0 or iterations <= 0:
        return image
    u = image.astype(float, copy=True)
    eps = 1e-6
    for _ in range(iterations):
        gx = np.zeros_like(u)
        gy = np.zeros_like(u)
        gx[:, :-1] = u[:, 1:] - u[:, :-1]
        gy[:-1, :] = u[1:, :] - u[:-1, :]
        norm = np.sqrt(gx * gx + gy * gy + eps)
        nx = gx / norm
        ny = gy / norm
        div = np.zeros_like(u)
        div[:, :-1] += nx[:, :-1]
        div[:, 1:] -= nx[:, :-1]
        div[:-1, :] += ny[:-1, :]
        div[1:, :] -= ny[:-1, :]
        u += step * weight * div
    return u


def _fallback_sart_tv_reconstruct(sinogram, angles_deg, n_out=128, iterations=12,
                                  relaxation=0.65, tv_weight=0.02,
                                  tv_iterations=6, nonnegative=True):
    s = np.asarray(sinogram, dtype=float)
    n_ang, n_det = s.shape
    if len(angles_deg) != n_ang:
        raise ValueError("angles_deg length must match sinogram rows")

    x = np.zeros(n_out * n_out, dtype=float)
    blocks = _sart_projectors(n_out, n_det, angles_deg)

    for _ in range(iterations):
        for i, (rows, pixels, weights, row_sum, col_sum) in enumerate(blocks):
            estimate = np.zeros(n_det, dtype=float)
            np.add.at(estimate, rows, weights * x[pixels])
            residual = (s[i] - estimate) / row_sum
            correction = np.zeros_like(x)
            np.add.at(correction, pixels, weights * residual[rows])
            x += relaxation * correction / col_sum
        img = x.reshape(n_out, n_out)
        if nonnegative:
            np.maximum(img, 0.0, out=img)
        img = _tv_flow_denoise(img, weight=tv_weight, iterations=tv_iterations)
        if nonnegative:
            np.maximum(img, 0.0, out=img)
        x = img.ravel()
    return x.reshape(n_out, n_out)


def sart_tv_reconstruct(sinogram, angles_deg, n_out=128, iterations=12,
                        relaxation=0.65, tv_weight=0.02,
                        tv_iterations=6, nonnegative=True):
    """SART + TV 正则，面向稀疏角度/低统计投影。"""
    s = np.asarray(sinogram, dtype=float)
    if s.shape[0] != len(angles_deg):
        raise ValueError("angles_deg length must match sinogram rows")

    try:
        from skimage.restoration import denoise_tv_chambolle
        from skimage.transform import iradon_sart, resize
    except ImportError:
        return _fallback_sart_tv_reconstruct(
            s, angles_deg, n_out=n_out, iterations=iterations,
            relaxation=relaxation, tv_weight=tv_weight,
            tv_iterations=tv_iterations, nonnegative=nonnegative,
        )

    theta = np.asarray(angles_deg, dtype=float)
    radon_image = s.T
    image = np.zeros((radon_image.shape[0], radon_image.shape[0]), dtype=float)
    clip = (0.0, np.inf) if nonnegative else None
    for _ in range(iterations):
        image = iradon_sart(radon_image, theta=theta, image=image,
                            relaxation=relaxation, clip=clip)
        if tv_weight > 0 and tv_iterations > 0:
            for _ in range(tv_iterations):
                image = denoise_tv_chambolle(image, weight=tv_weight)
        if nonnegative:
            np.maximum(image, 0.0, out=image)
        if not np.isfinite(image).all():
            raise FloatingPointError("SART-TV produced non-finite values")

    if image.shape != (n_out, n_out):
        image = resize(image, (n_out, n_out), order=1, mode="reflect",
                       anti_aliasing=False, preserve_range=True)
    return np.asarray(image, dtype=float)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", default="degeneracy", help="phantom mode (degeneracy/steel/single)")
    ap.add_argument("--material", default=None)
    ap.add_argument("--angles", type=int, default=120, help="投影数 (0-180° 等分)")
    ap.add_argument("--events", type=int, default=100000)
    ap.add_argument("--nx", type=int, default=128)
    ap.add_argument("--slab-half", type=int, default=None,
                    help="z 中心切片半宽(pixel); cttest 默认按 40mm 高度聚合")
    ap.add_argument("--smooth", type=float, default=4.0, help="sinogram 平滑 sigma")
    ap.add_argument("--method", choices=["fbp", "sart-tv", "both"], default="both",
                    help="重建方法: FBP, SART+TV, 或二者都输出")
    ap.add_argument("--sart-iters", type=int, default=8, help="SART-TV 迭代轮数")
    ap.add_argument("--sart-relaxation", type=float, default=0.08, help="SART 松弛因子")
    ap.add_argument("--tv-weight", type=float, default=0.04, help="TV 正则强度")
    ap.add_argument("--tv-iters", type=int, default=1, help="每轮 SART 后的 TV 去噪步数")
    ap.add_argument("--tag", default="tomo", help="输出文件标签")
    ap.add_argument("--threads", type=int, default=16)
    ap.add_argument("--raw-root", type=Path, default=TOMO_RAW,
                    help="raw projection directory; contains empty and sample_###")
    ap.add_argument("--skip-sim", action="store_true")
    args = ap.parse_args()

    if not NT.exists():
        raise SystemExit(f"未找到 {NT}, 请先 cmake --build build")

    angs = np.linspace(0, 180, args.angles, endpoint=False)
    nz = args.nx

    if not args.skip_sim:
        # empty (I0, angle 无关, 跑一次)
        print(f"=== empty I0 (一次) ===")
        run_sim("empty", "empty", 0, args.events, raw_root=args.raw_root, threads=args.threads)
        # 每角度 sample
        for k, a in enumerate(angs):
            print(f"=== sample angle {a:.1f}° ({k+1}/{len(angs)}) ===")
            run_sim(f"sample_{k:03d}", args.mode, a, args.events, args.material,
                    raw_root=args.raw_root, threads=args.threads)

    I0 = primary_image(args.raw_root / "empty", nz, nz)
    # z 中心切片: 对轴向均匀的 cttest 聚合整根棒高度；其他 phantom 保持薄切片。
    cy = nz // 2
    slab_half = default_slab_half_width(args.mode, nz) if args.slab_half is None else args.slab_half
    slab_half = max(1, min(nz // 2 - 1, slab_half))
    slab = slice(cy - slab_half, cy + slab_half + 1)
    I0c = I0[:, slab].sum(axis=1)
    min_i0 = adaptive_min_i0(I0c)
    sino = np.zeros((len(angs), nz))
    for k in range(len(angs)):
        I = primary_image(args.raw_root / f"sample_{k:03d}", nz, nz)
        Ic = I[:, slab].sum(axis=1)
        A = attenuation_from_counts(Ic, I0c, min_i0=min_i0)
        sino[k] = A
        # 诊断
        mean_bin = I0c[I0c > 0].mean() if (I0c > 0).any() else 0
        low_frac = (I0c < min_i0).sum() / len(I0c)
        print(f"  angle {angs[k]:5.1f}°: slab=±{slab_half}px  I0均值/bin={mean_bin:.0f}  I0阈值={min_i0:.1f}  低计数bin比={low_frac:.1%}")

    np.save(C.IMAGES / f"sinogram_{args.tag}.npy", sino)
    sino_contrast = remove_angle_invariant_background(sino)
    np.save(C.IMAGES / f"sinogram_contrast_{args.tag}.npy", sino_contrast)
    recon = None
    recon_contrast = None
    recon_sart = None
    recon_sart_contrast = None
    if args.method in ("fbp", "both"):
        recon = fbp_reconstruct(sino, angs, n_out=nz, smooth_sigma=args.smooth)
        np.save(C.IMAGES / f"reconstruction_{args.tag}.npy", recon)
        recon_contrast = fbp_reconstruct(sino_contrast, angs, n_out=nz, smooth_sigma=args.smooth)
        np.save(C.IMAGES / f"reconstruction_contrast_{args.tag}.npy", recon_contrast)
    if args.method in ("sart-tv", "both"):
        recon_sart = sart_tv_reconstruct(
            sino, angs, n_out=nz, iterations=args.sart_iters,
            relaxation=args.sart_relaxation, tv_weight=args.tv_weight,
            tv_iterations=args.tv_iters,
        )
        np.save(C.IMAGES / f"reconstruction_sart_tv_{args.tag}.npy", recon_sart)
        recon_sart_contrast = sart_tv_reconstruct(
            sino_contrast, angs, n_out=nz, iterations=args.sart_iters,
            relaxation=args.sart_relaxation, tv_weight=args.tv_weight,
            tv_iterations=args.tv_iters, nonnegative=False,
        )
        np.save(C.IMAGES / f"reconstruction_sart_tv_contrast_{args.tag}.npy", recon_sart_contrast)

    panels = [("I0 (empty)", I0, "viridis", None)]
    panels.append(("Sinogram", sino, "gray", "sino"))
    if recon is not None:
        panels.append(("FBP attenuation", recon, "magma", "percentile"))
    if recon_sart is not None:
        panels.append(("SART-TV attenuation", recon_sart, "magma", "percentile"))
    contrast_img = recon_sart_contrast if recon_sart_contrast is not None else recon_contrast
    if contrast_img is not None:
        panels.append(("SART-TV contrast" if recon_sart_contrast is not None else "FBP contrast",
                       contrast_img, "coolwarm", "symmetric"))

    fig, axes = plt.subplots(1, len(panels), figsize=(4.5 * len(panels), 4))
    if len(panels) == 1:
        axes = [axes]
    for ax, (title, image, cmap, scale) in zip(axes, panels):
        if scale == "sino":
            ax.imshow(image, aspect="auto", cmap=cmap, extent=[0, nz, angs[-1], angs[0]])
            ax.set_xlabel("detector pixel_x")
            ax.set_ylabel("angle (deg)")
        elif scale == "percentile":
            lo, hi = np.percentile(image, [2, 98])
            ax.imshow(image, origin="lower", cmap=cmap, vmin=lo, vmax=hi)
        elif scale == "symmetric":
            clim = np.percentile(np.abs(image), 98)
            if clim <= 0:
                clim = 1.0
            ax.imshow(image, origin="lower", cmap=cmap, vmin=-clim, vmax=clim)
        else:
            ax.imshow(image, origin="lower", cmap=cmap)
        ax.set_title(title)
    fig.tight_layout(); fig.savefig(C.FIGURES / f"fig_tomography_{args.tag}.png", dpi=130)
    print(f"[tomo] tag={args.tag} angles={len(angs)} -> sinogram/reconstruction/fig_tomography_{args.tag}")


if __name__ == "__main__":
    main()
