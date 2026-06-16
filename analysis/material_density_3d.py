"""3D material concentration reconstruction from NT flux and HPGe windows.

Axis convention:
    truth/material volume: (material, x, y, z)
    neutron attenuation volume: (x, y, z)
    HPGe scan maps: (window, y, z)
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
from scipy.optimize import nnls


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT_ROOT = ROOT / "outputs"
DEFAULT_MATERIAL_NAMES = ("PE", "Al", "Fe")


def default_response_matrix() -> np.ndarray:
    """Synthetic HPGe window response for PE/Al/Fe-like components."""
    return np.array([
        [0.80, 0.18, 0.05],
        [0.36, 0.70, 0.18],
        [0.10, 0.36, 0.76],
        [0.03, 0.18, 0.88],
        [0.01, 0.08, 0.42],
    ], dtype=float)


def make_gradient_cylinder_truth(shape: tuple[int, int, int],
                                 radius: float,
                                 n_materials: int = 3) -> np.ndarray:
    """Build a synthetic gradient cylinder concentration phantom.

    The cylinder axis is x.  Concentrations vary smoothly in y/z inside the
    circular cross-section and are zero outside the target.
    """
    if len(shape) != 3:
        raise ValueError("shape must be (nx, ny, nz)")
    if n_materials != 3:
        raise ValueError("the built-in gradient phantom currently uses 3 materials")

    nx, ny, nz = (int(v) for v in shape)
    if nx <= 0 or ny <= 0 or nz <= 0:
        raise ValueError("shape dimensions must be positive")
    if radius <= 0:
        raise ValueError("radius must be positive")

    y = np.arange(ny, dtype=float) - (ny - 1) / 2.0
    z = np.arange(nz, dtype=float) - (nz - 1) / 2.0
    yy, zz = np.meshgrid(y, z, indexing="ij")
    rr = np.sqrt(yy * yy + zz * zz)
    mask = rr <= float(radius)

    y_norm = np.clip((yy / float(radius) + 1.0) * 0.5, 0.0, 1.0)
    z_norm = np.clip((zz / float(radius) + 1.0) * 0.5, 0.0, 1.0)
    radial = np.clip(rr / float(radius), 0.0, 1.0)

    fractions = np.stack([
        0.62 * (1.0 - y_norm) + 0.18 * (1.0 - radial),
        0.55 * y_norm * (1.0 - 0.35 * z_norm) + 0.12,
        0.45 * z_norm + 0.35 * radial,
    ], axis=0)
    fractions *= mask[None, :, :]
    total = fractions.sum(axis=0, keepdims=True)
    fractions = np.divide(fractions, total, out=np.zeros_like(fractions), where=total > 0)

    x = np.linspace(-1.0, 1.0, nx, dtype=float)
    axial_taper = (0.86 + 0.14 * np.cos(np.pi * x))[:, None, None]

    truth = fractions[:, None, :, :] * axial_taper[None, :, :, :]
    return np.asarray(truth, dtype=float)


def forward_gamma_counts(truth: np.ndarray,
                         response: np.ndarray,
                         flux: np.ndarray) -> np.ndarray:
    """Project material concentration to HPGe window counts.

    response has shape (windows, materials).  The HPGe yield is proportional
    to local neutron flux and material areal density along x.
    """
    truth = _as_material_volume(truth)
    response = np.asarray(response, dtype=float)
    flux = np.asarray(flux, dtype=float)

    n_materials = truth.shape[0]
    if response.ndim != 2 or response.shape[1] != n_materials:
        raise ValueError("response must have shape (windows, materials)")
    if flux.shape != truth.shape[2:4]:
        raise ValueError("flux must have shape (y, z)")

    areal_density = truth.sum(axis=1)
    gamma_yield = np.einsum("wm,myz->wyz", response, areal_density)
    return gamma_yield * flux[None, :, :]


def normalize_gamma_by_flux(counts: np.ndarray,
                            flux: np.ndarray,
                            min_flux: float = 1e-12) -> np.ndarray:
    """Convert HPGe counts to flux-normalized gamma yield."""
    counts = np.asarray(counts, dtype=float)
    flux = np.asarray(flux, dtype=float)
    if counts.ndim != 3:
        raise ValueError("counts must have shape (windows, y, z)")
    if flux.shape != counts.shape[1:3]:
        raise ValueError("flux must have shape (y, z)")

    out = np.zeros_like(counts, dtype=float)
    valid = flux > float(min_flux)
    np.divide(counts, flux[None, :, :], out=out, where=valid[None, :, :])
    return out


def solve_areal_density(gamma_yield: np.ndarray,
                        response: np.ndarray) -> np.ndarray:
    """Solve nonnegative material areal density at each y/z scan position."""
    gamma_yield = np.asarray(gamma_yield, dtype=float)
    response = np.asarray(response, dtype=float)
    if gamma_yield.ndim != 3:
        raise ValueError("gamma_yield must have shape (windows, y, z)")
    if response.ndim != 2 or response.shape[0] != gamma_yield.shape[0]:
        raise ValueError("response must have shape (windows, materials)")

    _, ny, nz = gamma_yield.shape
    n_materials = response.shape[1]
    recovered = np.zeros((n_materials, ny, nz), dtype=float)
    for iy in range(ny):
        for iz in range(nz):
            recovered[:, iy, iz], _ = nnls(response, gamma_yield[:, iy, iz])
    return recovered


def distribute_areal_density_to_volume(recovered: np.ndarray,
                                       nt_mu: np.ndarray,
                                       min_weight: float = 1e-12) -> np.ndarray:
    """Distribute per-column material areal density over x using NT weights."""
    recovered = np.asarray(recovered, dtype=float)
    nt_mu = np.asarray(nt_mu, dtype=float)
    if recovered.ndim != 3:
        raise ValueError("recovered must have shape (materials, y, z)")
    if nt_mu.ndim != 3 or nt_mu.shape[1:3] != recovered.shape[1:3]:
        raise ValueError("nt_mu must have shape (x, y, z)")

    weights = np.clip(nt_mu, 0.0, None)
    sums = weights.sum(axis=0, keepdims=True)
    weights = np.divide(weights, sums, out=np.zeros_like(weights), where=sums > min_weight)

    empty = sums[0] <= min_weight
    if np.any(empty):
        weights[:, empty] = 1.0 / nt_mu.shape[0]

    return recovered[:, None, :, :] * weights[None, :, :, :]


def reconstruct_material_volume(counts: np.ndarray,
                                flux: np.ndarray,
                                response: np.ndarray,
                                nt_mu: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Run the full HPGe/NT fusion reconstruction pipeline."""
    gamma_yield = normalize_gamma_by_flux(counts, flux)
    areal_density = solve_areal_density(gamma_yield, response)
    volume = distribute_areal_density_to_volume(areal_density, nt_mu)
    return volume, areal_density


