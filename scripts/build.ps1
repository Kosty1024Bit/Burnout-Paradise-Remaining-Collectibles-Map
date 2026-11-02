$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$cmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
& $cmake -S $root -B (Join-Path $root 'build') -G 'Visual Studio 17 2022' -A Win32
if ($LASTEXITCODE) { throw 'CMake configure failed' }
& $cmake --build (Join-Path $root 'build') --config Release --parallel 4
if ($LASTEXITCODE) { throw 'Build failed' }
