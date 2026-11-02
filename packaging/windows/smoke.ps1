[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $PackageDir,
    [Parameter(Mandatory = $true)] [string] $ScratchDir,
    [ValidateRange(5, 300)] [int] $TimeoutSeconds = 30
)

$ErrorActionPreference = 'Stop'
$PackageDir = (Resolve-Path $PackageDir).Path
$Exe = Join-Path $PackageDir 'caongan_led.exe'
if (-not (Test-Path $Exe -PathType Leaf)) { throw "Khong tim thay $Exe" }
if (Test-Path $ScratchDir) { throw "Thu muc smoke da ton tai; khong ghi de: $ScratchDir" }
New-Item -ItemType Directory -Path $ScratchDir -Force | Out-Null
$ScratchDir = (Resolve-Path $ScratchDir).Path
$Stderr = Join-Path $ScratchDir 'stderr.log'
$Stdout = Join-Path $ScratchDir 'stdout.log'

$Variables = @('PATH', 'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QML_IMPORT_PATH',
    'QML2_IMPORT_PATH', 'QT_QPA_PLATFORM', 'QT_FATAL_WARNINGS', 'QT_LOGGING_RULES',
    'CAONGAN_LED_BASE_DIR', 'CAONGAN_LED_DATA_DIR', 'CAONGAN_LED_CONFIG_DIR', 'CAONGAN_LED_LOG_DIR')
$Previous = @{}
foreach ($name in $Variables) { $Previous[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
$Process = $null
try {
    # Khong cho phep dung Qt/QML tu kit build; app van can MSVC runtime hop le tren may.
    $env:PATH = "$(Join-Path $env:SystemRoot 'System32');$env:SystemRoot"
    foreach ($name in @('QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QML_IMPORT_PATH',
            'QML2_IMPORT_PATH', 'QT_LOGGING_RULES', 'CAONGAN_LED_DATA_DIR',
            'CAONGAN_LED_CONFIG_DIR', 'CAONGAN_LED_LOG_DIR')) {
        [Environment]::SetEnvironmentVariable($name, $null, 'Process')
    }
    $env:QT_QPA_PLATFORM = 'windows'
    $env:QT_FATAL_WARNINGS = '1'
    $env:CAONGAN_LED_BASE_DIR = Join-Path $ScratchDir 'app-data'
    $Process = Start-Process -FilePath $Exe -ArgumentList @('--preview', '--simulate') `
        -WorkingDirectory $ScratchDir -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr -PassThru

    $Ready = $false
    for ($i = 0; $i -lt ($TimeoutSeconds * 2); $i++) {
        Start-Sleep -Milliseconds 500
        $Process.Refresh()
        if ($Process.HasExited) { break }
        if (Test-Path $Stderr) {
            try {
                if ((Get-Content $Stderr -Raw -Encoding utf8) -match 'config =') { $Ready = $true; break }
            } catch { }
        }
    }
    if (-not $Ready) { throw "App thoat som hoac chua san sang; xem $Stderr" }
    Start-Sleep -Seconds 2
    $Process.Refresh()
    if ($Process.HasExited) { throw "App thoat ngay sau khi san sang; xem $Stderr" }
    if (-not (Test-Path (Join-Path $env:CAONGAN_LED_BASE_DIR 'data/snapshot.db') -PathType Leaf)) {
        throw "Khong tao duoc SQLite snapshot; xem $Stderr"
    }
    Write-Host 'Smoke package thanh cong: QML window va SQLite da khoi dong.'
} finally {
    if ($Process) {
        $Process.Refresh()
        if (-not $Process.HasExited) {
            Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
            $Process.WaitForExit(5000) | Out-Null
        }
    }
    foreach ($name in $Variables) { [Environment]::SetEnvironmentVariable($name, $Previous[$name], 'Process') }
}
