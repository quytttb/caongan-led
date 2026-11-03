[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $QtRoot,
    [string] $BuildDir = "",
    [string] $OutputRoot = "",
    [string] $Version = '1.0.0'
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if ($Version -notmatch '^\d+\.\d+\.\d+([-.][A-Za-z0-9.-]+)?$') { throw 'Version khong hop le.' }
if (-not $BuildDir) { $BuildDir = Join-Path $Root 'build/windows-release' }
if (-not $OutputRoot) { $OutputRoot = Join-Path $Root 'dist' }
$QtRoot = (Resolve-Path $QtRoot).Path
$BuildDir = (Resolve-Path $BuildDir).Path
$QtPaths = Join-Path $QtRoot 'bin/qtpaths6.exe'
if (-not (Test-Path $QtPaths)) { $QtPaths = Join-Path $QtRoot 'bin/qtpaths.exe' }
$Deploy = Join-Path $QtRoot 'bin/windeployqt.exe'
foreach ($path in @($QtPaths, $Deploy, (Join-Path $BuildDir 'caongan_led.exe'))) {
    if (-not (Test-Path $path)) { throw "Khong tim thay $path" }
}
$Pinned = (Get-Content (Join-Path $Root 'packaging/qt_version.txt') -Raw).Trim()
$Actual = (& $QtPaths --qt-version | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $Actual -ne $Pinned) { throw "Can Qt $Pinned, khong phai '$Actual'." }
$VersionLine = Select-String -Path (Join-Path $BuildDir 'CMakeCache.txt') -Pattern '^CMAKE_PROJECT_VERSION:STATIC=(.+)$' | Select-Object -First 1
if (-not $VersionLine -or $VersionLine.Matches[0].Groups[1].Value -ne $Version) {
    throw "Version '$Version' khong khop CMAKE_PROJECT_VERSION cua build."
}
$QtDirLine = Select-String -Path (Join-Path $BuildDir 'CMakeCache.txt') -Pattern '^Qt6_DIR:PATH=(.+)$' | Select-Object -First 1
if (-not $QtDirLine) { throw 'CMakeCache khong co Qt6_DIR.' }
$CachedQtDir = (Resolve-Path ($QtDirLine.Matches[0].Groups[1].Value)).Path
$SelectedQtDir = (Resolve-Path (Join-Path $QtRoot 'lib/cmake/Qt6')).Path
if ($CachedQtDir -ine $SelectedQtDir) { throw "QtRoot khong khop kit da build: $CachedQtDir" }
$PackageName = "caongan-led-$Version-win-x64"
$Destination = Join-Path $OutputRoot $PackageName
$Zip = Join-Path $OutputRoot "$PackageName.zip"
$Checksum = "$Zip.sha256"
if ((Test-Path $Destination) -or (Test-Path $Zip) -or (Test-Path $Checksum)) {
    throw "Dich da ton tai; khong ghi de: $Destination / $Zip / $Checksum"
}
New-Item -ItemType Directory -Path $Destination | Out-Null
Copy-Item (Join-Path $BuildDir 'caongan_led.exe') $Destination
$env:PATH = "$(Join-Path $QtRoot 'bin');$env:PATH"
& $Deploy --release --no-compiler-runtime --qmldir (Join-Path $Root 'src/app/qml') --qmlimport $BuildDir (Join-Path $Destination 'caongan_led.exe')
if ($LASTEXITCODE -ne 0) { throw "windeployqt that bai; goi chua hoan thanh: $Destination" }
foreach ($relative in @('platforms/qwindows.dll', 'sqldrivers/qsqlite.dll')) {
    if (-not (Test-Path (Join-Path $Destination $relative))) { throw "Thieu plugin $relative" }
}
Copy-Item (Join-Path $Root 'README.md') $Destination
Copy-Item (Join-Path $Root 'THIRD_PARTY_NOTICES.md') $Destination
Copy-Item (Join-Path $Root 'docs') $Destination -Recurse
Copy-Item (Join-Path $Root 'sample-data') $Destination -Recurse
Copy-Item (Join-Path $Root 'packaging') $Destination -Recurse
New-Item -ItemType Directory -Path (Join-Path $Destination 'tools') | Out-Null
Copy-Item (Join-Path $Root 'tools/tt10_simulator.py') (Join-Path $Destination 'tools/tt10_simulator.py')
New-Item -ItemType Directory -Path (Join-Path $Destination 'licenses') | Out-Null
Copy-Item (Join-Path $Root 'shared/logger-ui-kit/LICENSE') (Join-Path $Destination 'licenses/logger-ui-kit-MIT.txt')
Copy-Item (Join-Path $Root 'resources/fonts/Inter/OFL.txt') (Join-Path $Destination 'licenses/Inter-OFL.txt')
Copy-Item (Join-Path $PSScriptRoot 'README.md') (Join-Path $Destination 'windows-deployment.md')
Copy-Item (Join-Path $PSScriptRoot 'TaskScheduler.example.xml') $Destination
Compress-Archive -Path $Destination -DestinationPath $Zip
$Hash = (Get-FileHash -Algorithm SHA256 $Zip).Hash.ToLowerInvariant()
Set-Content -Path $Checksum -Value "$Hash  $(Split-Path $Zip -Leaf)" -Encoding ASCII
Write-Warning 'Goi chua duoc xac nhan tren Windows sach; kiem tra Qt license va Visual C++ Redistributable chinh thuc.'
Write-Host "ZIP: $Zip`nSHA-256: $Hash"
