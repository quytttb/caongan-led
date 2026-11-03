[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $PackageDir,
    [Parameter(Mandatory = $true)] [string] $OutputDir,
    [Parameter(Mandatory = $true)] [string] $Version,
    [Parameter(Mandatory = $true)] [string] $BuildDir,
    [string] $WixBin = ''
)

$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$') {
    throw 'Version phai co dang MAJOR.MINOR.PATCH.'
}
$Parts = $Version.Split('.') | ForEach-Object { [long]$_ }
if ($Parts[0] -gt 255 -or $Parts[1] -gt 255 -or $Parts[2] -gt 65535) {
    throw 'MSI yeu cau major/minor <= 255 va patch <= 65535.'
}
$PackageDir = (Resolve-Path $PackageDir).Path
if (-not (Test-Path (Join-Path $PackageDir 'caongan_led.exe'))) { throw 'Thieu caongan_led.exe.' }
if (-not $WixBin) {
    $Command = Get-Command candle.exe -ErrorAction SilentlyContinue
    if ($Command) { $WixBin = Split-Path $Command.Source }
    elseif ($env:WIX) { $WixBin = Join-Path $env:WIX 'bin' }
    else {
        foreach ($Candidate in @(
            'C:\Program Files (x86)\WiX Toolset v3.14\bin',
            'C:\Program Files\WiX Toolset v3.14\bin')) {
            if (Test-Path (Join-Path $Candidate 'candle.exe')) { $WixBin = $Candidate; break }
        }
    }
}
foreach ($Tool in @('heat.exe', 'candle.exe', 'light.exe')) {
    if (-not $WixBin -or -not (Test-Path (Join-Path $WixBin $Tool))) { throw "Thieu WiX 3: $Tool" }
}
if (Test-Path $BuildDir) { throw "Thu muc build MSI da ton tai: $BuildDir" }
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$BuildDir = (Resolve-Path $BuildDir).Path
$OutputDir = (Resolve-Path $OutputDir).Path
$Msi = Join-Path $OutputDir "caongan-led-$Version-win-x64.msi"
if ((Test-Path $Msi) -or (Test-Path "$Msi.sha256")) { throw "MSI da ton tai: $Msi" }
$Harvest = Join-Path $BuildDir 'payload.wxs'
$Definition = Join-Path $PSScriptRoot 'CaoNganLed.wxs'

& (Join-Path $WixBin 'heat.exe') dir $PackageDir -nologo -cg AppFiles -ag -srd -sreg -scom `
    -var var.PayloadDir -dr INSTALLFOLDER -out $Harvest
if ($LASTEXITCODE -ne 0) { throw 'WiX heat that bai.' }
& (Join-Path $WixBin 'candle.exe') -nologo -arch x64 "-dVersion=$Version" `
    '-out' (Join-Path $BuildDir 'product.wixobj') $Definition
if ($LASTEXITCODE -ne 0) { throw 'WiX candle product that bai.' }
& (Join-Path $WixBin 'candle.exe') -nologo -arch x64 "-dPayloadDir=$PackageDir" `
    '-out' (Join-Path $BuildDir 'payload.wixobj') $Harvest
if ($LASTEXITCODE -ne 0) { throw 'WiX candle payload that bai.' }
& (Join-Path $WixBin 'light.exe') -nologo -out $Msi `
    (Join-Path $BuildDir 'product.wixobj') (Join-Path $BuildDir 'payload.wixobj')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $Msi)) { throw 'WiX light that bai.' }
$Hash = (Get-FileHash -Algorithm SHA256 $Msi).Hash.ToLowerInvariant()
Set-Content -Path "$Msi.sha256" -Value "$Hash  $(Split-Path $Msi -Leaf)" -Encoding ASCII
Write-Host "MSI: $Msi`nSHA-256: $Hash"
