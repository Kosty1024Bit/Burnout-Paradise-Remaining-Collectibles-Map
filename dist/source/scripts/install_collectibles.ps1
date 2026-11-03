param([string]$GameDirectory='D:/Burnout Paradise Remastered')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if(Get-Process BurnoutPR -ErrorAction SilentlyContinue){throw 'Close Burnout Paradise Remastered first.'}
$exe=Join-Path $game 'BurnoutPR.exe'
if((Get-FileHash -LiteralPath $exe).Hash -ne 'E08528C1F696FAB1D1552576CF3E63537253E1F7A264809C919BA2DE5A982CC9'){throw 'Unsupported executable.'}
$receiptPath=Join-Path $root 'artifacts/install-receipt.json'
if(-not(Test-Path -LiteralPath $receiptPath)){throw 'Existing speedometer installation receipt required.'}
$receipt=Get-Content -Raw -LiteralPath $receiptPath | ConvertFrom-Json
$target=Join-Path $game 'dinput8.dll'
if((Get-FileHash -LiteralPath $target).Hash -ne $receipt.dllSha256){throw 'Installed DLL differs from this workspace receipt.'}
$dll=Join-Path $root 'build/Release/dinput8.dll'
if(-not(Test-Path -LiteralPath $dll)){throw 'Build the mod first.'}
& python (Join-Path $PSScriptRoot 'prepare_collectibles.py') --game $game --output (Join-Path $root 'artifacts/private/Collectibles.bin')
if($LASTEXITCODE){throw 'Marker cache preparation failed.'}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$backup=Join-Path $root "backups/collectibles-$stamp"
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$names=@('dinput8.dll','CenterSpeedometer.ini','Collectibles.ini','Collectibles.bin')
foreach($name in $names){$p=Join-Path $game $name;if(Test-Path -LiteralPath $p){Copy-Item -LiteralPath $p -Destination (Join-Path $backup $name)}}
Copy-Item -LiteralPath $receiptPath -Destination (Join-Path $backup 'install-receipt.json')
$env:UM_HOME=Join-Path $root 'backups/um'
& um backup create (Join-Path $env:LOCALAPPDATA 'Criterion Games/Burnout Paradise Remastered') --name bpr-before-collectibles
if($LASTEXITCODE){throw 'Profile backup failed.'}
Copy-Item -LiteralPath $dll -Destination $target
Copy-Item -LiteralPath (Join-Path $root 'Collectibles.ini') -Destination (Join-Path $game 'Collectibles.ini')
Copy-Item -LiteralPath (Join-Path $root 'artifacts/private/Collectibles.bin') -Destination (Join-Path $game 'Collectibles.bin')
$receipt.dllSha256=(Get-FileHash -LiteralPath $dll).Hash
$receipt.installedFiles=@('dinput8.dll','CenterSpeedometer.ini','Collectibles.ini','Collectibles.bin')
$receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath -Encoding utf8
@{backup=$backup;game=$game;dllSha256=$receipt.dllSha256;date=(Get-Date -Format o)} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'artifacts/collectibles-install-receipt.json') -Encoding utf8
Write-Output "Installed remaining collectibles map + existing speedometer. Rollback: $backup"
