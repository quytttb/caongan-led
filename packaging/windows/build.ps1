[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $QtRoot,
    [string] $BuildDir = ""
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (-not $BuildDir) { $BuildDir = Join-Path $Root 'build/windows-release' }
$QtRoot = (Resolve-Path $QtRoot).Path
$QtPaths = Join-Path $QtRoot 'bin/qtpaths6.exe'
if (-not (Test-Path $QtPaths)) { $QtPaths = Join-Path $QtRoot 'bin/qtpaths.exe' }
if (-not (Test-Path $QtPaths)) { throw "Khong tim thay $QtPaths" }
$Pinned = (Get-Content (Join-Path $Root 'packaging/qt_version.txt') -Raw).Trim()
$Actual = (& $QtPaths --qt-version | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $Actual -ne $Pinned) {
    throw "Can Qt $Pinned, nhung kit tra ve '$Actual'."
}

$Kit = Join-Path $Root 'shared/logger-ui-kit'
if (-not (Test-Path (Join-Path $Kit 'src/theme/CMakeLists.txt'))) {
    throw 'Thieu logger-ui-kit; xem docs/provenance.md.'
}
if ((Get-Command git -ErrorAction SilentlyContinue) -and (Test-Path (Join-Path $Kit '.git'))) {
    $Revision = (& git -C $Kit rev-parse HEAD | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $Revision -ne '24ef01544e398e0e181933a71ee00d40f28c697e') {
        throw "UI kit sai revision: $Revision"
    }
    $Dirty = (& git -C $Kit status --porcelain --untracked-files=all | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $Dirty) { throw 'UI kit khong sach; hay kiem tra, khong tu dong huy thay doi.' }
} else {
    Write-Warning 'Khong xac minh duoc revision UI kit bang git; can doi chieu provenance thu cong.'
}

# Chay trong x64 Native Tools Command Prompt / Developer PowerShell (MSVC va Ninja).
$env:PATH = "$(Join-Path $QtRoot 'bin');$env:PATH"
& cmake -S $Root -B $BuildDir -G Ninja '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_PREFIX_PATH=$QtRoot" '-DCAONGAN_BUILD_TESTS=ON'
if ($LASTEXITCODE -ne 0) { throw 'CMake configure that bai.' }
& cmake --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build that bai.' }
& ctest --test-dir $BuildDir --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'CTest that bai.' }
Write-Host "Build/test xong: $BuildDir"
