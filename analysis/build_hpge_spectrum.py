"""由 HPGe event-level CSV 构建瞬发伽马能谱 + 能窗特征。

输入: hpge_events csv (event 级 total_edep_keV)
输出: 谱直方图 (.npy/.png), 能窗计数 (用于材料库/识别)
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

import common as C


def spectrum_and_windows(edep_keV, bins=np.linspace(0, 10000, 501)):
    hist, edges = np.histogram(edep_keV, bins=bins)
    centers = 0.5 * (edges[:-1] + edges[1:])
    wins = []
    for lo, hi in C.GAMMA_WINDOWS:
        wins.append(float(np.sum((edep_keV >= lo) & (edep_keV < hi))))
    return centers, hist, wins


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", type=Path, default=C.RAW)
    ap.add_argument("--run", type=int, required=True)
    ap.add_argument("--tag", required=True)
    ap.add_argument("--smear-keV", type=float, default=0.0,
                    help="额外高斯展宽 sigma (keV), 0=关")
    args = ap.parse_args()

    df = C.read_hpge(args.raw, args.run)
    if df.empty or "total_edep_keV" not in df:
        print(f"[hpge] {args.tag}: 无 HPGe 事件")
        e = np.array([], dtype=float)
    else:
        e = df.total_edep_keV.values.astype(float)
        e = e[e > 0]

    if args.smear_keV > 0 and len(e):
        e_smeared = e + np.random.normal(0, args.smear_keV, size=e.size)
        e_smeared = np.clip(e_smeared, 0, None)
    else:
        e_smeared = e

    centers, hist, wins = spectrum_and_windows(e_smeared)
    np.save(C.SPECTRA / f"spectrum_{args.tag}.npy", np.vstack([centers, hist]))
    feat = {name: w for name, w in zip(C.GAMMA_WINDOW_NAMES, wins)}
    feat["n_events"] = int(len(e))
    feat["mean_edep_keV"] = float(e.mean()) if len(e) else 0.0
    with open(C.SPECTRA / f"features_{args.tag}.json", "w") as f:
        json.dump(feat, f, indent=2)

    plt.figure(figsize=(8, 5))
    plt.step(centers, hist, where="mid", label=args.tag)
    for (lo, hi), name in zip(C.GAMMA_WINDOWS, C.GAMMA_WINDOW_NAMES):
        plt.axvspan(lo, hi, alpha=0.08)
    plt.xlabel("Energy (keV)"); plt.ylabel("counts / event")
    plt.title(f"HPGe prompt-gamma spectrum: {args.tag}")
    plt.legend(); plt.tight_layout()
    plt.savefig(C.FIGURES / f"hpge_spectrum_{args.tag}.png", dpi=130)
    print(f"[hpge] {args.tag}: n_events={len(e)} windows={wins}")


if __name__ == "__main__":
    main()
