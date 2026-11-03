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

## Remaining collectibles map (2026-11-03/04)

- User requests remaining smash gates, billboards and super jumps on menu map or minimap.
- Preserve working center speedometer; extend existing proxy if practical. No game/profile writes yet.
- `um scan` confirms x86 native and no detected anti-cheat. Offline scope.
- KB search Burnout Paradise returned no notes. Existing matty-ross source repositories do not implement collectibles filtering.
- Read-only local executable analysis: profile total billboards at +0xA780, super jumps +0x8770 (references at VA 0xA67F09 and 0xA67F2C). These are totals only, not sufficient for per-object filtering.
- Research references: https://github.com/matty-ross/bpr-mods-repository ; https://github.com/matty-ross/bpr-open-mods ; https://github.com/BurnoutDecomp/b5-decomp . Public reconstructed source checked out sparsely in TEMP/bpr-decomp-reference, outside distributable. Console layouts are hints only; verify each against user's PC binary.
- Tools: Capstone 5.0.9 and pefile 2024.8.26 installed under ignored third_party/re_tools. Sandbox cannot read their elevated install ACL; analysis script uses approved escalation.
- Next: identify per-object records and PC coordinate transform; then implement read-only marker renderer, build and test.
- Trigger resource is version 42, PC `bnd2` with zlib stream; generic table at resource+0x44, 56-byte stride. PC subtype mapping differs from old console: 8=jump, 9=smash, 13=billboard. Found 86 jump entry regions/65 unique keys, 475 smash gates, 165 billboards. Keys are groupId(+44) or ownId(+36), world X/Z at +0/+8. Converter validates all unique totals.
- PC profile version 31: mainland sets at +0x7770 + type*0x1008, 512 uint64 entries, count at +0x1000. Island data at profile+0x282F8: billboard records +0/count +0x168, smash +0x16C/count +0x3C4, jump +0x3C8/count +0x440. Island lookup compares low ID words. Profile Deserialise at VA 0xA078F0, ret 8; IsStuntElementDone static body 0x03215650.
- Native MainMap OutputViewState entry VA 0x4F9780 (static thunk to 0x032C4EA0), thiscall(event*), event size48: world rect, normalized view rect, textures ptr, zoom float, map type, active byte. Type0=main menu map. Normalised-to-device matrix VA0x149A1F0, 3 padded rows. Normalized rect is 0..1, not -1..1.
- First build and converter passed. Installed after tool approval; backup backups/collectibles-20261104-001457, profile backup backups/um/backups/bpr-before-collectibles/20261104-001458.zip.
- First runtime PID23500 loads renderer and preserves speedometer, but collectibles hook disabled itself (unsupported map callback); no markers yet. User opened map. Must diagnose runtime entry bytes/status; do not call this verified.
- External read-only OpenProcess probe returned access denied. No protection bypass attempted. Added in-process hook diagnostic logging for next normal launch.
- Runtime diagnosis: Capstone prints `mov ebx,esp` / `mov ebp,esp`, but the binary encodes them as 89 E3 / 89 E5. Initial guards incorrectly assumed 8B DC / 8B EC. Fixed the guards; this was an implementation error, not an incompatible game.
- At 00:20:56 Moscow all three hooks return MH_OK(create/enable), PID3100. DLL installed with profile backup 20261104-002051. Waiting for actual map capture and progress log to validate rendering/filtering.
- VERIFIED at 00:22–00:24: native menu map photographed successfully through WGC (artifacts/map-test_small.png) and inspected. All three marker shapes align with world roads and follow user pan/zoom; legend reads remaining counts24/144/65. Native backbuffer snapshot also saved in game folder.
- Live profile reported completed41 jumps,331 gates,100 billboards. At00:23:48 gates advanced332 and remaining143. User explicitly confirmed: «Метка исчезла, счётчик уменьшился». Speedometer continued reporting actual driving speeds throughout. No automated keyboard/mouse input used.
- Package lint: um publish check dist passes, no failures/warnings. Generated marker cache is excluded; installer reconstructs it from own game files.
