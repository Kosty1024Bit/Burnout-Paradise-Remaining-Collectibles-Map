$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$dist=Join-Path $root 'dist'
Copy-Item -LiteralPath (Join-Path $root 'build/Release/dinput8.dll') -Destination (Join-Path $dist 'dinput8.dll')
foreach($name in @('README.md','LICENSE','CenterSpeedometer.ini','Collectibles.ini')){
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination (Join-Path $dist $name)
}
Copy-Item -LiteralPath (Join-Path $root 'scripts/prepare_collectibles.py') -Destination (Join-Path $dist 'prepare_collectibles.py')
Copy-Item -LiteralPath (Join-Path $root 'scripts/package_install.ps1') -Destination (Join-Path $dist 'Install.ps1')
foreach($name in @('CMakeLists.txt','README.md','LICENSE')){
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination (Join-Path $dist "source/$name")
}
foreach($folder in @('docs/screenshots','source/docs/screenshots')){
    $destination=Join-Path $dist $folder
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Copy-Item -Path (Join-Path $root 'docs/screenshots/*.png') -Destination $destination -Force
}
Copy-Item -Path (Join-Path $root 'src/*') -Destination (Join-Path $dist 'source/src') -Force
foreach($name in @('build.ps1','fetch_dependencies.ps1','install.ps1','install_collectibles.ps1','uninstall.ps1','prepare_collectibles.py')){
    Copy-Item -LiteralPath (Join-Path $root "scripts/$name") -Destination (Join-Path $dist "source/scripts/$name")
}
if(Get-ChildItem -LiteralPath $dist -Recurse -File | Where-Object Name -eq 'Collectibles.bin'){throw 'Generated game data must not be packaged.'}
$zip=Join-Path $root 'artifacts/BPR-Remaining-Collectibles-1.1.1.zip'
Compress-Archive -Path (Join-Path $dist '*') -DestinationPath $zip -Force
Write-Output $zip
