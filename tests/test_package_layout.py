#!/usr/bin/env python3
"""Structural tests for modular packages + hard-fork branding.

Drives the real package/source tree on disk (not a reimplementation of
ESPHome). Fails if connectivity/fleet_api leak into mandatory base, if the
example is not an explicit package map, or if Pedro ownership branding
returns outside allowed dependency/credit exceptions.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_PACKAGES = [
    "packages/core/base.yml",
    "packages/core/diagnostics.yml",
    "packages/core/ble_tracker.yml",
    "packages/connectivity/wifi.yml",
    "packages/connectivity/ethernet-lan8720.yml",
    "packages/features/vehicle.yml",
    "packages/features/fleet_api.yml",
    "packages/features/listener.yml",
    "packages/boards/esp32dev.yml",
]

FORBIDDEN_TOP_LEVEL_IN_BASE = re.compile(
    r"^(wifi|ethernet|tesla_fleet_api_server|tesla_ble_vehicle|ble_client)\s*:",
    re.MULTILINE,
)

# Ownership branding: Pedro must not appear as project owner. Allowed:
# - modern.md (design notes)
# - packages/core/base.yml BLE_library dependency URL
# - one historical library credit line in README.md
PEDRO_RE = re.compile(r"PedroKTFC|pedroktfc|\bPedro\b")

ALLOWED_PEDRO_PATHS = {
    Path("modern.md"),
    Path("packages/core/base.yml"),
    Path("README.md"),
    Path("tests/test_package_layout.py"),
}


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        fail(f"missing required file {rel}")
    return path.read_text(encoding="utf-8")


def test_required_packages_exist() -> None:
    for rel in REQUIRED_PACKAGES:
        if not (ROOT / rel).is_file():
            fail(f"missing package {rel}")
    print("OK required packages exist")


def test_base_is_minimal() -> None:
    base = read("packages/core/base.yml")
    m = FORBIDDEN_TOP_LEVEL_IN_BASE.search(base)
    if m:
        fail(f"base.yml must not define top-level {m.group(1)}:")
    for needle in ("wifi:", "ethernet:", "tesla_fleet_api_server:", "tesla_ble_vehicle:"):
        # external_components may list component *names*; forbid config keys only
        if re.search(rf"^{re.escape(needle)}", base, re.MULTILINE):
            fail(f"base.yml must not configure {needle}")
    if "Jabe.esphome-tesla" not in base:
        fail("base.yml project name must be Jabe.esphome-tesla")
    if "PedroKTFC.esphome-tesla-ble" in base:
        fail("base.yml still uses Pedro project name")
    print("OK base is minimal and Jabe-branded")


def test_connectivity_and_fleet_split() -> None:
    wifi = read("packages/connectivity/wifi.yml")
    eth = read("packages/connectivity/ethernet-lan8720.yml")
    fleet = read("packages/features/fleet_api.yml")
    vehicle = read("packages/features/vehicle.yml")
    if not re.search(r"^wifi\s*:", wifi, re.MULTILINE):
        fail("wifi package missing wifi:")
    if not re.search(r"^ethernet\s*:", eth, re.MULTILINE):
        fail("ethernet package missing ethernet:")
    if "LAN8720" not in eth:
        fail("ethernet package missing LAN8720 defaults")
    for pin in ("GPIO23", "GPIO18", "GPIO17", "GPIO12", "CLK_OUT"):
        if pin not in eth:
            fail(f"ethernet package missing default pin/mode {pin}")
    if "clk_mode: GPIO17_OUT" in eth:
        fail("ethernet package still uses deprecated clk_mode enum form")
    if not re.search(r"^tesla_fleet_api_server\s*:", fleet, re.MULTILINE):
        fail("fleet_api package missing tesla_fleet_api_server:")
    if re.search(r"^tesla_fleet_api_server\s*:", vehicle, re.MULTILINE):
        fail("vehicle package must not include tesla_fleet_api_server:")
    if re.search(r"^(wifi|ethernet)\s*:", vehicle, re.MULTILINE):
        fail("vehicle package must not include network config")
    print("OK connectivity and fleet_api are optional packages")


def test_example_is_package_map() -> None:
    example = read("tesla-ble.example.yml")
    if "packages:" not in example:
        fail("example missing packages:")
    # Must not use old multi-file bundle of base+common+project+client
    if re.search(r"files:\s*\[.*base\.yml", example):
        fail("example still uses multi-file package bundle")
    for key in ("base:", "board:", "network:", "vehicle:"):
        if key not in example:
            fail(f"example shopping list missing {key}")
    if "github://Jabe/esphome-tesla" not in example:
        fail("example must point remote packages at Jabe")
    if "PedroKTFC" in example:
        fail("example still references PedroKTFC remotes")
    # Optional picks documented
    for opt in ("fleet_api", "wifi.yml", "ethernet-lan8720", "listener", "language"):
        if opt not in example:
            fail(f"example should document optional pick involving {opt}")
    local = read("tesla-ble.local.yml")
    if "!include packages/core/base.yml" not in local:
        fail("local example should !include modular packages")
    print("OK example is explicit package shopping list")


def test_packages_are_unopinionated_about_secrets() -> None:
    """Remote packages must not hardcode !secret names; values come from the
    node config via top-level substitutions or package vars."""
    hits: list[str] = []
    for path in sorted((ROOT / "packages").rglob("*.yml")):
        rel = path.relative_to(ROOT)
        for i, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            code = line.split("#", 1)[0]  # comments may mention !secret
            if "!secret" in code:
                hits.append(f"{rel}:{i}:{line.strip()}")
    if hits:
        fail("packages must not use !secret:\n  " + "\n  ".join(hits))
    print("OK packages hardcode no !secret names")


def test_no_pedro_ownership_branding() -> None:
    hits: list[str] = []
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(ROOT)
        parts = set(rel.parts)
        if parts & {".git", ".esphome", ".venv", "__pycache__", "node_modules"}:
            continue
        if rel.suffix.lower() in {".png", ".jpg", ".jpeg", ".gif", ".pdf", ".bin", ".pyc"}:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for i, line in enumerate(text.splitlines(), 1):
            if not PEDRO_RE.search(line):
                continue
            if rel in ALLOWED_PEDRO_PATHS:
                # base: only library dependency URL
                if rel == Path("packages/core/base.yml"):
                    if "tesla-ble.git" not in line and "BLE_library" not in line:
                        hits.append(f"{rel}:{i}:{line.strip()}")
                # README: only the single historical library credit
                elif rel == Path("README.md"):
                    if "tesla-ble" not in line.lower() and "library" not in line.lower():
                        hits.append(f"{rel}:{i}:{line.strip()}")
                continue
            hits.append(f"{rel}:{i}:{line.strip()}")
    if hits:
        fail("Pedro ownership branding remains:\n  " + "\n  ".join(hits))
    print("OK no Pedro ownership branding outside allowed credits")


def test_components_codeowners() -> None:
    for rel in (
        "components/tesla_ble_vehicle/__init__.py",
        "components/tesla_ble_listener/__init__.py",
        "components/tesla_fleet_api_server/__init__.py",
    ):
        text = read(rel)
        if 'CODEOWNERS = ["@Jabe"]' not in text:
            fail(f"{rel} CODEOWNERS must be @Jabe")
    print("OK components CODEOWNERS are Jabe")


def test_project_identity() -> None:
    base = read("packages/core/base.yml")
    if "github://Jabe/esphome-tesla" not in base:
        fail("base default component source must be Jabe")
    dash = read("packages/external_components.dashboard.yml")
    if "github://Jabe/esphome-tesla" not in dash:
        fail("dashboard external_components must be Jabe")
    if "PedroKTFC" in dash:
        fail("dashboard external_components still Pedro")
    print("OK project remote identity is Jabe")


def main() -> None:
    test_required_packages_exist()
    test_base_is_minimal()
    test_connectivity_and_fleet_split()
    test_example_is_package_map()
    test_packages_are_unopinionated_about_secrets()
    test_no_pedro_ownership_branding()
    test_components_codeowners()
    test_project_identity()
    print("\nAll structural tests passed.")


if __name__ == "__main__":
    main()
