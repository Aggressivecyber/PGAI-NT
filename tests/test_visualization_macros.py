from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def _read(name: str) -> str:
    return (ROOT / name).read_text()


def test_scan_concentration_visual_macro_sets_offset_and_draws_tracks() -> None:
    text = _read("vis_scan_concentration.mac")

    assert "/pgai/phantom/mode gradient_cylinder" in text
    assert "/pgai/source/spotSize 2 mm" in text
    assert "/pgai/source/centerY 12 mm" in text
    assert "/pgai/source/centerZ -12 mm" in text
    assert "/pgai/hpge/centerY 12 mm" in text
    assert "/pgai/hpge/centerZ -12 mm" in text
    assert "/tracking/storeTrajectory 1" in text
    assert "/run/beamOn 20" in text


def test_visual_switch_macros_rebuild_geometry_before_refreshing() -> None:
    gradient = _read("vis_switch_gradient_scan.mac")
    cttest = _read("vis_switch_cttest.mac")

    for text in (gradient, cttest):
        assert "/run/reinitializeGeometry" in text
        assert "/vis/drawVolume" in text
        assert "/vis/viewer/refresh" in text


def test_visual_view_macros_refresh_existing_viewer() -> None:
    for name in ("vis_view_xy.mac", "vis_view_xz.mac", "vis_view_yz.mac"):
        text = _read(name)
        assert "/vis/viewer/set/viewpointVector" in text
        assert "/vis/viewer/refresh" in text


if __name__ == "__main__":
    test_scan_concentration_visual_macro_sets_offset_and_draws_tracks()
    test_visual_switch_macros_rebuild_geometry_before_refreshing()
    test_visual_view_macros_refresh_existing_viewer()
