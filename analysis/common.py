"""PGAI-NT 分析公共工具: CSV 分片读取、路径配置、phantom 几何真值。"""
from __future__ import annotations

import glob
import os
from pathlib import Path

import numpy as np
import pandas as pd

# ---- 路径 ----
ROOT = Path(__file__).resolve().parent.parent
RAW = ROOT / "outputs" / "raw"
IMAGES = ROOT / "outputs" / "images"
SPECTRA = ROOT / "outputs" / "spectra"
LIBRARY = ROOT / "outputs" / "library"
METRICS = ROOT / "outputs" / "metrics"
FIGURES = ROOT / "outputs" / "figures"

for d in (RAW, IMAGES, SPECTRA, LIBRARY, METRICS, FIGURES):
    d.mkdir(parents=True, exist_ok=True)

# ---- CSV 列定义 (与 RunAction / EventAction 一致) ----
TRANSMISSION_COLS = [
    "run_id", "event_id", "angle_deg", "pixel_x", "pixel_y", "edep_keV",
    "time_ns", "particle_name", "track_id", "parent_id", "creator_process",
    "kinetic_energy_MeV", "local_x_mm", "local_y_mm", "local_z_mm",
    "is_primary_neutron", "is_scattered_neutron",
]
HPGE_COLS = [
    "run_id", "event_id", "angle_deg", "total_edep_keV", "n_steps",
    "first_gamma_energy_keV", "particle_names", "dominant_creator_process",
    "detector_name",
]

# ---- 几何参数 (须与 PGAIConfig 默认一致) ----
DETECTOR_SIZE_MM = 60.0
PIXELS_X = 128
PIXELS_Y = 128

# HPGe 能窗 (keV)
GAMMA_WINDOWS = [
    (200, 1000),
    (1000, 2000),
    (2000, 4000),
    (4000, 7000),
    (7000, 10000),
]
GAMMA_WINDOW_NAMES = ["G1_0.2-1MeV", "G2_1-2MeV", "G3_2-4MeV", "G4_4-7MeV", "G5_7-10MeV"]

MATERIALS = ["PE", "Al", "Fe", "Cu", "Pb", "Ni"]


def _read_shards(pattern: str, columns: list[str]) -> pd.DataFrame:
    """合并 G4 MT 分片 csv (跳过 # header 注释)。"""
    files = sorted(glob.glob(pattern))
    if not files:
        return pd.DataFrame(columns=columns)
    frames = []
    for f in files:
        try:
            df = pd.read_csv(f, comment="#", header=None, names=columns)
        except pd.errors.EmptyDataError:
            continue
        if len(df):
            frames.append(df)
    if not frames:
        return pd.DataFrame(columns=columns)
    return pd.concat(frames, ignore_index=True)


def read_transmission(raw_dir: Path, run_id: int) -> pd.DataFrame:
    pat = str(raw_dir / f"pgai_run{run_id}_nt_transmission_t*.csv")
    return _read_shards(pat, TRANSMISSION_COLS)


def read_hpge(raw_dir: Path, run_id: int) -> pd.DataFrame:
    pat = str(raw_dir / f"pgai_run{run_id}_nt_hpge_events_t*.csv")
    return _read_shards(pat, HPGE_COLS)


def pixel_to_coord(pixel: np.ndarray, n_pix: int, size_mm: float) -> np.ndarray:
    """像素中心 -> mm 坐标 (相对探测器中心)。"""
    return (pixel + 0.5) / n_pix * size_mm - size_mm * 0.5


# ---- degeneracy phantom 真值 (须与 BuildMaterialDegeneracyPhantom 一致) ----
# (材料, 块中心 y mm, 厚度 mm) — 不同材料不同厚度, 制造中子透射退化
DEGEN_BLOCKS = [
    ("PE",  -18.0, 60.0),
    ("Al",  -10.8, 40.0),
    ("Fe",  -3.6, 15.0),
    ("Cu",   3.6, 12.0),
    ("Pb",  10.8, 10.0),
    ("air", 18.0, 40.0),
]
DEGEN_BLOCK_R_MM = 3.0


def degen_truth_label(y_mm: float, z_mm: float) -> str:
    """根据局部坐标返回 degeneracy phantom 的材料真值 (无块返回 'air'背景)。"""
    for mat, yc, _ in DEGEN_BLOCKS:
        if (y_mm - yc) ** 2 + z_mm ** 2 <= DEGEN_BLOCK_R_MM ** 2:
            return mat
    return "air"


def degen_block_thickness(y_mm: float, z_mm: float) -> float:
    """返回该坐标处 degeneracy 块的厚度 (mm), 无块返回 0。"""
    for mat, yc, thk in DEGEN_BLOCKS:
        if (y_mm - yc) ** 2 + z_mm ** 2 <= DEGEN_BLOCK_R_MM ** 2:
            return thk
    return 0.0


def build_degen_truth_map(nx: int = PIXELS_X, ny: int = PIXELS_Y,
                          size_mm: float = DETECTOR_SIZE_MM) -> np.ndarray:
    """(nx, ny) 材料索引图。pixel_x<-local.y (phantom y), pixel_y<-local.z。"""
    labels = np.full((nx, ny), -1, dtype=int)
    names = ["background"] + [b[0] for b in DEGEN_BLOCKS]
    for ix in range(nx):
        y_mm = pixel_to_coord(ix, nx, size_mm)   # pixel_x -> y
        for iy in range(ny):
            z_mm = pixel_to_coord(iy, ny, size_mm)  # pixel_y -> z
            mat = degen_truth_label(y_mm, z_mm)
            labels[ix, iy] = names.index(mat) if mat in names else 0
    return labels, names