def run_synthetic_reconstruction(output_root: Path | str = DEFAULT_OUTPUT_ROOT,
                                 shape: tuple[int, int, int] = (48, 56, 56),
                                 radius: float | None = None,
                                 noise: bool = False,
                                 seed: int = 12345,
                                 make_figure: bool = True,
                                 material_names: tuple[str, ...] = DEFAULT_MATERIAL_NAMES,
                                 response: np.ndarray | None = None) -> dict[str, object]:
    """Generate a gradient cylinder dataset, reconstruct it, and save outputs."""
    nx, ny, nz = (int(v) for v in shape)
    radius = float(radius if radius is not None else 0.40 * min(ny, nz))
    response = default_response_matrix() if response is None else np.asarray(response, dtype=float)
    truth = make_gradient_cylinder_truth((nx, ny, nz), radius=radius,
                                         n_materials=len(material_names))
    flux = synthetic_flux_map(ny, nz)
    counts = forward_gamma_counts(truth, response, flux)
    if noise:
        rng = np.random.default_rng(seed)
        counts = rng.poisson(np.clip(counts, 0.0, None)).astype(float)

    attenuation_coeffs = np.array([0.020, 0.060, 0.110], dtype=float)
    if len(material_names) != len(attenuation_coeffs):
        attenuation_coeffs = np.linspace(0.02, 0.11, len(material_names), dtype=float)
    nt_mu = np.einsum("m,mxyz->xyz", attenuation_coeffs, truth)

    volume, areal_density = reconstruct_material_volume(counts, flux, response, nt_mu)
    summary = save_reconstruction_outputs(
        volume=volume,
        areal_density=areal_density,
        output_root=output_root,
        material_names=material_names,
        truth=truth,
        flux=flux,
        counts=counts,
        nt_mu=nt_mu,
        make_figure=make_figure,
        metadata={
            "mode": "synthetic_gradient_cylinder",
            "shape": [nx, ny, nz],
            "radius": radius,
            "noise": bool(noise),
            "seed": int(seed),
        },
    )
    return summary


