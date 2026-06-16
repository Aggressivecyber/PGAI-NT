from __future__ import annotations

import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MIN_CLEARANCE_MM = 5.0


def _read(path: str) -> str:
    return (ROOT / path).read_text()


def _config_mm(name: str) -> float:
    text = _read("include/PGAIConfig.hh")
    match = re.search(rf"{name}\s*=\s*([0-9.]+)\s*\*\s*mm", text)
    if not match:
        raise AssertionError(f"could not find {name} in PGAIConfig.hh")
    return float(match.group(1))


def _sample_stage_max_x_extent_mm() -> float:
    text = _read("src/DetectorConstruction.cc")

    tubs = re.search(
        r'G4Tubs\("SampleStage",\s*0,\s*([0-9.]+)\s*\*\s*mm,',
        text,
    )
    if tubs:
        return float(tubs.group(1))

    box = re.search(
        r'G4Box\("SampleStage",\s*([0-9.]+)\s*\*\s*mm,\s*'
        r"([0-9.]+)\s*\*\s*mm,",
        text,
    )
    if box:
        half_x = float(box.group(1))
        half_y = float(box.group(2))
        return math.hypot(half_x, half_y)

    raise AssertionError("could not find SampleStage solid definition")


def _cttest_value(name: str) -> float:
    text = _read("src/DetectorConstruction.cc")
    match = re.search(rf"{name}\s*=\s*([0-9.]+)\s*\*\s*mm", text)
    if not match:
        raise AssertionError(f"could not find {name} in cttest geometry")
    return float(match.group(1))


def test_rotating_sample_stage_never_overlaps_transmission_screen() -> None:
    detector_distance = _config_mm("detectorDistance")
    scint_thickness = _config_mm("scintThickness")
    window_thickness = _config_mm("screenWindowThk")
    frame = _config_mm("screenFrame")
    carrier_half_depth = (scint_thickness + 2.0 * window_thickness) * 0.5 + frame
    screen_front_x = detector_distance - carrier_half_depth

    stage_extent = _sample_stage_max_x_extent_mm()
    clearance = screen_front_x - stage_extent

    assert clearance >= MIN_CLEARANCE_MM, (
        "rotating SampleStage can overlap the transmission screen: "
        f"stage_extent={stage_extent:.1f} mm, screen_front={screen_front_x:.1f} mm, "
        f"clearance={clearance:.1f} mm"
    )


def test_cttest_phantom_fills_about_80_percent_of_fov() -> None:
    detector_size = _config_mm("detectorSize")
    r_big = _cttest_value("Rbig")
    r_ring = _cttest_value("Rring")
    r_small = _cttest_value("Rsmall")

    assert math.isclose(2.0 * r_big / detector_size, 0.8, abs_tol=0.02)
    assert r_ring + r_small < r_big


def test_cttest_outer_cylinder_uses_aluminum_shell() -> None:
    detector = _read("src/DetectorConstruction.cc")

    assert 'auto Al = G4Material::GetMaterial("G4_Al")' in detector
    assert 'new G4LogicalVolume(solidBig, Al, "CTPhantomLV")' in detector


def test_hpge_scan_supports_yz_and_moves_collimator_with_detector() -> None:
    config = _read("include/PGAIConfig.hh")
    header = _read("include/PGAIMessenger.hh")
    messenger = _read("src/PGAIMessenger.cc")
    detector = _read("src/DetectorConstruction.cc")

    assert "hpgeCenterY" in config
    assert "hpgeCenterZ" in config
    assert "fCmdHPGeCenterY" in header
    assert "fCmdHPGeCenterZ" in header
    assert '"/pgai/hpge/centerY"' in messenger
    assert '"/pgai/hpge/centerZ"' in messenger
    assert "gConfig.hpgeCenterZ" in messenger

    assert "G4ThreeVector scanOffset(0, gConfig.hpgeCenterY, gConfig.hpgeCenterZ)" in detector
    assert "collimPos = G4ThreeVector(0, housingFront - cLen * 0.5, 0) + scanOffset" in detector


def test_source_beam_center_can_follow_hpge_scan_point() -> None:
    config = _read("include/PGAIConfig.hh")
    header = _read("include/PGAIMessenger.hh")
    messenger = _read("src/PGAIMessenger.cc")
    primary = _read("src/PrimaryGeneratorAction.cc")

    assert "sourceCenterY" in config
    assert "sourceCenterZ" in config
    assert "fCmdSourceCenterY" in header
    assert "fCmdSourceCenterZ" in header
    assert '"/pgai/source/centerY"' in messenger
    assert '"/pgai/source/centerZ"' in messenger
    assert "gConfig.sourceCenterY" in messenger
    assert "gConfig.sourceCenterZ" in messenger
    assert "G4ThreeVector srcPos(-gConfig.sourceDistance, gConfig.sourceCenterY, gConfig.sourceCenterZ)" in primary


def test_gradient_cylinder_phantom_mode_is_available() -> None:
    header = _read("include/DetectorConstruction.hh")
    messenger = _read("src/PGAIMessenger.cc")
    detector = _read("src/DetectorConstruction.cc")

    assert "BuildGradientCylinderPhantom" in header
    assert "gradient_cylinder" in messenger
    assert 'mode == "gradient_cylinder"' in detector
    assert "GradientCylinder" in detector


