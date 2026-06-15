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
