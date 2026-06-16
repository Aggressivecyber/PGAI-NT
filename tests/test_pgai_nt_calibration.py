from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis"))

from pgai_nt_calibration import (  # noqa: E402
    PointMeasurement,
    _hpge_signal_energies,
    build_nt_attenuation_vector,
    build_response_matrix,
    format_point_name,
    invert_measurements,
    write_full_macro_tree,
    write_macro_set,
)


def test_format_point_name_is_stable_for_negative_coordinates() -> None:
    assert format_point_name(-12, 12) == "pt_y-12_z12"
    assert format_point_name(0, -4.5) == "pt_y0_z-4p5"


def test_write_macro_set_aligns_fine_beam_and_hpge_centers() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write_macro_set(root, mode="calibration_block", material="PE",
                        points=[(-12.0, 12.0)], events=123, threads=4)
        text = (root / "pt_y-12_z12" / "run.mac").read_text()

        assert "/run/numberOfThreads 4" in text
        assert "/pgai/source/spotSize 2 mm" in text
        assert "/pgai/source/centerY -12 mm" in text
        assert "/pgai/source/centerZ 12 mm" in text
        assert "/pgai/hpge/centerY -12 mm" in text
        assert "/pgai/hpge/centerZ 12 mm" in text
        assert "/pgai/phantom/mode calibration_block" in text
        assert "/pgai/phantom/singleMaterial PE" in text
        assert "/run/beamOn 123" in text


def test_write_macro_set_enables_prompt_gamma_cone_bias_by_default() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write_macro_set(root, mode="gradient_cylinder",
                        points=[(12.0, -12.0)], events=456, threads=2)
        text = (root / "pt_y12_z-12" / "run.mac").read_text()

        assert "/pgai/bias/gammaCone true" in text
        assert "/pgai/bias/gammaConeAngle 10 deg" in text


def test_write_macro_set_can_set_thermal_neutron_energy() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write_macro_set(root, mode="gradient_cylinder",
                        points=[(0.0, 0.0)], events=50, threads=2,
                        source_energy=0.00405, source_energy_unit="eV")
        text = (root / "pt_y0_z0" / "run.mac").read_text()

    assert "/pgai/source/energy 0.00405 eV" in text
    assert "/run/beamOn 50" in text


def test_write_full_macro_tree_passes_prompt_gamma_bias_options() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write_full_macro_tree(root, points=[(0.0, 0.0)], events=789, threads=3,
                              gamma_cone_bias=False, gamma_cone_angle_deg=6.0)
        text = (root / "gradient" / "pt_y0_z0" / "run.mac").read_text()

        assert "/pgai/bias/gammaCone false" in text
        assert "/pgai/bias/gammaConeAngle 6 deg" in text


def test_hpge_signal_prefers_sample_gamma_deposit_when_available() -> None:
    hpge = pd.DataFrame({
        "total_edep_keV": [500.0, 1200.0, 2400.0],
        "sample_gamma_edep_keV": [0.0, 2223.0, 0.0],
        "sample_gamma_first_energy_keV": [-1000.0, 2223.0, -1000.0],
    })

    energies = _hpge_signal_energies(hpge)

    assert np.allclose(energies, [2223.0])


def test_hpge_signal_does_not_fallback_when_new_sample_column_is_zero() -> None:
    hpge = pd.DataFrame({
        "total_edep_keV": [500.0, 1200.0, 2400.0],
        "sample_gamma_edep_keV": [0.0, 0.0, 0.0],
        "sample_gamma_first_energy_keV": [-1000.0, -1000.0, -1000.0],
    })

    energies = _hpge_signal_energies(hpge)

    assert len(energies) == 0


