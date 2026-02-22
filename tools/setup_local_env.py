#!/usr/bin/env python3
from __future__ import annotations

import argparse
import glob
import json
import os
from pathlib import Path
from typing import Any


def pick_latest(paths: list[str]) -> str | None:
    if not paths:
        return None
    return sorted(paths)[-1]


def pick_first_existing(patterns: list[str]) -> str | None:
    for pattern in patterns:
        matches = sorted(glob.glob(pattern))
        if matches:
            return matches[0]
    return None


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def save_json(path: Path, payload: dict[str, Any]) -> None:
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def load_env(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    values: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value
    return values


def detect_local_values() -> dict[str, str | None]:
    idf_path = os.environ.get("IDF_PATH")
    if not idf_path:
        idf_path = pick_latest(
            glob.glob(str(Path.home() / ".espressif" / "v*" / "esp-idf"))
        )

    clangd_path = pick_latest(
        glob.glob(
            str(
                Path.home()
                / ".espressif"
                / "tools"
                / "esp-clang"
                / "*"
                / "esp-clang"
                / "bin"
                / "clangd"
            )
        )
    )

    gcc_path = pick_latest(
        glob.glob(
            str(
                Path.home()
                / ".espressif"
                / "tools"
                / "xtensa-esp-elf"
                / "*"
                / "xtensa-esp-elf"
                / "bin"
                / "xtensa-esp32-elf-gcc"
            )
        )
    )

    serial_port = pick_first_existing(
        [
            "/dev/ttyACM*",
            "/dev/ttyUSB*",
            "/dev/cu.usbmodem*",
            "/dev/cu.SLAB_USBtoUART*",
            "/dev/cu.wchusbserial*",
            "/dev/cu.usbserial*",
            "/dev/serial/by-id/*",
        ]
    )

    return {
        "IDF_PATH": idf_path,
        "CLANGD_PATH": clangd_path,
        "XTENSA_GCC_PATH": gcc_path,
        "ESP_PORT": serial_port,
    }


def update_settings(workspace: Path, values: dict[str, str | None]) -> None:
    settings_path = workspace / ".vscode" / "settings.json"
    cpp_props_path = workspace / ".vscode" / "c_cpp_properties.json"

    settings = load_json(settings_path)
    cpp_props = load_json(cpp_props_path)

    if values["IDF_PATH"]:
        settings["idf.currentSetup"] = values["IDF_PATH"]
    if values["ESP_PORT"]:
        settings["idf.port"] = values["ESP_PORT"]
    if values["CLANGD_PATH"]:
        settings["clangd.path"] = values["CLANGD_PATH"]

    configs = cpp_props.get("configurations", [])
    if configs and values["XTENSA_GCC_PATH"]:
        configs[0]["compilerPath"] = values["XTENSA_GCC_PATH"]

    save_json(settings_path, settings)
    save_json(cpp_props_path, cpp_props)


def write_env_file(workspace: Path, values: dict[str, str | None]) -> Path:
    env_path = workspace / ".env.local"
    existing = load_env(env_path)
    merged = dict(existing)

    tool_keys = ["IDF_PATH", "CLANGD_PATH", "XTENSA_GCC_PATH", "ESP_PORT"]
    for key in tool_keys:
        detected = values.get(key)
        if detected:
            merged[key] = detected
        elif key not in merged:
            merged[key] = ""

    merged.setdefault("WIFI_SSID", "")
    merged.setdefault("WIFI_PASSWORD", "")

    ordered_keys = tool_keys + ["WIFI_SSID", "WIFI_PASSWORD"]
    extra_keys = sorted(k for k in merged if k not in ordered_keys)

    lines = []
    for key in ordered_keys + extra_keys:
        lines.append(f"{key}={merged.get(key, '')}")

    env_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return env_path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Set local ESP-IDF tool paths for this workspace"
    )
    parser.add_argument("--workspace", default=".", help="Workspace root path")
    args = parser.parse_args()

    workspace = Path(args.workspace).resolve()
    values = detect_local_values()
    update_settings(workspace, values)
    env_path = write_env_file(workspace, values)

    print("Updated VS Code settings with local machine paths")
    print(f"Wrote {env_path}")
    for key, value in values.items():
        print(f"{key}={value or ''}")


if __name__ == "__main__":
    main()
