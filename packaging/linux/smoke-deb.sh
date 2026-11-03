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
    local data_dir="$1"
    shift
    env -i HOME="$scratch" PATH=/usr/bin:/bin LANG=C.UTF-8 \
        QT_QPA_PLATFORM=offscreen QT_FATAL_WARNINGS=1 XDG_RUNTIME_DIR="$scratch/xdg" \
        CAONGAN_LED_BASE_DIR="$data_dir" "$prefix/bin/caongan_led" "$@"
}
[[ "$(run_app "$scratch/app-data" --version)" == "CaonganLed $version" ]]
# Nguồn trống trong sandbox; chế độ thường phải khởi tạo SQLite.
run_app "$scratch/app-data" --preview --verify-preview-windows
[[ -f "$scratch/app-data/data/snapshot.db" ]]
# Chế độ mô phỏng cũng phải chạy được, nhưng không cần/không nên dựa vào DB thật.
run_app "$scratch/simulation-data" --preview --simulate --verify-preview-windows
echo 'DEB_SMOKE_OK'
