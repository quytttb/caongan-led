[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $MsiPath,
    [Parameter(Mandatory = $true)] [string] $ScratchDir,
    [Parameter(Mandatory = $true)] [string] $PackageSmokeScript
)

$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Chi chay smoke MSI tren runner co lap.' }
$MsiPath = (Resolve-Path $MsiPath).Path
$PackageSmokeScript = (Resolve-Path $PackageSmokeScript).Path
if (Test-Path $ScratchDir) { throw "Scratch da ton tai: $ScratchDir" }
New-Item -ItemType Directory -Path $ScratchDir -Force | Out-Null
$ScratchDir = (Resolve-Path $ScratchDir).Path
$InstallDir = Join-Path $ScratchDir 'installed'
$Installed = $false
try {
    $Install = Start-Process -FilePath msiexec.exe -Wait -PassThru -ArgumentList @(
        '/i', "`"$MsiPath`"", '/qn', '/norestart', "INSTALLFOLDER=`"$InstallDir`"",
        '/l*v', "`"$(Join-Path $ScratchDir 'install.log')`"")
    if ($Install.ExitCode -ne 0) { throw "MSI install that bai: $($Install.ExitCode)" }
    $Installed = $true
    & $PackageSmokeScript -PackageDir $InstallDir -ScratchDir (Join-Path $ScratchDir 'app-smoke')
} finally {
    if ($Installed) {
        $Uninstall = Start-Process -FilePath msiexec.exe -Wait -PassThru -ArgumentList @(
            '/x', "`"$MsiPath`"", '/qn', '/norestart', '/l*v', "`"$(Join-Path $ScratchDir 'uninstall.log')`"")
        if ($Uninstall.ExitCode -ne 0) { throw "MSI uninstall that bai: $($Uninstall.ExitCode)" }
        if (Test-Path (Join-Path $InstallDir 'caongan_led.exe')) { throw 'Executable van con sau uninstall.' }
        if (-not (Test-Path (Join-Path $ScratchDir 'app-smoke/app-data/data/snapshot.db'))) {
            throw 'Uninstall khong duoc xoa du lieu ung dung.'
        }
    }
}
Write-Host 'MSI_INSTALL_UNINSTALL_SMOKE_OK'
