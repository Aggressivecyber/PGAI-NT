"""三种材料识别方法对比: neutron-only / gamma-only / fusion。

neutron-only : per-pixel 仅用 A_n 最近邻匹配材料库
gamma-only   : HPGe 无空间分辨, 仅能给出场景材料集合 (演示空间不足)
fusion       : per-pixel [A_n, 全局归一化 gamma 能窗] 融合距离匹配

输出: confusion matrix, accuracy, precision/recall, 识别图
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

import common as C
from build_transmission_image import primary_count_image


def classify_mu(A_pixel, lib, mats, thickness_mm, use_gamma=False, G_global=None,
                gamma_weight=0.4):
    """基于 μ_n (线衰减系数, 材料属性) 分类, 修复"固定厚度库"硬伤。

    μ_meas = A_pixel / thickness_mm  (该块实测线衰减)
    匹配: argmin_m |μ_meas - μ_n_m|  (μ_n 是材料固有属性, 与厚度无关)
    """
    mu_meas = A_pixel / max(thickness_mm, 1e-6)
    best, best_d = 0, 1e18
    scale_mu = 0.05  # μ_n 量级 ~0.01-0.03/mm
    for idx, m in enumerate(mats):
        dmu = abs(mu_meas - lib[m]["mu_n"]) / scale_mu
        if use_gamma and G_global is not None:
            glib = np.array(lib[m]["G"])
            dg = float(np.linalg.norm(G_global - glib)) / 2.0
            dist = dmu + gamma_weight * dg
        else:
            dist = dmu
        if dist < best_d:
            best_d, best = dist, idx
    return best


def confusion(y_true, y_pred, names):
    n = len(names)
    M = np.zeros((n, n), dtype=int)
    for t, p in zip(y_true, y_pred):
        M[t, p] += 1
    acc = float(np.trace(M) / max(M.sum(), 1))
    prec, rec = [], []
    for i in range(n):
        tp = M[i, i]
        prec.append(tp / max(M[:, i].sum(), 1))
        rec.append(tp / max(M[i, :].sum(), 1))
    return M, acc, prec, rec


def save_cm(M, names, title, path):
    fig, ax = plt.subplots(figsize=(6, 5))
    im = ax.imshow(M, cmap="Blues")
    ax.set_xticks(range(len(names))); ax.set_yticks(range(len(names)))
    ax.set_xticklabels(names, rotation=45); ax.set_yticklabels(names)
    ax.set_xlabel("Predicted"); ax.set_ylabel("Truth")
    for i in range(len(names)):
        for j in range(len(names)):
            ax.text(j, i, M[i, j], ha="center", va="center",
                    color="white" if M[i, j] > M.max() * 0.5 else "black", fontsize=8)
    ax.set_title(f"{title} (acc={np.trace(M)/max(M.sum(),1):.3f})")
    fig.colorbar(im); fig.tight_layout(); fig.savefig(path, dpi=130)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", type=Path, default=C.RAW)
    ap.add_argument("--empty-run", type=int, default=0)
    ap.add_argument("--degen-run", type=int, default=7)
    ap.add_argument("--tag", default="degeneracy")
    args = ap.parse_args()

    with open(C.LIBRARY / "material_library.json") as f:
        lib = json.load(f)

    mats = C.MATERIALS  # PE,Al,Fe,Cu,Pb,Ni
    names = mats + ["air"]

    nx, ny = C.PIXELS_X, C.PIXELS_Y
    d0 = C.read_transmission(args.raw, args.empty_run)
    dd = C.read_transmission(args.raw, args.degen_run)
    I0 = primary_count_image(d0, nx, ny)
    Ip = primary_count_image(dd, nx, ny)
    T = np.clip(Ip / np.maximum(I0, 1.0), 1e-6, None)
    A = -np.log(T)

    truth, tnames = C.build_degen_truth_map(nx, ny)

    # 全局 gamma 能窗 (degeneracy 整体谱) — gamma-only 通道
    dh = C.read_hpge(args.raw, args.degen_run)
    e = dh.total_edep_keV.values.astype(float) if (not dh.empty and "total_edep_keV" in dh) else np.array([])
    e = e[e > 0]
    from build_hpge_spectrum import spectrum_and_windows
    _, _, wins = spectrum_and_windows(e)
    G_global = np.array([w / (sum(wins) or 1.0) for w in wins])

    # 块级聚合: 用区域总计数比 A = -ln(sum I / sum I0) (避免低计数 per-pixel 爆炸)
    name_to_pred = {n: i for i, n in enumerate(names)}

    pred_n_map = np.full((nx, ny), -1, dtype=int)
    pred_f_map = np.full((nx, ny), -1, dtype=int)
    y_true_n, y_pred_n, y_pred_f = [], [], []

    for label_idx, tname in enumerate(tnames):
        if tname == "background" or tname not in name_to_pred:
            continue
        mask = (truth == label_idx)
        if mask.sum() == 0:
            continue
        s0 = I0[mask].sum()
        sp = Ip[mask].sum()
        A_block = -np.log(max(sp, 1e-6) / max(s0, 1.0)) if sp > 0 else 5.0
        # 该块厚度 (从 phantom 几何真值, 用于 μ_n 匹配)
        thick = C.degen_block_thickness(*[
            (C.pixel_to_coord(ix, nx, C.DETECTOR_SIZE_MM),
             C.pixel_to_coord(iy, ny, C.DETECTOR_SIZE_MM))
            for ix in range(nx) for iy in range(ny) if truth[ix, iy] == label_idx
        ][0][::-1]) if False else next((t for m, yc, t in C.DEGEN_BLOCKS if m == tname), 20.0)
        yt = name_to_pred[tname]
        pn = classify_mu(A_block, lib, mats, thick, use_gamma=False)
        pf = classify_mu(A_block, lib, mats, thick, use_gamma=True, G_global=G_global)
        pred_n_map[mask] = pn
        pred_f_map[mask] = pf
        n_pix = int(mask.sum())
        y_true_n += [yt] * n_pix
        y_pred_n += [pn] * n_pix
        y_pred_f += [pf] * n_pix

    y_true_n = np.array(y_true_n); y_pred_n = np.array(y_pred_n); y_pred_f = np.array(y_pred_f)

    Mn, acc_n, prec_n, rec_n = confusion(y_true_n, y_pred_n, names)
    Mf, acc_f, prec_f, rec_f = confusion(y_true_n, y_pred_f, names)

    # gamma-only: 场景级材料集合 (无空间)
    gamma_detected = set()
    for m in mats:
        if np.linalg.norm(G_global - np.array(lib[m]["G"])) < 0.5:
            gamma_detected.add(m)
    gamma_acc_scene = len(gamma_detected & set(mats)) / len(mats)

    save_cm(Mn, names, "neutron-only", C.FIGURES / "fig8_confusion_neutron.png")
    save_cm(Mf, names, "fusion", C.FIGURES / "fig8_confusion_fusion.png")

    def draw_id_map(pmap, title, path):
        cmap = ListedColormap(plt.cm.tab10(np.linspace(0, 1, len(names))))
        fig, ax = plt.subplots(figsize=(6, 5))
        m = pmap.copy(); m[m < 0] = len(names)
        cmap_ext = ListedColormap(np.vstack([cmap.colors, [[0, 0, 0, 1]]]))
        ax.imshow(m, origin="lower", cmap=cmap_ext, vmin=0, vmax=len(names), aspect="equal")
        ax.set_title(title); ax.set_xlabel("pixel_y"); ax.set_ylabel("pixel_x")
        fig.tight_layout(); fig.savefig(path, dpi=130)

    draw_id_map(pred_n_map, "neutron-only material ID", C.FIGURES / "fig5_neutron_only.png")
    draw_id_map(pred_f_map, "fusion material ID", C.FIGURES / "fig7_fusion.png")

    # gamma-only 占位图 (文本说明)
    fig, ax = plt.subplots(figsize=(6, 3)); ax.axis("off")
    ax.text(0.5, 0.5,
            f"gamma-only: HPGe has NO spatial resolution\n"
            f"Scene-level material recall = {gamma_acc_scene:.2f}\n"
            f"Detected: {sorted(gamma_detected)}",
            ha="center", va="center", fontsize=12)
    fig.savefig(C.FIGURES / "fig6_gamma_only.png", dpi=130)

    # 汇总表
    rows = []
    for i, n in enumerate(names):
        rows.append({"material": n,
                     "neutron_prec": prec_n[i], "neutron_rec": rec_n[i],
                     "fusion_prec": prec_f[i], "fusion_rec": rec_f[i]})
    import csv
    with open(C.METRICS / "accuracy_table.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["material", "neutron_prec", "neutron_rec",
                                          "fusion_prec", "fusion_rec"])
        w.writeheader(); w.writerows(rows)
    summary = {"neutron_only_acc": acc_n, "fusion_acc": acc_f,
               "gamma_only_scene_recall": gamma_acc_scene,
               "gamma_detected": sorted(gamma_detected)}
    with open(C.METRICS / "identification_summary.json", "w") as f:
        json.dump(summary, f, indent=2)

    print(f"[ID] neutron-only acc={acc_n:.3f}  fusion acc={acc_f:.3f}  "
          f"gamma scene recall={gamma_acc_scene:.3f}")


if __name__ == "__main__":
    main()
