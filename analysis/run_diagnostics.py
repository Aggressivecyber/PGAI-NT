"""P0-2 诊断: 扫描所有 run, 输出数据链路健康度 CSV。

每 run 统计: 透射(uncollided/scattered neutron, edep events), HPGe(event/gamma-like/mean edep)
用于判断物理信号是否充足 (HPGe 每材料应 >几百事件才有意义)。
"""
from __future__ import annotations

import csv
import glob
from pathlib import Path

import common as C

# run_id -> (tag, mode, material) 与 run_batch RUNS 一致
RUN_MAP = {
    0: ("empty", "empty", "-"),
    1: ("single_PE", "single", "PE"),
    2: ("single_Al", "single", "Al"),
    3: ("single_Fe", "single", "Fe"),
    4: ("single_Cu", "single", "Cu"),
    5: ("single_Pb", "single", "Pb"),
    6: ("single_Ni", "single", "Ni"),
    7: ("degeneracy", "degeneracy", "mixed"),
    8: ("steel", "steel", "mixed"),
}


def count_csv(pattern):
    return len(glob.glob(pattern))


def main():
    rows = []
    for rid in sorted(RUN_MAP):
        tag, mode, mat = RUN_MAP[rid]
        df = C.read_transmission(C.RAW, rid)
        dh = C.read_hpge(C.RAW, rid)

        n_neutron = int((df.particle_name == "neutron").sum()) if not df.empty else 0
        n_uncollided = int((df.is_primary_neutron == 1).sum()) if not df.empty else 0
        n_scattered = int((df.is_scattered_neutron == 1).sum()) if not df.empty else 0
        n_edep = int((df.edep_keV > 0).sum()) if not df.empty else 0

        n_hpge = len(dh)
        n_gamma = int((dh.first_gamma_energy_keV > 0).sum()) if n_hpge else 0
        mean_edep = float(dh.total_edep_keV.mean()) if n_hpge else 0.0

        rows.append({
            "run_id": rid, "tag": tag, "mode": mode, "material": mat,
            "transmission_rows": len(df),
            "neutron_hits": n_neutron,
            "uncollided_neutron": n_uncollided,
            "scattered_neutron": n_scattered,
            "edep_events": n_edep,
            "hpge_events": n_hpge,
            "hpge_gamma_like": n_gamma,
            "mean_hpge_edep_keV": round(mean_edep, 1),
        })

    out = C.METRICS / "diagnostics.csv"
    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    print(f"{'run':>3} {'material':>8} {'uncollided':>10} {'scattered':>9} {'HPGe':>7} {'gamma':>6} {'mean_keV':>8}")
    for r in rows:
        flag = " ⚠低统计" if (r["material"] not in ("-", "mixed") and r["hpge_events"] < 200) else ""
        print(f"{r['run_id']:>3} {r['material']:>8} {r['uncollided_neutron']:>10} "
              f"{r['scattered_neutron']:>9} {r['hpge_events']:>7} {r['hpge_gamma_like']:>6} "
              f"{r['mean_hpge_edep_keV']:>8}{flag}")
    print(f"\n saved -> {out}")


if __name__ == "__main__":
    main()