def run_array_reconstruction(counts: np.ndarray,
                             flux: np.ndarray,
                             response: np.ndarray,
                             nt_mu: np.ndarray,
                             output_root: Path | str = DEFAULT_OUTPUT_ROOT,
                             material_names: tuple[str, ...] = DEFAULT_MATERIAL_NAMES,
                             truth: np.ndarray | None = None,
                             make_figure: bool = True) -> dict[str, object]:
    """Reconstruct from externally supplied arrays and save outputs."""
    volume, areal_density = reconstruct_material_volume(counts, flux, response, nt_mu)
    return save_reconstruction_outputs(
        volume=volume,
        areal_density=areal_density,
        output_root=output_root,
        material_names=material_names,
        truth=truth,
        flux=flux,
        counts=counts,
        nt_mu=nt_mu,
        make_figure=make_figure,
        metadata={"mode": "array_input"},
    )


def synthetic_flux_map(ny: int, nz: int, base_flux: float = 2.5e5) -> np.ndarray:
    """Smooth nonuniform neutron flux map over the HPGe y/z scan plane."""
    yy, zz = np.meshgrid(np.linspace(-1.0, 1.0, ny),
                         np.linspace(-1.0, 1.0, nz),
                         indexing="ij")
    flux = float(base_flux) * (1.0 - 0.25 * yy + 0.15 * zz)
    return np.clip(flux, 0.05 * float(base_flux), None)


def save_reconstruction_outputs(volume: np.ndarray,
                                areal_density: np.ndarray,
                                output_root: Path | str,
                                material_names: tuple[str, ...],
                                truth: np.ndarray | None = None,
                                flux: np.ndarray | None = None,
                                counts: np.ndarray | None = None,
                                nt_mu: np.ndarray | None = None,
                                make_figure: bool = True,
                                metadata: dict[str, object] | None = None) -> dict[str, object]:
    """Write reconstructed volumes, optional intermediates, figure, and metrics."""
    volume = _as_material_volume(volume)
    output_root = Path(output_root)
    images = output_root / "images"
    figures = output_root / "figures"
    metrics = output_root / "metrics"
    for directory in (images, figures, metrics):
        directory.mkdir(parents=True, exist_ok=True)

    material_names = tuple(material_names)
    if len(material_names) != volume.shape[0]:
        raise ValueError("material_names length must match the material axis")

    volume_paths: dict[str, str] = {}
    for imat, name in enumerate(material_names):
        path = images / f"material_density_volume_{_safe_name(name)}.npy"
        np.save(path, volume[imat])
        volume_paths[name] = str(path)

    np.save(images / "material_density_areal_density.npy", np.asarray(areal_density, dtype=float))
    if truth is not None:
        np.save(images / "material_density_truth.npy", _as_material_volume(truth))
    if flux is not None:
        np.save(images / "material_density_flux.npy", np.asarray(flux, dtype=float))
    if counts is not None:
        np.save(images / "material_density_hpge_counts.npy", np.asarray(counts, dtype=float))
    if nt_mu is not None:
        np.save(images / "material_density_nt_mu.npy", np.asarray(nt_mu, dtype=float))

    summary = _summary_dict(volume, material_names, volume_paths, truth=truth,
                            metadata=metadata)
    if make_figure:
        fig_path = figures / "fig_3d_material_density_slices.png"
        plot_material_slices(volume, fig_path, material_names, truth=truth)
        summary["figure"] = str(fig_path)

    summary_path = metrics / "material_density_summary.json"
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2)
    return summary


