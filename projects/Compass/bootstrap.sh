#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_BIN="${PYTHON_BIN:-python3}"

if [[ ! -x "${ROOT_DIR}/.venv/bin/python" ]]; then
    "${PYTHON_BIN}" -m venv "${ROOT_DIR}/.venv"
fi

if ! "${ROOT_DIR}/.venv/bin/python" -m pip --version >/dev/null 2>&1; then
    echo "Project virtualenv is missing pip. Install python3-venv, remove .venv, then run ./bootstrap.sh again." >&2
    exit 1
fi

"${ROOT_DIR}/.venv/bin/python" -m pip install --upgrade pip
"${ROOT_DIR}/.venv/bin/python" -m pip install pypng lz4 Pillow

if [[ ! -d "${ROOT_DIR}/dependencies/lvgl/.git" || \
      ! -d "${ROOT_DIR}/dependencies/spdlog/.git" || \
      ! -d "${ROOT_DIR}/dependencies/smooth_ui_toolkit/.git" ]]; then
    "${ROOT_DIR}/.venv/bin/python" "${ROOT_DIR}/fetch_repos.py"
fi

echo "Compass bootstrap complete."
echo "Build with:"
echo "  cmake -S . -B build/sdl -DCOMPASS_USE_SDL=ON"
echo "  cmake --build build/sdl -j$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