def test_response_matrix_and_inversion_recover_known_fractions() -> None:
    response_truth = np.array([
        [0.80, 0.18, 0.05],
        [0.36, 0.70, 0.18],
        [0.10, 0.36, 0.76],
        [0.03, 0.18, 0.88],
        [0.01, 0.08, 0.42],
    ], dtype=float)
    flux = 1000.0
    calibration = {
        "PE": [PointMeasurement("pe", 0, 0, flux, response_truth[:, 0] * flux)],
        "Al": [PointMeasurement("al", 0, 0, flux, response_truth[:, 1] * flux)],
        "Fe": [PointMeasurement("fe", 0, 0, flux, response_truth[:, 2] * flux)],
    }

    response = build_response_matrix(calibration, ["PE", "Al", "Fe"])
    fractions = np.array([0.2, 0.5, 0.3])
    sample = [
        PointMeasurement("mix", 0, 0, flux, response_truth @ fractions * flux),
    ]
    rows = invert_measurements(sample, response, ["PE", "Al", "Fe"])

    assert np.allclose(response, response_truth)
    assert np.allclose(
        [rows[0]["PE_fraction"], rows[0]["Al_fraction"], rows[0]["Fe_fraction"]],
        fractions,
    )


def test_response_matrix_suppresses_low_count_calibration_windows() -> None:
    flux = 1000.0
    calibration = {
        "PE": [PointMeasurement("pe", 0, 0, flux, np.array([1, 0, 0, 0, 0], dtype=float))],
        "Al": [PointMeasurement("al", 0, 0, flux, np.array([10, 0, 0, 0, 0], dtype=float))],
        "Fe": [PointMeasurement("fe", 0, 0, flux, np.array([12, 0, 0, 0, 0], dtype=float))],
    }

    response = build_response_matrix(calibration, ["PE", "Al", "Fe"],
                                     min_window_counts=5.0)

    assert response[0, 0] == 0.0
    assert response[0, 1] > 0.0
    assert response[0, 2] > 0.0


def test_pgai_nt_inversion_uses_nt_to_recover_gamma_silent_material() -> None:
    response_truth = np.array([
        [0.0, 0.80, 0.10],
        [0.0, 0.20, 0.90],
        [0.0, 0.00, 0.00],
        [0.0, 0.00, 0.00],
        [0.0, 0.00, 0.00],
    ], dtype=float)
    nt_truth = np.array([0.95, 0.45, 1.25], dtype=float)
    flux = 1000.0
    cal_flux = flux * np.exp(-nt_truth)
    calibration = {
        "PE": [PointMeasurement("pe", 0, 0, cal_flux[0],
                                np.zeros(5), nt_i0=flux)],
        "Al": [PointMeasurement("al", 0, 0, cal_flux[1],
                                response_truth[:, 1] * cal_flux[1], nt_i0=flux)],
        "Fe": [PointMeasurement("fe", 0, 0, cal_flux[2],
                                response_truth[:, 2] * cal_flux[2], nt_i0=flux)],
    }
    fractions = np.array([0.30, 0.45, 0.25])
    mixed_mu = float(nt_truth @ fractions)
    sample_flux = flux * np.exp(-mixed_mu)
    sample = [
        PointMeasurement("mix", 0, 0, sample_flux,
                         response_truth @ fractions * sample_flux,
                         nt_i0=flux),
    ]

    response = build_response_matrix(calibration, ["PE", "Al", "Fe"])
    nt_attenuation = build_nt_attenuation_vector(calibration, ["PE", "Al", "Fe"])
    rows = invert_measurements(sample, response, ["PE", "Al", "Fe"],
                               nt_attenuation=nt_attenuation)

    assert np.allclose(nt_attenuation, nt_truth)
    assert rows[0]["gamma_only_PE_fraction"] == 0.0
    assert np.allclose(
        [rows[0]["PE_fraction"], rows[0]["Al_fraction"], rows[0]["Fe_fraction"]],
        fractions,
        atol=1e-6,
    )


if __name__ == "__main__":
    test_format_point_name_is_stable_for_negative_coordinates()
    test_write_macro_set_aligns_fine_beam_and_hpge_centers()
    test_write_macro_set_enables_prompt_gamma_cone_bias_by_default()
    test_write_macro_set_can_set_thermal_neutron_energy()
    test_write_full_macro_tree_passes_prompt_gamma_bias_options()
    test_hpge_signal_prefers_sample_gamma_deposit_when_available()
    test_hpge_signal_does_not_fallback_when_new_sample_column_is_zero()
    test_response_matrix_and_inversion_recover_known_fractions()
    test_response_matrix_suppresses_low_count_calibration_windows()
    test_pgai_nt_inversion_uses_nt_to_recover_gamma_silent_material()
