"""一键批处理: 跑全部 phantom 模式 + 生成透射图/能谱/材料库/识别/论文图。

用法:
    python analysis/run_batch.py                    # 验收 (events=5000)
    python analysis/run_batch.py --events 50000     # 正式统计量
    python analysis/run_batch.py --skip-sim         # 仅重跑分析 (复用已有 raw)
"""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

import common as C

ROOT = C.ROOT
NT = ROOT / "build" / "NT"

# (run_id, tag, mode, material_or_None)
RUNS = [
    (0, "empty",        "empty",     None),
    (1, "single_PE",    "single",    "PE"),
    (2, "single_Al",    "single",    "Al"),
    (3, "single_Fe",    "single",    "Fe"),
    (4, "single_Cu",    "single",    "Cu"),
    (5, "single_Pb",    "single",    "Pb"),
    (6, "single_Ni",    "single",    "Ni"),
    (7, "degeneracy",   "degeneracy", None),
    (8, "steel",        "steel",     None),
]


def gen_macro(work: Path, mode: str, material, events: int, thickness_mm=20.0,
              spot_mm=36.0):
    lines = [
        f"/pgai/phantom/mode {mode}",
        f"/pgai/source/spotSize {spot_mm} mm",   # 照射野覆盖样品投影区
    ]
    if material:
        lines.append(f"/pgai/phantom/singleMaterial {material}")
        lines.append(f"/pgai/phantom/singleThickness {thickness_mm} mm")
    lines += ["/pgai/run/angle 0 deg", "/run/initialize", f"/run/beamOn {events}"]
    (work / "run.mac").write_text("\n".join(lines) + "\n")


def run_one(rid, tag, mode, material, events, skip_sim):
    work = C.RAW / tag
    if skip_sim:
        return
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    gen_macro(work, mode, material, events)
    print(f"=== SIM run{rid} {tag} ({mode} {material}) {events} events ===")
    with open(work / "log.txt", "w") as logf:
        subprocess.run([str(NT), "run.mac"], cwd=work, stdout=logf, stderr=subprocess.STDOUT, check=True)
    # 归并: pgai_run0_* -> RAW/pgai_run{rid}_*
    for f in work.glob("pgai_run0_nt_*_t*.csv"):
        dst = C.RAW / f.name.replace("pgai_run0_", f"pgai_run{rid}_")
        shutil.copy(f, dst)


def fig2_source_spectrum():
    """Fig 2: 4.05 MeV 源在不同能量展宽下的分布 (理论高斯采样)。"""
    E = 4050.0  # keV
    spreads = [0.0, 0.01, 0.03, 0.05, 0.10]
    x = np.linspace(3500, 4600, 400)
    plt.figure(figsize=(8, 5))
    for s in spreads:
        sigma = s * E
        y = np.exp(-0.5 * ((x - E) / (sigma if sigma > 0 else 1.0)) ** 2)
        if s == 0:
            y = (np.abs(x - E) < 1).astype(float)
        plt.plot(x, y, label=f"spread={s*100:.0f}%")
    plt.axvline(E, color="k", ls="--", lw=0.8)
    plt.xlabel("Energy (keV)"); plt.ylabel("rel. intensity")
    plt.title("Fig 2: 4.05 MeV quasi-monoenergetic source")
    plt.legend(); plt.tight_layout()
    plt.savefig(C.FIGURES / "fig2_source_spectrum.png", dpi=130)


def fig1_geometry():
    """Fig 1: 双模态几何示意图。"""
    fig, ax = plt.subplots(figsize=(9, 4))
    ax.add_patch(plt.Circle((-800, 0), 15, color="gold"))               # 源
    ax.text(-800, 40, "4.05 MeV n source", ha="center")
    ax.add_patch(plt.Rectangle((-10, -15), 20, 30, color="gray"))       # phantom
    ax.text(0, -35, "phantom", ha="center")
    ax.add_patch(plt.Rectangle((35, -20), 10, 40, color="cyan", alpha=0.6))  # 闪烁体
    ax.text(40, -35, "transmission scint.", ha="center", fontsize=8)
    ax.add_patch(plt.Circle((0, 250), 25, color="blue", alpha=0.5))     # HPGe
    ax.text(0, 300, "HPGe", ha="center")
    ax.annotate("", xy=(-15, 0), xytext=(-785, 0), arrowprops=dict(arrowstyle="->", color="orange"))
    ax.annotate("", xy=(-7, 250), xytext=(0, 10), arrowprops=dict(arrowstyle="->", color="blue", lw=0.8))
    ax.set_xlim(-900, 100); ax.set_ylim(-60, 350); ax.set_aspect("equal")
    ax.set_title("Fig 1: dual-modal geometry (top view)")
    ax.set_xlabel("beam x (mm)")
    fig.tight_layout(); fig.savefig(C.FIGURES / "fig1_geometry.png", dpi=130)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--events", type=int, default=5000)
    ap.add_argument("--skip-sim", action="store_true")
    args = ap.parse_args()

    if not NT.exists():
        raise SystemExit(f"未找到可执行文件 {NT}, 请先 cmake --build build")

    # ---- Stage 1: 仿真 ----
    for rid, tag, mode, mat in RUNS:
        run_one(rid, tag, mode, mat, args.events, args.skip_sim)

    # ---- Stage 2: 分析 ----
    env = {"PYTHONPATH": str(ROOT / "analysis")}
    py = lambda mod: subprocess.run(
        ["python3", str(ROOT / "analysis" / mod)],
        env={**env, "PATH": __import__("os").environ["PATH"]}, check=False)

    # 材料库
    py("build_material_library.py")
    # 透射图 (degeneracy / steel)
    subprocess.run(["python3", str(ROOT / "analysis" / "build_transmission_image.py"),
                    "--sample-run", "7", "--tag", "degeneracy"], check=False)
    subprocess.run(["python3", str(ROOT / "analysis" / "build_transmission_image.py"),
                    "--sample-run", "8", "--tag", "steel"], check=False)
    # HPGe 谱 (各材料)
    spec_runs = [(1, "PE"), (2, "Al"), (3, "Fe"), (4, "Cu"), (5, "Pb"), (6, "Ni"), (7, "mixed")]
    for rid, tag in spec_runs:
        subprocess.run(["python3", str(ROOT / "analysis" / "build_hpge_spectrum.py"),
                        "--run", str(rid), "--tag", tag], check=False)
    # 材料识别 (三方法)
    py("material_identification.py")

    # ---- Stage 3: 论文图 ----
    fig1_geometry()
    fig2_source_spectrum()

    print("\n=== DONE ===")
    print(f"figures  -> {C.FIGURES}")
    print(f"metrics  -> {C.METRICS}")
    print(f"library  -> {C.LIBRARY}")
    summ = C.METRICS / "identification_summary.json"
    if summ.exists():
        print("summary:", json.loads(summ.read_text()))


if __name__ == "__main__":
    main()
