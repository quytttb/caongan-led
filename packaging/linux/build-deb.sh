#!/usr/bin/env bash
set -euo pipefail

tool_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_dir="${CAONGAN_SOURCE_DIR:-$(cd "$tool_dir/../.." && pwd)}"
version="${CAONGAN_PACKAGE_VERSION:?Set CAONGAN_PACKAGE_VERSION to MAJOR.MINOR.PATCH}"
build_dir="${CAONGAN_PACKAGE_BUILD_DIR:?Set CAONGAN_PACKAGE_BUILD_DIR to a build directory}"
output_dir="${CAONGAN_PACKAGE_OUTPUT_DIR:?Set CAONGAN_PACKAGE_OUTPUT_DIR to an output directory}"

if [[ ! "$version" =~ ^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]]; then
    echo 'Invalid package version.' >&2
    exit 1
fi
command -v patchelf >/dev/null || { echo 'patchelf is required.' >&2; exit 1; }
command -v dpkg-deb >/dev/null || { echo 'dpkg-deb is required.' >&2; exit 1; }
mkdir -p "$build_dir" "$output_dir"
name="caongan-led_${version}_amd64.deb"
if [[ -e "$output_dir/$name" || -e "$output_dir/$name.sha256" ]]; then
    echo "Package already exists: $output_dir/$name" >&2
    exit 1
fi

args=(-S "$tool_dir" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release
      "-DCAONGAN_APP_SOURCE_DIR=$source_dir" "-DCAONGAN_PACKAGE_VERSION=$version"
      -DCMAKE_INSTALL_PREFIX=/opt/caongan-led -DCMAKE_INSTALL_LIBDIR=lib)
if [[ -n "${QT_ROOT_DIR:-}" ]]; then
    args+=("-DCMAKE_PREFIX_PATH=$QT_ROOT_DIR")
fi
cmake "${args[@]}"
cmake --build "$build_dir" --target caongan_led --parallel "${CAONGAN_BUILD_JOBS:-4}"
(cd "$build_dir" && cpack -G DEB)
cp "$build_dir/$name" "$output_dir/$name"
(cd "$output_dir" && sha256sum "$name" > "$name.sha256")
echo "DEB: $output_dir/$name"