def plot_material_slices(volume: np.ndarray,
                         path: Path | str,
                         material_names: tuple[str, ...],
                         truth: np.ndarray | None = None) -> None:
    """Save central x-slice plots for reconstructed material concentration."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    volume = _as_material_volume(volume)
    truth_arr = None if truth is None else _as_material_volume(truth)
    n_materials, nx, _, _ = volume.shape
    x_mid = nx // 2
    n_rows = 1 if truth_arr is None else 3

    fig, axes = plt.subplots(n_rows, n_materials,
                             figsize=(3.4 * n_materials, 3.1 * n_rows),
                             squeeze=False)
    for imat, name in enumerate(material_names):
        vmax = float(np.max(volume[imat]))
        if truth_arr is not None:
            vmax = max(vmax, float(np.max(truth_arr[imat])))
        vmax = vmax if vmax > 0 else 1.0

        axes[0, imat].imshow(volume[imat, x_mid].T, origin="lower",
                             cmap="viridis", vmin=0.0, vmax=vmax)
        axes[0, imat].set_title(f"{name} recon")
        if truth_arr is not None:
            axes[1, imat].imshow(truth_arr[imat, x_mid].T, origin="lower",
                                 cmap="viridis", vmin=0.0, vmax=vmax)
            axes[1, imat].set_title(f"{name} truth")
            err = np.abs(volume[imat, x_mid] - truth_arr[imat, x_mid])
            axes[2, imat].imshow(err.T, origin="lower", cmap="magma",
                                 vmin=0.0, vmax=max(float(np.max(err)), 1e-12))
            axes[2, imat].set_title(f"{name} abs error")
    for ax in axes.ravel():
        ax.set_xticks([])
        ax.set_yticks([])
    fig.tight_layout()
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=150)
    plt.close(fig)


def _summary_dict(volume: np.ndarray,
                  material_names: tuple[str, ...],
                  volume_paths: dict[str, str],
                  truth: np.ndarray | None = None,
                  metadata: dict[str, object] | None = None) -> dict[str, object]:
    summary: dict[str, object] = {
        "material_names": list(material_names),
        "volume_shape": list(volume.shape),
        "volume_paths": volume_paths,
        "total_reconstructed_density": [
            float(volume[imat].sum()) for imat in range(volume.shape[0])
        ],
    }
    if metadata:
        summary.update(metadata)
    if truth is not None:
        truth_arr = _as_material_volume(truth)
        diff = volume - truth_arr
        summary.update({
            "mean_absolute_error": float(np.mean(np.abs(diff))),
            "root_mean_square_error": float(np.sqrt(np.mean(diff * diff))),
            "max_absolute_error": float(np.max(np.abs(diff))),
            "total_true_density": [
                float(truth_arr[imat].sum()) for imat in range(truth_arr.shape[0])
            ],
        })
    return summary


def _safe_name(name: str) -> str:
    return "".join(ch if ch.isalnum() or ch in ("-", "_") else "_" for ch in name)


def _load_array(path: Path) -> np.ndarray:
    if path.suffix == ".npy":
        return np.load(path)
    if path.suffix == ".npz":
        data = np.load(path)
        first = data.files[0]
        return data[first]
    if path.suffix == ".json":
        with open(path) as f:
            return np.asarray(json.load(f), dtype=float)
    return np.loadtxt(path, delimiter=",")


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="3D material concentration reconstruction from HPGe and NT maps"
    )
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--shape", type=int, nargs=3, metavar=("NX", "NY", "NZ"),
                        default=(48, 56, 56))
    parser.add_argument("--radius", type=float, default=None)
    parser.add_argument("--noise", action="store_true",
                        help="add Poisson noise in synthetic mode")
    parser.add_argument("--seed", type=int, default=12345)
    parser.add_argument("--no-figure", action="store_true")
    parser.add_argument("--material-names", nargs="+", default=list(DEFAULT_MATERIAL_NAMES))
    parser.add_argument("--counts", type=Path,
                        help="HPGe counts array, shape (windows, y, z)")
    parser.add_argument("--flux", type=Path, help="NT flux array, shape (y, z)")
    parser.add_argument("--response", type=Path,
                        help="HPGe response matrix, shape (windows, materials)")
    parser.add_argument("--nt-mu", type=Path, help="NT attenuation volume, shape (x, y, z)")
    parser.add_argument("--truth", type=Path,
                        help="optional truth volume for metrics, shape (materials, x, y, z)")
    return parser.parse_args()


def main() -> None:
    args = _parse_args()
    material_names = tuple(args.material_names)
    make_figure = not args.no_figure

    array_args = [args.counts, args.flux, args.response, args.nt_mu]
    if any(v is not None for v in array_args):
        if not all(v is not None for v in array_args):
            raise SystemExit("--counts, --flux, --response, and --nt-mu must be provided together")
        truth = None if args.truth is None else _load_array(args.truth)
        summary = run_array_reconstruction(
            counts=_load_array(args.counts),
            flux=_load_array(args.flux),
            response=_load_array(args.response),
            nt_mu=_load_array(args.nt_mu),
            output_root=args.output_root,
            material_names=material_names,
            truth=truth,
            make_figure=make_figure,
        )
    else:
        summary = run_synthetic_reconstruction(
            output_root=args.output_root,
            shape=tuple(args.shape),
            radius=args.radius,
            noise=args.noise,
            seed=args.seed,
            make_figure=make_figure,
            material_names=material_names,
        )

    print(json.dumps(summary, indent=2))


def _as_material_volume(volume: np.ndarray) -> np.ndarray:
    arr = np.asarray(volume, dtype=float)
    if arr.ndim != 4:
        raise ValueError("volume must have shape (materials, x, y, z)")
    return arr


if __name__ == "__main__":
    main()
