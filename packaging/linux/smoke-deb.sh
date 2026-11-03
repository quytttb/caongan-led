#!/usr/bin/env bash
set -euo pipefail

package="${1:?DEB path required}"
scratch="${2:?New scratch directory required}"
version="${3:?Expected version required}"
if [[ -e "$scratch" ]]; then
    echo "Scratch directory already exists: $scratch" >&2
    exit 1
fi
mkdir -p "$scratch/root" "$scratch/xdg"
chmod 700 "$scratch/xdg"
[[ "$(dpkg-deb -f "$package" Version)" == "$version" ]]
[[ "$(dpkg-deb -f "$package" Architecture)" == 'amd64' ]]
dpkg-deb -x "$package" "$scratch/root"
prefix="$scratch/root/opt/caongan-led"
for path in bin/caongan_led bin/qt.conf plugins/platforms/libqxcb.so \
            plugins/platforms/libqwayland.so plugins/platforms/libqoffscreen.so \
            plugins/sqldrivers/libqsqlite.so; do
    [[ -f "$prefix/$path" ]] || { echo "Missing $path" >&2; exit 1; }
done
for binary in "$prefix/bin/caongan_led" "$prefix/plugins/platforms/libqxcb.so" \
              "$prefix/plugins/platforms/libqwayland.so" "$prefix/plugins/sqldrivers/libqsqlite.so"; do
    dependencies="$(env -i PATH=/usr/bin:/bin ldd "$binary")"
    if grep -q 'not found' <<< "$dependencies"; then
        echo "Missing shared library for $binary: $dependencies" >&2
        exit 1
    fi
    if [[ -n "${QT_ROOT_DIR:-}" ]] && grep -Fq "$QT_ROOT_DIR" <<< "$dependencies"; then
        echo "Package is using Qt from the build machine: $binary" >&2
        exit 1
    fi
done
run_app() {
    env -i HOME="$scratch" PATH=/usr/bin:/bin LANG=C.UTF-8 \
        QT_QPA_PLATFORM=offscreen QT_FATAL_WARNINGS=1 XDG_RUNTIME_DIR="$scratch/xdg" \
        CAONGAN_LED_BASE_DIR="$scratch/app-data" "$prefix/bin/caongan_led" "$@"
}
[[ "$(run_app --version)" == "CaonganLed $version" ]]
run_app --preview --simulate --verify-preview-windows
[[ -f "$scratch/app-data/data/snapshot.db" ]]
echo 'DEB_SMOKE_OK'