def test_calibration_block_phantom_covers_scan_intersection_volume() -> None:
    header = _read("include/DetectorConstruction.hh")
    messenger = _read("src/PGAIMessenger.cc")
    detector = _read("src/DetectorConstruction.cc")

    assert "BuildCalibrationBlockPhantom" in header
    assert "calibration_block" in messenger
    assert 'mode == "calibration_block"' in detector
    assert "CalibrationBlock" in detector
    assert "blockX = 36.0 * mm" in detector
    assert "blockY = 52.0 * mm" in detector
    assert "blockZ = 52.0 * mm" in detector
    assert "GetPhantomMaterial(gConfig.singleMaterial)" in detector


def test_hpge_collimator_is_long_and_close_to_target() -> None:
    hpge_distance = _config_mm("hpgeDistance")
    hpge_h = _config_mm("hpgeH")
    hpge_vacuum_gap = _config_mm("hpgeVacuumGap")
    collim_len = _config_mm("collimLen")
    hole_front = _config_mm("collimHoleFront")
    hole_back = _config_mm("collimHoleBack")

    housing_front = hpge_distance - (hpge_h * 0.5 + hpge_vacuum_gap)
    collim_front = housing_front - collim_len
    half_angle_deg = math.degrees(math.atan2(hole_back - hole_front, collim_len))

    assert collim_len >= 70.0
    assert 38.0 <= collim_front <= 45.0
    assert hole_front <= 2.0
    assert hole_back <= 6.0
    assert half_angle_deg <= 3.0


def test_hpge_collimator_small_aperture_faces_sample() -> None:
    detector = _read("src/DetectorConstruction.cc")

    assert (
        'G4Cons("CollimHole", 0, gConfig.collimHoleFront,\n'
        '\t                                  0, gConfig.collimHoleBack'
    ) in detector
    assert "rotateX(90 * CLHEP::deg);  // Cons +z(HPGe-side larger hole) -> world +y" in detector


def test_hpge_scores_sample_origin_gamma_signal() -> None:
    track_info = _read("include/PGAITrackInfo.hh")
    hit = _read("include/HPGeHit.hh")
    stepping = _read("src/SteppingAction.cc")
    hpge_sd = _read("src/HPGeSpectrometerSD.cc")
    event = _read("src/EventAction.cc")
    run = _read("src/RunAction.cc")
    common = _read("analysis/common.py")

    assert "bornInPhantom" in track_info
    assert "sourceGammaEnergyKeV" in track_info
    assert "fromPhantomGamma" in hit
    assert "sampleGammaEdep" in hit

    assert "GetSecondaryInCurrentStep" in stepping
    assert "CopyInfoToTrack" in stepping
    assert "sourceGammaEnergyKeV = secondary->GetKineticEnergy() / keV" in stepping

    assert "PGAITrackInfo" in hpge_sd
    assert "hit->fromPhantomGamma = info && info->bornInPhantom" in hpge_sd

    assert "sampleGammaEDep" in event
    assert "sample_gamma_edep_keV" in run
    assert "sample_gamma_first_energy_keV" in run
    assert "sample_gamma_hit_count" in run
    assert '"sample_gamma_edep_keV"' in common


def test_prompt_gamma_cone_bias_is_configurable_and_changes_gamma_direction() -> None:
    config = _read("include/PGAIConfig.hh")
    header = _read("include/PGAIMessenger.hh")
    messenger = _read("src/PGAIMessenger.cc")
    stepping = _read("src/SteppingAction.cc")
    run = _read("src/RunAction.cc")
    common = _read("analysis/common.py")

    assert "gammaConeBias" in config
    assert "gammaConeBiasAngle" in config
    assert "fDirBias" in header
    assert "fCmdGammaConeBias" in header
    assert "fCmdGammaConeBiasAngle" in header
    assert '"/pgai/bias/gammaCone"' in messenger
    assert '"/pgai/bias/gammaConeAngle"' in messenger

    assert "RandomDirectionInCone" in stepping
    assert "ApplyPromptGammaConeBias" in stepping
    assert "secondary->SetMomentumDirection" in stepping
    assert "sourceBiasWeight" in stepping

    assert "sample_gamma_bias_weight" in run
    assert '"sample_gamma_bias_weight"' in common


if __name__ == "__main__":
    test_rotating_sample_stage_never_overlaps_transmission_screen()
    test_cttest_phantom_fills_about_80_percent_of_fov()
    test_cttest_outer_cylinder_uses_aluminum_shell()
    test_hpge_scan_supports_yz_and_moves_collimator_with_detector()
    test_source_beam_center_can_follow_hpge_scan_point()
    test_gradient_cylinder_phantom_mode_is_available()
    test_calibration_block_phantom_covers_scan_intersection_volume()
    test_hpge_collimator_is_long_and_close_to_target()
    test_hpge_collimator_small_aperture_faces_sample()
    test_hpge_scores_sample_origin_gamma_signal()
    test_prompt_gamma_cone_bias_is_configurable_and_changes_gamma_direction()
