$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    if (-not (Test-Path 'third_party/bpr-mods/.git')) {
        git clone https://github.com/matty-ross/bpr-mods-repository.git third_party/bpr-mods
        if ($LASTEXITCODE) { throw 'bpr-mods clone failed' }
        git -C third_party/bpr-mods checkout 13d204f79991d09d70466dc99ce6f963ed5f7e85
        if ($LASTEXITCODE) { throw 'bpr-mods checkout failed' }
    }
    git -C third_party/bpr-mods submodule update --init --depth 1 vendor/imgui/modules/imgui
    if ($LASTEXITCODE) { throw 'ImGui download failed' }
    if (-not (Test-Path 'third_party/minhook/.git')) {
        git clone https://github.com/TsudaKageyu/minhook.git third_party/minhook
        if ($LASTEXITCODE) { throw 'MinHook clone failed' }
        git -C third_party/minhook checkout 8af6b4acae5a9388fd742b56fa79ece89d96f823
        if ($LASTEXITCODE) { throw 'MinHook checkout failed' }
    }
} finally { Pop-Location }
