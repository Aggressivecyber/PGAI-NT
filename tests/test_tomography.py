from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis"))

from run_tomography import (  # noqa: E402
    attenuation_from_counts,
    default_slab_half_width,
    fbp_reconstruct,
    gen_macro,
    remove_angle_invariant_background,
    sart_tv_reconstruct,
)


def test_low_stat_projection_keeps_valid_detector_bins() -> None:
    i0 = np.array([0.0, 2.0, 10.0, 14.0, 19.0, 40.0])
    sample = np.array([0.0, 1.0, 8.0, 7.0, 10.0, 20.0])

    attenuation = attenuation_from_counts(sample, i0)

    assert attenuation[0] == 0.0
    assert attenuation[1] == 0.0
    assert np.all(attenuation[2:] > 0.0)


def test_zero_sample_count_does_not_create_extreme_fbp_outlier() -> None:
    attenuation = attenuation_from_counts(
        np.array([0.0]),
        np.array([20.0]),
        min_i0=3.0,
    )

    assert 0.0 < attenuation[0] < 5.0


def test_cttest_uses_thicker_slab_to_accumulate_axial_statistics() -> None:
    assert default_slab_half_width("cttest", 128) == 37
    assert default_slab_half_width("degeneracy", 128) == 10


def test_tomography_macro_sets_threads_and_gradient_mode() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "run.mac"
        gen_macro(path, "gradient_cylinder", 45.0, 12345,
                  threads=12, energy=0.00405, energy_unit="eV")
        text = path.read_text()

    assert "/run/numberOfThreads 12" in text
    assert "/pgai/source/energy 0.00405 eV" in text
    assert "/pgai/source/spotSize 65 mm" in text
    assert "/pgai/phantom/mode gradient_cylinder" in text
    assert "/pgai/run/angle 45 deg" in text
    assert "/run/beamOn 12345" in text


def test_angle_invariant_background_subtraction_preserves_rotating_signal() -> None:
    background = np.array([[1.0, 2.0, 3.0]])
    moving = np.array([
        [0.0, 0.5, 0.0],
        [0.5, 0.0, 0.0],
        [0.0, 0.0, 0.5],
    ])
    contrast = remove_angle_invariant_background(background + moving)

    assert np.allclose(np.median(contrast, axis=0), 0.0)
    assert np.count_nonzero(np.abs(contrast) > 0.1) > 0


def _disk_phantom(n: int) -> np.ndarray:
    coords = np.arange(n) - (n - 1) / 2.0
    x, y = np.meshgrid(coords, coords, indexing="ij")
    phantom = np.zeros((n, n), dtype=float)
    phantom[x * x + y * y <= 18**2] = 0.20
    for i, value in enumerate([0.08, 0.16, 0.28, 0.38, 0.52, 0.30]):
        theta = np.deg2rad(i * 60.0)
        cx = 10.5 * np.cos(theta)
        cy = 10.5 * np.sin(theta)
        phantom[(x - cx) ** 2 + (y - cy) ** 2 <= 4.5**2] = value
    return phantom


def _project_image(image: np.ndarray, angles_deg: np.ndarray) -> np.ndarray:
    n = image.shape[0]
    center = (n - 1) / 2.0
    grid = np.arange(n) - center
    x, y = np.meshgrid(grid, grid, indexing="ij")
    rows = []
    for angle in np.deg2rad(angles_deg):
        t = x * np.sin(angle) + y * np.cos(angle) + center
        row = np.zeros(n, dtype=float)
        valid = (t >= 0) & (t < n - 1)
        t_valid = t[valid]
        values = image[valid]
        t0 = np.floor(t_valid).astype(int)
        weight = t_valid - t0
        np.add.at(row, t0, values * (1.0 - weight))
        np.add.at(row, t0 + 1, values * weight)
        rows.append(row)
    return np.vstack(rows)


def test_sart_tv_beats_fbp_on_sparse_noisy_phantom() -> None:
    rng = np.random.default_rng(4)
    n = 48
    angles = np.linspace(0, 180, 24, endpoint=False)
    truth = _disk_phantom(n)
    sinogram = _project_image(truth, angles)
    sinogram += rng.normal(scale=0.18, size=sinogram.shape)
    sinogram = np.clip(sinogram, 0.0, None)

    fbp = fbp_reconstruct(sinogram, angles, n_out=n, smooth_sigma=1.0)
    sart = sart_tv_reconstruct(
        sinogram,
        angles,
        n_out=n,
        iterations=8,
        relaxation=0.08,
        tv_weight=0.04,
        tv_iterations=1,
    )

    fbp_scale = np.linalg.lstsq(fbp.reshape(-1, 1), truth.ravel(), rcond=None)[0][0]
    sart_scale = np.linalg.lstsq(sart.reshape(-1, 1), truth.ravel(), rcond=None)[0][0]
    fbp_mse = np.mean((fbp * fbp_scale - truth) ** 2)
    sart_mse = np.mean((sart * sart_scale - truth) ** 2)

    assert sart_mse < fbp_mse * 0.90


if __name__ == "__main__":
    test_low_stat_projection_keeps_valid_detector_bins()
    test_zero_sample_count_does_not_create_extreme_fbp_outlier()
    test_cttest_uses_thicker_slab_to_accumulate_axial_statistics()
    test_tomography_macro_sets_threads_and_gradient_mode()
    test_angle_invariant_background_subtraction_preserves_rotating_signal()
    test_sart_tv_beats_fbp_on_sparse_noisy_phantom()
