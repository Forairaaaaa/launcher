#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PACKAGE_NAME="${PACKAGE_NAME:-m5cardputerzero-compass}"
PACKAGE_VERSION="${PACKAGE_VERSION:-0.1.0}"
PACKAGE_SUFFIX="${PACKAGE_SUFFIX:-m5stack1}"
DEB_ARCH="${DEB_ARCH:-arm64}"
MAINTAINER="${MAINTAINER:-m5stack <m5stack@m5stack.com>}"
PARALLEL="${PARALLEL:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/package}"
STAGE_DIR="${STAGE_DIR:-${ROOT_DIR}/build/deb-root}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist}"
BIN_NAME="M5CardputerZero-Compass"
PACKAGE_ICON_NAME="${PACKAGE_ICON_NAME:-m5cardputerzero-compass.png}"
CONFIG_DIR="${CONFIG_DIR:-\$HOME/.config/Compass}"
CMAKE_BIN="${CMAKE:-cmake}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

if ! command -v dpkg-deb >/dev/null 2>&1; then
    echo "dpkg-deb not found. Install dpkg-dev first." >&2
    exit 1
fi

if ! command -v "${CMAKE_BIN}" >/dev/null 2>&1; then
    echo "cmake not found. Install CMake or set CMAKE=/path/to/cmake." >&2
    exit 1
fi

CMAKE_CONFIGURE_ARGS=(
    -S "${ROOT_DIR}"
    -B "${BUILD_DIR}"
    -DCOMPASS_USE_SDL=OFF
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)

if [[ -z "${CMAKE_TOOLCHAIN_FILE:-}" && "${DEB_ARCH}" == "arm64" ]]; then
    host_arch="$(uname -m)"
    if [[ "${host_arch}" != "aarch64" && "${host_arch}" != "arm64" && -x "$(command -v aarch64-linux-gnu-gcc || true)" ]]; then
        CMAKE_CONFIGURE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/aarch64-linux-gnu.cmake")
    fi
elif [[ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]]; then
    CMAKE_CONFIGURE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}")
fi

"${CMAKE_BIN}" "${CMAKE_CONFIGURE_ARGS[@]}" ${EXTRA_CMAKE_ARGS:-}
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

EXECUTABLE="${ROOT_DIR}/dist/${BIN_NAME}"
ICON_FILE="${SCRIPT_DIR}/images/compass.png"
DESKTOP_TEMPLATE="${SCRIPT_DIR}/compass.desktop.in"

for path in "${EXECUTABLE}" "${ICON_FILE}" "${DESKTOP_TEMPLATE}"; do
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
sed \
    -e "s|@DESKTOP_EXEC@|${DESKTOP_EXEC}|g" \
    -e "s|@PACKAGE_ICON_NAME@|${PACKAGE_ICON_NAME}|g" \
    "${DESKTOP_TEMPLATE}" >"${STAGE_DIR}/usr/share/APPLaunch/applications/compass.desktop"
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
