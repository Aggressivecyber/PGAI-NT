from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis"))

from pgai_nt_calibration import (  # noqa: E402
    PointMeasurement,
    build_response_matrix,
    format_point_name,
    invert_measurements,
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


if __name__ == "__main__":
    test_format_point_name_is_stable_for_negative_coordinates()
    test_write_macro_set_aligns_fine_beam_and_hpge_centers()
    test_response_matrix_and_inversion_recover_known_fractions()
