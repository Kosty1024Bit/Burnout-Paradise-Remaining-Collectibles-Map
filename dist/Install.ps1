param([string]$GameDirectory='D:/Burnout Paradise Remastered')
$ErrorActionPreference='Stop'
if(Get-Process BurnoutPR -ErrorAction SilentlyContinue){throw 'Close Burnout Paradise Remastered first.'}
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if((Get-FileHash -LiteralPath (Join-Path $game 'BurnoutPR.exe')).Hash -ne 'E08528C1F696FAB1D1552576CF3E63537253E1F7A264809C919BA2DE5A982CC9'){throw 'Unsupported game executable.'}
$dll=Join-Path $PSScriptRoot 'dinput8.dll'
$target=Join-Path $game 'dinput8.dll'
$ownHash=(Get-FileHash -LiteralPath $dll).Hash
$legacyHash='0162755E2F03F2EEB9ECC7C2931389C528C378C97D252C8C071978BA401481CE'
if((Test-Path -LiteralPath $target) -and ((Get-FileHash -LiteralPath $target).Hash -notin @($ownHash,$legacyHash))){throw 'Another dinput8 mod is installed. Refusing to overwrite it.'}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$backup=Join-Path $PSScriptRoot "backups/$stamp"
New-Item -ItemType Directory -Path $backup -Force | Out-Null
foreach($name in @('dinput8.dll','Collectibles.ini','Collectibles.bin')){
    $p=Join-Path $game $name
    if(Test-Path -LiteralPath $p){Copy-Item -LiteralPath $p -Destination (Join-Path $backup $name)}
}
$profile=Join-Path $env:LOCALAPPDATA 'Criterion Games/Burnout Paradise Remastered'
if(Test-Path -LiteralPath $profile){Compress-Archive -LiteralPath $profile -DestinationPath (Join-Path $backup 'profile.zip')}
$cache=Join-Path $backup 'prepared-Collectibles.bin'
& python (Join-Path $PSScriptRoot 'prepare_collectibles.py') --game $game --output $cache
if($LASTEXITCODE){throw 'Coordinate cache preparation failed. Game files unchanged.'}
Copy-Item -LiteralPath $dll -Destination $target
Copy-Item -LiteralPath $cache -Destination (Join-Path $game 'Collectibles.bin')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Collectibles.ini') -Destination (Join-Path $game 'Collectibles.ini')
if(-not(Test-Path -LiteralPath (Join-Path $game 'CenterSpeedometer.ini'))){Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CenterSpeedometer.ini') -Destination (Join-Path $game 'CenterSpeedometer.ini')}
Write-Output "Installed. Backup: $backup"
