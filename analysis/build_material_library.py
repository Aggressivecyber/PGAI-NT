"""由单材料标定 (empty + 每种材料) 构建材料响应库。

每材料特征: A_n (中子衰减), G1..G5 (HPGe 能窗计数比)
输出: outputs/library/material_library.json
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

import common as C
from build_transmission_image import primary_count_image
from build_hpge_spectrum import spectrum_and_windows


def material_region_mask(nx, ny, size_mm, r_mm=15.0):
    """单材料圆柱 (半径 r_mm) 在探测器 y-z 平面的像素掩膜。"""
    mask = np.zeros((nx, ny), dtype=bool)
    for ix in range(nx):
        y = C.pixel_to_coord(ix, nx, size_mm)
        for iy in range(ny):
            z = C.pixel_to_coord(iy, ny, size_mm)
            if y * y + z * z <= r_mm * r_mm:
                mask[ix, iy] = True
    return mask


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", type=Path, default=C.RAW)
    ap.add_argument("--empty-run", type=int, default=0)
    ap.add_argument("--thickness-mm", type=float, default=20.0)
    args = ap.parse_args()

    nx, ny = C.PIXELS_X, C.PIXELS_Y
    d0 = C.read_transmission(args.raw, args.empty_run)
    I0 = primary_count_image(d0, nx, ny)
    mask = material_region_mask(nx, ny, C.DETECTOR_SIZE_MM)
    I0_region = max(I0[mask].sum(), 1.0)

    # run_id 映射: 与 run_batch 默认一致 (1=PE,2=Al,3=Fe,4=Cu,5=Pb,6=Ni)
    run_map = {"PE": 1, "Al": 2, "Fe": 3, "Cu": 4, "Pb": 5, "Ni": 6}

    lib = {}
    for mat, rid in run_map.items():
        d = C.read_transmission(args.raw, rid)
        I = primary_count_image(d, nx, ny)
        I_region = I[mask].sum()
        A_n = -np.log(max(I_region, 1e-6) / I0_region) if I_region > 0 else 0.0

        dh = C.read_hpge(args.raw, rid)
        e = dh.total_edep_keV.values.astype(float) if (not dh.empty and "total_edep_keV" in dh) else np.array([])
        e = e[e > 0]
        _, _, wins = spectrum_and_windows(e)
        total = sum(wins) or 1.0
        Gn = [w / total for w in wins]  # 归一化能窗比

        lib[mat] = {
            "A_n": float(A_n), "mu_n": float(A_n / args.thickness_mm),  # μ_n = 线衰减系数 (1/mm), 材料属性
            "thickness_mm": args.thickness_mm,
            "n_hpge_events": int(len(e)),
            "G": [float(g) for g in Gn],
        }
        print(f"[lib] {mat}: A_n={A_n:.4f} mu_n={A_n/args.thickness_mm:.5f}/mm G={[f'{g:.3f}' for g in Gn]} n_hpge={len(e)}")

    # air: 近似无衰减
    lib["air"] = {"A_n": 0.0, "mu_n": 0.0, "thickness_mm": 0.0, "n_hpge_events": 0,
                  "G": [0.0] * len(C.GAMMA_WINDOWS)}

    with open(C.LIBRARY / "material_library.json", "w") as f:
        json.dump(lib, f, indent=2)
    print(f"[lib] saved -> {C.LIBRARY}/material_library.json")


if __name__ == "__main__":
    main()
