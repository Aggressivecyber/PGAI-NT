from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis"))

from material_density_3d import (  # noqa: E402
    distribute_areal_density_to_volume,
    forward_gamma_counts,
    make_gradient_cylinder_truth,
    normalize_gamma_by_flux,
    run_synthetic_reconstruction,
    solve_areal_density,
)


def _response_matrix() -> np.ndarray:
    return np.array([
        [0.80, 0.18, 0.05],
        [0.16, 0.72, 0.20],
        [0.04, 0.10, 0.75],
        [0.02, 0.25, 0.92],
    ], dtype=float)


def test_flux_normalized_hpge_recovers_material_areal_density() -> None:
    truth = make_gradient_cylinder_truth((24, 28, 20), radius=10.0)
    response = _response_matrix()

    yy, zz = np.meshgrid(np.linspace(-1, 1, 28), np.linspace(-1, 1, 20), indexing="ij")
    flux = 2.5e5 * (1.0 - 0.25 * yy + 0.15 * zz)
    counts = forward_gamma_counts(truth, response, flux)

    gamma_yield = normalize_gamma_by_flux(counts, flux)
    recovered = solve_areal_density(gamma_yield, response)
    expected = truth.sum(axis=1)

    assert recovered.shape == expected.shape
    assert np.mean(np.abs(recovered - expected)) < 1e-8


def test_areal_density_distribution_preserves_column_sums() -> None:
    truth = make_gradient_cylinder_truth((24, 28, 20), radius=10.0)
    response = _response_matrix()
    flux = np.full((28, 20), 1.0e5)
    counts = forward_gamma_counts(truth, response, flux)
    recovered = solve_areal_density(normalize_gamma_by_flux(counts, flux), response)
    nt_mu = 0.02 * truth[0] + 0.06 * truth[1] + 0.11 * truth[2]

    volume = distribute_areal_density_to_volume(recovered, nt_mu)

    assert volume.shape == truth.shape
    assert np.all(volume >= 0.0)
    assert np.allclose(volume.sum(axis=1), recovered)


def test_synthetic_reconstruction_writes_volume_outputs() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        summary = run_synthetic_reconstruction(
            output_root=root,
            shape=(12, 14, 10),
            radius=4.5,
            noise=False,
            make_figure=True,
        )

        assert summary["mean_absolute_error"] < 1e-8
        assert (root / "images" / "material_density_volume_PE.npy").exists()
        assert (root / "images" / "material_density_volume_Al.npy").exists()
        assert (root / "images" / "material_density_volume_Fe.npy").exists()
        assert (root / "figures" / "fig_3d_material_density_slices.png").exists()
        assert (root / "metrics" / "material_density_summary.json").exists()


if __name__ == "__main__":
    test_flux_normalized_hpge_recovers_material_areal_density()
    test_areal_density_distribution_preserves_column_sums()
    test_synthetic_reconstruction_writes_volume_outputs()
