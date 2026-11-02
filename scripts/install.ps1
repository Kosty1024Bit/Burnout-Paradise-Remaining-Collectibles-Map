param([string]$GameDirectory = 'D:/Burnout Paradise Remastered')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$game = (Resolve-Path -LiteralPath $GameDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $game 'BurnoutPR.exe'))) { throw 'BurnoutPR.exe not found' }
if (Get-Process BurnoutPR -ErrorAction SilentlyContinue) { throw 'Close the game before installing.' }
$dll = Join-Path $root 'build/Release/dinput8.dll'
if (-not (Test-Path -LiteralPath $dll)) { throw 'Build the mod first.' }
foreach ($name in @('dinput8.dll','CenterSpeedometer.ini')) {
    if (Test-Path -LiteralPath (Join-Path $game $name)) { throw "Existing $name detected; refusing to overwrite it." }
}
$expected = 'E08528C1F696FAB1D1552576CF3E63537253E1F7A264809C919BA2DE5A982CC9'
if ((Get-FileHash -LiteralPath (Join-Path $game 'BurnoutPR.exe')).Hash -ne $expected) { throw 'Game executable changed since compatibility inspection.' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backup = Join-Path $root "backups/install-$stamp"
New-Item -ItemType Directory -Path $backup -Force | Out-Null
Get-ChildItem -LiteralPath $game -File | Select-Object Name,Length,LastWriteTimeUtc | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'game-root-before.json') -Encoding utf8
$env:UM_HOME = Join-Path $root 'backups/um'
$profile = Join-Path $env:LOCALAPPDATA 'Criterion Games/Burnout Paradise Remastered'
if (Test-Path -LiteralPath $profile) {
    & um backup create $profile --name bpr-before-center-speedometer
    if ($LASTEXITCODE) { throw 'Profile backup failed.' }
}
Copy-Item -LiteralPath $dll -Destination (Join-Path $game 'dinput8.dll')
Copy-Item -LiteralPath (Join-Path $root 'CenterSpeedometer.ini') -Destination (Join-Path $game 'CenterSpeedometer.ini')
@{
    gameDirectory=$game
    backup=$backup
    installedFiles=@('dinput8.dll','CenterSpeedometer.ini')
    dllSha256=(Get-FileHash -LiteralPath $dll).Hash
    date=(Get-Date -Format o)
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'artifacts/install-receipt.json') -Encoding utf8
Write-Output "Installed Center Speedometer into $game"
Write-Output "Backup: $backup"
