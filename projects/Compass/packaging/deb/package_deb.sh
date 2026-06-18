#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PACKAGE_NAME="${PACKAGE_NAME:-m5cardputerzero-compass}"
PACKAGE_SUFFIX="${PACKAGE_SUFFIX:-m5stack1}"
DEB_ARCH="arm64"
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

read_cmake_cache_value() {
    local name="$1"
    local cache_file="${BUILD_DIR}/CMakeCache.txt"
    local line=""

    if [[ ! -f "${cache_file}" ]]; then
        echo "CMake cache not found: ${cache_file}" >&2
        return 1
    fi

    line="$(grep -E "^${name}(:[^=]*)?=" "${cache_file}" | tail -n 1 || true)"
    if [[ -z "${line}" ]]; then
        echo "CMake cache value not found: ${name}" >&2
        return 1
    fi

    printf "%s\n" "${line#*=}"
}

CMAKE_CONFIGURE_ARGS=(
    -S "${ROOT_DIR}"
    -B "${BUILD_DIR}"
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    -DCOMPASS_USE_SDL=OFF
)

host_arch="$(uname -m)"
if [[ "${host_arch}" != "aarch64" && "${host_arch}" != "arm64" ]]; then
    if [[ ! -x "$(command -v aarch64-linux-gnu-gcc || true)" || ! -x "$(command -v aarch64-linux-gnu-g++ || true)" ]]; then
        echo "aarch64-linux-gnu-gcc/g++ not found. Install the GNU aarch64 toolchain for cp0 packaging." >&2
        exit 1
    fi
    CMAKE_CONFIGURE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/aarch64-linux-gnu.cmake")
fi

"${CMAKE_BIN}" "${CMAKE_CONFIGURE_ARGS[@]}"
if [[ "$(read_cmake_cache_value COMPASS_USE_SDL)" != "OFF" ]]; then
    echo "Invalid package build: COMPASS_USE_SDL must be OFF for cp0 packaging." >&2
    exit 1
fi
PACKAGE_VERSION="$(read_cmake_cache_value CMAKE_PROJECT_VERSION)"
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
