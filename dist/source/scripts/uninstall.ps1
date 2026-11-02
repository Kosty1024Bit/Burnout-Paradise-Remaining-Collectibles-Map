param([string]$GameDirectory = 'D:/Burnout Paradise Remastered')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$game = (Resolve-Path -LiteralPath $GameDirectory).Path
if (Get-Process BurnoutPR -ErrorAction SilentlyContinue) { throw 'Close the game before uninstalling.' }
$receipt = Get-Content -Raw -LiteralPath (Join-Path $root 'artifacts/install-receipt.json') | ConvertFrom-Json
if ($game -ne $receipt.gameDirectory) { throw 'Install receipt belongs to another game folder.' }
$dll = Join-Path $game 'dinput8.dll'
if ((Test-Path -LiteralPath $dll) -and (Get-FileHash -LiteralPath $dll).Hash -ne $receipt.dllSha256) { throw 'DLL changed; refusing to remove another mod.' }
foreach ($name in @('dinput8.dll','CenterSpeedometer.ini','CenterSpeedometer.log')) {
    $target = Join-Path $game $name
    if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target }
}
Write-Output 'Center Speedometer removed. Original game files and saves were not removed.'
