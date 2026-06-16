"""PGAI-NT fine-beam calibration and point concentration inversion.

Workflow:
  1. Run empty, PE, Al, Fe calibration blocks with the same fine beam and HPGe
     scan centers.
  2. Compute response columns as HPGe energy-window counts / NT uncollided flux.
  3. Run gradient-cylinder scan points.
  4. Normalize each point's HPGe windows by its NT flux and solve NNLS.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from scipy.optimize import nnls

import common as C
from build_hpge_spectrum import spectrum_and_windows


ROOT = C.ROOT
NT = ROOT / "build" / "NT"
DEFAULT_POINTS = [(-12.0, -12.0), (-12.0, 12.0), (12.0, -12.0), (12.0, 12.0)]
DEFAULT_MATERIALS = ["PE", "Al", "Fe"]
DEFAULT_WINDOWS = C.GAMMA_WINDOW_NAMES


@dataclass(frozen=True)
class PointMeasurement:
    point: str
    center_y_mm: float
    center_z_mm: float
    nt_flux: float
    windows: np.ndarray
    hpge_positive_events: int = 0
    nt_i0: float | None = None


def format_point_name(y_mm: float, z_mm: float) -> str:
    def fmt(v: float) -> str:
        text = f"{v:g}".replace(".", "p")
        return text

    return f"pt_y{fmt(y_mm)}_z{fmt(z_mm)}"


def write_macro_set(root: Path | str,
                    mode: str,
                    points: list[tuple[float, float]] = DEFAULT_POINTS,
                    events: int = 100000,
                    threads: int = 16,
                    material: str | None = None,
                    spot_size_mm: float = 2.0,
                    gamma_cone_bias: bool = True,
                    gamma_cone_angle_deg: float = 10.0) -> list[Path]:
    root = Path(root)
    paths = []
    for y_mm, z_mm in points:
        work = root / format_point_name(y_mm, z_mm)
        work.mkdir(parents=True, exist_ok=True)
        lines = [
            f"/run/numberOfThreads {threads}",
            "/run/verbose 0",
            "/event/verbose 0",
            "/tracking/verbose 0",
            f"/pgai/source/spotSize {spot_size_mm:g} mm",
            f"/pgai/source/centerY {y_mm:g} mm",
            f"/pgai/source/centerZ {z_mm:g} mm",
            f"/pgai/bias/gammaCone {'true' if gamma_cone_bias else 'false'}",
            f"/pgai/bias/gammaConeAngle {gamma_cone_angle_deg:g} deg",
            f"/pgai/phantom/mode {mode}",
        ]
        if material is not None:
            lines.append(f"/pgai/phantom/singleMaterial {material}")
        lines.extend([
            f"/pgai/hpge/centerY {y_mm:g} mm",
            f"/pgai/hpge/centerZ {z_mm:g} mm",
            "/run/initialize",
            f"/run/beamOn {events}",
        ])
        path = work / "run.mac"
        path.write_text("\n".join(lines) + "\n")
        paths.append(path)
    return paths


def write_full_macro_tree(root: Path | str,
                          events: int = 100000,
                          threads: int = 16,
                          points: list[tuple[float, float]] = DEFAULT_POINTS,
                          gamma_cone_bias: bool = True,
                          gamma_cone_angle_deg: float = 10.0) -> dict[str, Path]:
    root = Path(root)
    paths = {
        "empty": root / "empty",
        "gradient": root / "gradient",
    }
    write_macro_set(paths["empty"], mode="empty", points=points,
                    events=events, threads=threads,
                    gamma_cone_bias=gamma_cone_bias,
                    gamma_cone_angle_deg=gamma_cone_angle_deg)
    write_macro_set(paths["gradient"], mode="gradient_cylinder", points=points,
                    events=events, threads=threads,
                    gamma_cone_bias=gamma_cone_bias,
                    gamma_cone_angle_deg=gamma_cone_angle_deg)
    for material in DEFAULT_MATERIALS:
        key = f"cal_{material}"
        paths[key] = root / key
        write_macro_set(paths[key], mode="calibration_block", material=material,
                        points=points, events=events, threads=threads,
                        gamma_cone_bias=gamma_cone_bias,
                        gamma_cone_angle_deg=gamma_cone_angle_deg)
    return paths


def run_macro_tree(root: Path | str, groups: list[str] | None = None) -> None:
    root = Path(root)
    selected = set(groups) if groups else None
    for group_dir in sorted(p for p in root.iterdir() if p.is_dir()):
        if selected and group_dir.name not in selected:
            continue
        for point_dir in sorted(p for p in group_dir.iterdir() if p.is_dir()):
            macro = point_dir / "run.mac"
            if not macro.exists():
                continue
            for old in point_dir.glob("pgai_run0_nt_*.csv"):
                old.unlink()
            with open(point_dir / "log.txt", "w") as log:
                subprocess.run([str(NT), "run.mac"], cwd=point_dir,
                               stdout=log, stderr=subprocess.STDOUT, check=True)


def read_point_measurement(point_dir: Path | str,
                           center_y_mm: float,
                           center_z_mm: float,
                           nt_i0: float | None = None) -> PointMeasurement:
    point_dir = Path(point_dir)
    hpge = C.read_hpge(point_dir, 0)
    trans = C.read_transmission(point_dir, 0)

    energies = _hpge_signal_energies(hpge)
    _, _, windows = spectrum_and_windows(energies)
    nt_flux = _uncollided_unique_events(trans)

    return PointMeasurement(
        point=point_dir.name,
        center_y_mm=center_y_mm,
        center_z_mm=center_z_mm,
        nt_flux=float(nt_flux),
        windows=np.asarray(windows, dtype=float),
        hpge_positive_events=int(len(energies)),
        nt_i0=nt_i0,
    )


def read_measurement_group(group_dir: Path | str,
                           points: list[tuple[float, float]] = DEFAULT_POINTS,
                           empty_group: dict[str, PointMeasurement] | None = None) -> list[PointMeasurement]:
    group_dir = Path(group_dir)
    measurements = []
    for y_mm, z_mm in points:
        name = format_point_name(y_mm, z_mm)
        i0 = empty_group[name].nt_flux if empty_group and name in empty_group else None
        measurements.append(read_point_measurement(group_dir / name, y_mm, z_mm, nt_i0=i0))
    return measurements


def build_response_matrix(calibration: dict[str, list[PointMeasurement]],
                          materials: list[str] = DEFAULT_MATERIALS,
                          min_flux: float = 1.0,
                          min_window_counts: float = 5.0) -> np.ndarray:
    columns = []
    for material in materials:
        if material not in calibration:
            raise ValueError(f"missing calibration material {material}")
        yields = []
        counts = []
        for meas in calibration[material]:
            if meas.nt_flux >= min_flux:
                yields.append(meas.windows / meas.nt_flux)
                counts.append(meas.windows)
        if not yields:
            raise ValueError(f"no usable calibration flux for {material}")
        column = np.mean(np.vstack(yields), axis=0)
        total_counts = np.sum(np.vstack(counts), axis=0)
        column = np.where(total_counts >= float(min_window_counts), column, 0.0)
        columns.append(column)
    return np.column_stack(columns)


def build_nt_attenuation_vector(calibration: dict[str, list[PointMeasurement]],
                                materials: list[str] = DEFAULT_MATERIALS) -> np.ndarray:
    values = []
    for material in materials:
        if material not in calibration:
            raise ValueError(f"missing calibration material {material}")
        mus = []
        for meas in calibration[material]:
            if meas.nt_i0 and meas.nt_i0 > 0 and meas.nt_flux > 0:
                transmission = max(meas.nt_flux / meas.nt_i0, 1e-30)
                mus.append(-math.log(transmission))
        if not mus:
            raise ValueError(f"no usable NT attenuation for {material}")
        values.append(float(np.mean(mus)))
    return np.asarray(values, dtype=float)


def invert_measurements(measurements: list[PointMeasurement],
                        response: np.ndarray,
                        materials: list[str] = DEFAULT_MATERIALS,
                        nt_attenuation: np.ndarray | None = None,
                        nt_weight: float = 1.0) -> list[dict[str, float | str]]:
    rows: list[dict[str, float | str]] = []
    for meas in measurements:
        gamma_yield = meas.windows / max(meas.nt_flux, 1.0)
        gamma_only_density, gamma_only_residual = nnls(response, gamma_yield)
        density = gamma_only_density
        residual = gamma_only_residual
        nt_mu = _measurement_nt_mu(meas)
        if nt_attenuation is not None and nt_mu is not None:
            density, residual = _solve_pgai_nt_nnls(
                response=response,
                gamma_yield=gamma_yield,
                nt_attenuation=np.asarray(nt_attenuation, dtype=float),
                nt_mu=nt_mu,
                nt_weight=nt_weight,
            )
        total = float(density.sum())
        fractions = density / total if total > 0 else np.zeros_like(density)
        gamma_total = float(gamma_only_density.sum())
        gamma_fractions = (
            gamma_only_density / gamma_total if gamma_total > 0 else np.zeros_like(gamma_only_density)
        )
        truth = gradient_truth_fraction(meas.center_y_mm, meas.center_z_mm)

        row: dict[str, float | str] = {
            "point": meas.point,
            "center_y_mm": float(meas.center_y_mm),
            "center_z_mm": float(meas.center_z_mm),
            "nt_flux": float(meas.nt_flux),
            "nt_i0": float(meas.nt_i0) if meas.nt_i0 is not None else 0.0,
            "nt_transmission": float(meas.nt_flux / meas.nt_i0) if meas.nt_i0 else 0.0,
            "hpge_positive_events": int(meas.hpge_positive_events),
            "hpge_window_sum": float(meas.windows.sum()),
            "hpge_poisson_rel_1sigma": float(1.0 / math.sqrt(meas.windows.sum())) if meas.windows.sum() > 0 else float("inf"),
            "nnls_residual": float(residual),
            "gamma_only_nnls_residual": float(gamma_only_residual),
            "nt_mu": float(nt_mu) if nt_mu is not None else 0.0,
        }
        for idx, name in enumerate(DEFAULT_WINDOWS):
            row[name] = float(meas.windows[idx])
            row[f"{name}_per_nt"] = float(gamma_yield[idx])
        for idx, material in enumerate(materials):
            row[f"gamma_only_{material}_density"] = float(gamma_only_density[idx])
            row[f"gamma_only_{material}_fraction"] = float(gamma_fractions[idx])
            row[f"{material}_density"] = float(density[idx])
            row[f"{material}_fraction"] = float(fractions[idx])
            row[f"truth_{material}_fraction"] = float(truth[idx])
        rows.append(row)
    return rows


def analyze_run_tree(root: Path | str,
                     output_prefix: str = "pgai_nt",
                     points: list[tuple[float, float]] = DEFAULT_POINTS,
                     materials: list[str] = DEFAULT_MATERIALS) -> dict[str, object]:
    root = Path(root)
    empty_list = read_measurement_group(root / "empty", points)
    empty = {m.point: m for m in empty_list}
    calibration = {
        material: read_measurement_group(root / f"cal_{material}", points, empty)
        for material in materials
    }
    response = build_response_matrix(calibration, materials)
    nt_attenuation = build_nt_attenuation_vector(calibration, materials)
    gradient = read_measurement_group(root / "gradient", points, empty)
    rows = invert_measurements(gradient, response, materials, nt_attenuation=nt_attenuation)

    C.METRICS.mkdir(parents=True, exist_ok=True)
    C.FIGURES.mkdir(parents=True, exist_ok=True)
    np.save(C.METRICS / f"{output_prefix}_response_matrix.npy", response)
    _write_response_csv(C.METRICS / f"{output_prefix}_response_matrix.csv", response, materials)
    _write_nt_attenuation_csv(C.METRICS / f"{output_prefix}_nt_attenuation.csv", nt_attenuation, materials)
    _write_rows_csv(C.METRICS / f"{output_prefix}_concentration_points.csv", rows)
    _plot_concentration_rows(C.FIGURES / f"fig_{output_prefix}_concentration_points.png", rows, materials)

    summary = {
        "run_root": str(root),
        "materials": materials,
        "points": rows,
        "response_matrix": response.tolist(),
        "nt_attenuation": nt_attenuation.tolist(),
    }
    with open(C.METRICS / f"{output_prefix}_summary.json", "w") as f:
        json.dump(summary, f, indent=2)
    return summary


def gradient_truth_fraction(y_mm: float, z_mm: float, radius: float = 28.0) -> np.ndarray:
    rr = math.hypot(y_mm, z_mm)
    if rr > radius:
        return np.zeros(3, dtype=float)
    y_norm = np.clip((y_mm / radius + 1.0) * 0.5, 0.0, 1.0)
    z_norm = np.clip((z_mm / radius + 1.0) * 0.5, 0.0, 1.0)
    radial = np.clip(rr / radius, 0.0, 1.0)
    weights = np.array([
        0.62 * (1.0 - y_norm) + 0.18 * (1.0 - radial),
        0.55 * y_norm * (1.0 - 0.35 * z_norm) + 0.12,
        0.45 * z_norm + 0.35 * radial,
    ], dtype=float)
    return weights / weights.sum()


def _uncollided_unique_events(trans) -> int:
    if trans.empty:
        return 0
    col = "is_uncollided_primary" if "is_uncollided_primary" in trans.columns else "is_primary_neutron"
    prim = trans[trans[col] == 1][["event_id", "pixel_x", "pixel_y"]].drop_duplicates()
    return int(len(prim))


def _measurement_nt_mu(meas: PointMeasurement) -> float | None:
    if meas.nt_i0 is None or meas.nt_i0 <= 0 or meas.nt_flux <= 0:
        return None
    transmission = max(meas.nt_flux / meas.nt_i0, 1e-30)
    return -math.log(transmission)


def _solve_pgai_nt_nnls(response: np.ndarray,
                        gamma_yield: np.ndarray,
                        nt_attenuation: np.ndarray,
                        nt_mu: float,
                        nt_weight: float = 1.0) -> tuple[np.ndarray, float]:
    response = np.asarray(response, dtype=float)
    gamma_yield = np.asarray(gamma_yield, dtype=float)
    nt_attenuation = np.asarray(nt_attenuation, dtype=float)

    row_norm = np.linalg.norm(response, axis=1)
    valid = row_norm > 0
    if np.any(valid):
        a_gamma = response[valid] / row_norm[valid, None]
        b_gamma = gamma_yield[valid] / row_norm[valid]
    else:
        a_gamma = np.zeros((0, response.shape[1]), dtype=float)
        b_gamma = np.zeros(0, dtype=float)

    nt_norm = max(float(np.linalg.norm(nt_attenuation)), 1e-30)
    a_nt = float(nt_weight) * nt_attenuation[None, :] / nt_norm
    b_nt = np.asarray([float(nt_weight) * nt_mu / nt_norm], dtype=float)

    a = np.vstack([a_gamma, a_nt])
    b = np.concatenate([b_gamma, b_nt])
    return nnls(a, b)


def _hpge_signal_energies(hpge) -> np.ndarray:
    if hpge.empty:
        return np.array([], dtype=float)

    if "sample_gamma_edep_keV" in hpge.columns:
        sample = hpge.sample_gamma_edep_keV.astype(float).to_numpy()
        finite = sample[np.isfinite(sample)]
        if len(finite):
            return finite[finite > 0]

    energies = hpge.total_edep_keV.astype(float).to_numpy()
    return energies[np.isfinite(energies) & (energies > 0)]


def _write_response_csv(path: Path, response: np.ndarray, materials: list[str]) -> None:
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["window", *materials])
        for idx, name in enumerate(DEFAULT_WINDOWS):
            writer.writerow([name, *[float(response[idx, j]) for j in range(len(materials))]])


def _write_nt_attenuation_csv(path: Path, nt_attenuation: np.ndarray, materials: list[str]) -> None:
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["material", "nt_mu"])
        for idx, material in enumerate(materials):
            writer.writerow([material, float(nt_attenuation[idx])])


def _write_rows_csv(path: Path, rows: list[dict[str, float | str]]) -> None:
    if not rows:
        return
    with open(path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def _plot_concentration_rows(path: Path, rows: list[dict[str, float | str]],
                             materials: list[str]) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    labels = [str(r["point"]) for r in rows]
    x = np.arange(len(rows))
    width = 0.24
    fig, axes = plt.subplots(2, 1, figsize=(9, 6), sharex=True)
    for idx, material in enumerate(materials):
        axes[0].bar(x + (idx - 1) * width,
                    [float(r[f"{material}_fraction"]) for r in rows],
                    width=width, label=f"{material} recon")
        axes[1].bar(x + (idx - 1) * width,
                    [float(r[f"truth_{material}_fraction"]) for r in rows],
                    width=width, label=f"{material} truth")
    axes[0].set_ylabel("reconstructed fraction")
    axes[1].set_ylabel("truth fraction")
    axes[1].set_xticks(x, labels, rotation=25, ha="right")
    for ax in axes:
        ax.set_ylim(0, 1)
        ax.legend(ncol=3, fontsize=8)
    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


def _parse_points(values: list[float] | None) -> list[tuple[float, float]]:
    if not values:
        return DEFAULT_POINTS
    if len(values) % 2 != 0:
        raise SystemExit("--points requires y z pairs")
    return [(float(values[i]), float(values[i + 1])) for i in range(0, len(values), 2)]


def main() -> None:
    parser = argparse.ArgumentParser(description="PGAI-NT calibration and concentration inversion")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_write = sub.add_parser("write-macros")
    p_write.add_argument("--root", type=Path, default=C.RAW / "pgai_nt_calibration")
    p_write.add_argument("--events", type=int, default=100000)
    p_write.add_argument("--threads", type=int, default=16)
    p_write.add_argument("--points", type=float, nargs="*")
    p_write.add_argument("--gamma-cone-angle", type=float, default=10.0,
                         help="prompt-gamma cone-bias half-angle in degrees")
    p_write.add_argument("--disable-gamma-cone-bias", action="store_true",
                         help="disable prompt-gamma cone bias in generated macros")

    p_run = sub.add_parser("run")
    p_run.add_argument("--root", type=Path, default=C.RAW / "pgai_nt_calibration")
    p_run.add_argument("--groups", nargs="*",
                       help="optional subset: empty gradient cal_PE cal_Al cal_Fe")

    p_analyze = sub.add_parser("analyze")
    p_analyze.add_argument("--root", type=Path, default=C.RAW / "pgai_nt_calibration")
    p_analyze.add_argument("--output-prefix", default="pgai_nt")
    p_analyze.add_argument("--points", type=float, nargs="*")

    p_all = sub.add_parser("all")
    p_all.add_argument("--root", type=Path, default=C.RAW / "pgai_nt_calibration")
    p_all.add_argument("--events", type=int, default=100000)
    p_all.add_argument("--threads", type=int, default=16)
    p_all.add_argument("--points", type=float, nargs="*")
    p_all.add_argument("--output-prefix", default="pgai_nt")
    p_all.add_argument("--keep-existing", action="store_true")
    p_all.add_argument("--gamma-cone-angle", type=float, default=10.0,
                       help="prompt-gamma cone-bias half-angle in degrees")
    p_all.add_argument("--disable-gamma-cone-bias", action="store_true",
                       help="disable prompt-gamma cone bias in generated macros")

    args = parser.parse_args()

    if args.cmd == "write-macros":
        write_full_macro_tree(args.root, events=args.events, threads=args.threads,
                              points=_parse_points(args.points),
                              gamma_cone_bias=not args.disable_gamma_cone_bias,
                              gamma_cone_angle_deg=args.gamma_cone_angle)
        print(f"[pgai-nt] macros written -> {args.root}")
    elif args.cmd == "run":
        run_macro_tree(args.root, groups=args.groups)
        print(f"[pgai-nt] simulations complete -> {args.root}")
    elif args.cmd == "analyze":
        summary = analyze_run_tree(args.root, output_prefix=args.output_prefix,
                                   points=_parse_points(args.points))
        print(json.dumps(summary, indent=2))
    elif args.cmd == "all":
        if args.root.exists() and not args.keep_existing:
            shutil.rmtree(args.root)
        write_full_macro_tree(args.root, events=args.events, threads=args.threads,
                              points=_parse_points(args.points),
                              gamma_cone_bias=not args.disable_gamma_cone_bias,
                              gamma_cone_angle_deg=args.gamma_cone_angle)
        run_macro_tree(args.root)
        summary = analyze_run_tree(args.root, output_prefix=args.output_prefix,
                                   points=_parse_points(args.points))
        print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
