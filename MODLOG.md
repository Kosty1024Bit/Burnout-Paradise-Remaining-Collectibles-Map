# Burnout Paradise Remastered: center speedometer

2026-11-02. User requested a bottom-center speedometer in km/h for D:\Burnout Paradise Remastered.

- Workspace: C:\Users\konst\Documents\ChatGPT\BurnoutPR
- Native x86, D3D11, PE timestamp 0x5B365B73; image base 0x00400000, image size 0x07F7E000.
- EXE SHA256 E08528C1F696FAB1D1552576CF3E63537253E1F7A264809C919BA2DE5A982CC9.
- No existing mod loader or anti-cheat found by um scan. Game not running at intake.
- Community reference: https://github.com/matty-ross/bpr-mods-repository (GPLv3); DashboardHud.cpp documents GuiPlayerInfo speed in integer mph at gameModule+0x8EFEC0+0x30. gameModule pointer VA 0x013FC8E0. RaceMainHudState pointer at gameModule+0x7FABBC, HUD active bool at pointee+0x14C; engine state at GuiPlayerInfo+0x48.
- Chosen route: dedicated dinput8 proxy (game imports DirectInput8Create), MinHook DXGI Present hook, own ImGui context and D3D11 rendering. No vehicle/physics changes. Relative addresses, guarded telemetry reads, hide outside driving HUD.
- Sources downloaded to third_party, excluded from git. ImGui and MinHook MIT; reference GPLv3 credited.
- Game installation must wait for explicit loader approval required by mod-any-game/SKILL.md, and filesystem escalation for D:.
- Build: MSVC 19.44.35221, Windows SDK 10.0.22621, CMake Win32 Release. Plugin plus shared-renderer preview compile successfully. D3D11 preview readback inspected: centered bottom card, Cyrillic КМ/Ч, readable at 1280×720. Conversion checks 0/100/-100/200 mph pass.
- Installed after explicit tool approval at 21:15 Moscow. Added dinput8.dll and CenterSpeedometer.ini only. Receipt: artifacts/install-receipt.json. Profile backup: backups/um/backups/bpr-before-center-speedometer/20261102-211522.zip, 104 files, 6.9 MB. Root inventory: backups/install-20261102-211521/game-root-before.json.
- Runtime: PID 23164; launch -skipvideos. Log confirms load, DXGI Present hook and renderer initialization. Actual driving HUD at 2560×1440 reports 0, 122, 121, 137 km/h. User chose to drive themselves; no keyboard or mouse automation performed. Initially hidden launch window was shown without activation or input.
- Verification complete: user explicitly confirmed «Всё работает.» on 2026-11-02. WGC screenshots returned a thin black image for this fullscreen game; no claim of successful in-game screenshot inspection. User confirmation plus live telemetry is the in-game oracle.
- um publish check dist passed without --game. The --game comparison falsely flags our two newly installed mod files as game assets; pre-install inventory proves they were absent. No stock game file is shipped.
- Dependencies pinned: bpr-mods 13d204f79991d09d70466dc99ce6f963ed5f7e85; ImGui b48d1afbe8ee8b238e2961dc363a949dd7304e23; MinHook 8af6b4acae5a9388fd742b56fa79ece89d96f823.
- No public upload or knowledge-base PR requested or performed.
