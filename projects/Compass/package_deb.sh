#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_NAME="${PACKAGE_NAME:-m5cardputerzero-compass}"
PACKAGE_VERSION="${PACKAGE_VERSION:-0.1.0}"
PACKAGE_SUFFIX="${PACKAGE_SUFFIX:-m5stack1}"
DEB_ARCH="${DEB_ARCH:-arm64}"
MAINTAINER="${MAINTAINER:-m5stack <m5stack@m5stack.com>}"
PARALLEL="${PARALLEL:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
STAGE_DIR="${STAGE_DIR:-${ROOT_DIR}/build/deb-root}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist}"
BIN_NAME="M5CardputerZero-Compass"
PACKAGE_ICON_NAME="${PACKAGE_ICON_NAME:-m5cardputerzero-compass.png}"
CONFIG_DIR="${CONFIG_DIR:-\$HOME/.config/Compass}"

if ! command -v dpkg-deb >/dev/null 2>&1; then
    echo "dpkg-deb not found. Install dpkg-dev first." >&2
    exit 1
fi

SCONS_BIN="${SCONS:-}"
if [[ -z "${SCONS_BIN}" ]]; then
    if [[ -x "${ROOT_DIR}/../../.venv/bin/scons" ]]; then
        SCONS_BIN="${ROOT_DIR}/../../.venv/bin/scons"
    elif command -v scons >/dev/null 2>&1; then
        SCONS_BIN="scons"
    else
        echo "scons not found. Activate the project virtualenv or set SCONS=/path/to/scons." >&2
        exit 1
    fi
fi

(
    cd "${ROOT_DIR}"
    CONFIG_DEFAULT_FILE="${CONFIG_DEFAULT_FILE:-linux_x86_cross_cp0_config_defaults.mk}" \
        "${SCONS_BIN}" -j"${PARALLEL}"
)

EXECUTABLE="${ROOT_DIR}/dist/${BIN_NAME}"
ICON_FILE="${ROOT_DIR}/share/images/compass.png"

for path in "${EXECUTABLE}" "${ICON_FILE}"; do
    if [[ ! -f "${path}" ]]; then
        echo "Required file not found: ${path}" >&2
        exit 1
    fi
done

rm -rf "${STAGE_DIR}"
mkdir -p \
    "${STAGE_DIR}/DEBIAN" \
    "${STAGE_DIR}/usr/share/APPLaunch/bin" \
    "${STAGE_DIR}/usr/share/APPLaunch/applications" \
    "${STAGE_DIR}/usr/share/APPLaunch/share/images" \
    "${DIST_DIR}"

install -m 755 "${EXECUTABLE}" "${STAGE_DIR}/usr/share/APPLaunch/bin/${BIN_NAME}"
DESKTOP_EXEC="mkdir -p \"${CONFIG_DIR}\" && COMPASS_CONFIG_DIR=\"${CONFIG_DIR}\" exec /usr/share/APPLaunch/bin/${BIN_NAME}"
cat >"${STAGE_DIR}/usr/share/APPLaunch/applications/compass.desktop" <<EOF
[Desktop Entry]
Name=Compass
Exec=${DESKTOP_EXEC}
Terminal=false
Icon=share/images/${PACKAGE_ICON_NAME}
Type=Application
EOF
install -m 644 "${ICON_FILE}" "${STAGE_DIR}/usr/share/APPLaunch/share/images/${PACKAGE_ICON_NAME}"

INSTALLED_SIZE="$(du -sk "${STAGE_DIR}/usr" | awk '{print $1}')"
cat >"${STAGE_DIR}/DEBIAN/control" <<EOF
Package: ${PACKAGE_NAME}
Version: ${PACKAGE_VERSION}
Section: utils
Priority: optional
Architecture: ${DEB_ARCH}
Maintainer: ${MAINTAINER}
Depends: libc6, libstdc++6, libgcc-s1
Installed-Size: ${INSTALLED_SIZE}
Description: Compass application for M5CardputerZero APPLaunch
 Compass application, launcher entry, and runtime icon.
EOF

DEB_PATH="${DIST_DIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}_${PACKAGE_SUFFIX}_${DEB_ARCH}.deb"
dpkg-deb --build --root-owner-group "${STAGE_DIR}" "${DEB_PATH}"

echo "Generated Debian package: ${DEB_PATH}"
