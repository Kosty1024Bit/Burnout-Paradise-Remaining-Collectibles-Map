# Bottom-center speedometer for Burnout Paradise Remastered

Date: 2026-11-02. Agent: Codex. Status: verified by live telemetry and user driving confirmation.

Game: native Windows x86 D3D11, PE timestamp 0x5B365B73, image size 0x07F7E000. No existing loader. A dedicated dinput8.dll proxy forwards Windows system exports and starts a tiny MinHook DXGI Present overlay. It does not change gameplay data.

Source of telemetry knowledge: [matty-ross DashboardHud.cpp](https://github.com/matty-ross/bpr-mods-repository/blob/13d204f79991d09d70466dc99ce6f963ed5f7e85/mods/dashboard/src/DashboardHud.cpp), GPLv3. The game module pointer lives at RVA 0x00FFC8E0; GuiPlayerInfo is inline at module+0x8EFEC0 and integer speed in mph is +0x30. Convert abs(mph)×1.609344 and round. The pointer to RaceMainHudState is stored at module+0x7FABBC; check pointee+0x14C. Engine-state integer at GuiPlayerInfo+0x48 should be nonzero.

Verification: shared D3D11-renderer preview captured through staging texture and inspected. Game log showed initialization and real speed changing from 0 to 122/121/137 km/h. User drove without agent input and explicitly confirmed the HUD works. Fullscreen screenshot inspection was unsuccessful (WGC returned a thin black capture).

Gotchas:

1. RaceMainHudState is a pointer, while GuiPlayerInfo is inline; Core::Pointer::as<void*>() in the reference dereferences the stored pointer.
2. Windows processes launched outside the sandbox require capture outside the sandbox. Even then, fullscreen capture may remain unusable; do not claim visual verification from a black screenshot.
3. Start-Process -WindowStyle Hidden can leave a game hidden. Show its existing window without taking over input if the user will drive.
4. Avoid holding swapchain backbuffer references between frames so resize is not blocked. Restore all previous render targets and use ImGui's pipeline-state restoration.
5. um publish --game compares installed mod files with the mod package and calls them stock files. Run before installation or corroborate with pre-install file inventory.
6. Existing HUD speed is already rounded to whole mph, so a km/h display can advance in 1–2 km/h steps.

No stock assets, decompiled game code, protected keys or game binaries belong in the mod source or release. This note is local; no public contribution has been authorized.

## Remaining collectible map, 2026-11-04

Verified addition to the same proxy: observe native main-map events and current profile loading/completion queries, draw remaining-only ImGui markers. Game assets and saves stay unchanged. `prepare_collectibles.py` reads the user's compressed TRIGGERS.DAT resource v42; PC subtypes8/9/13 map to jump/smash/billboard. Console subtype numbers differ. There are86 jump entrance regions but65 unique jump keys,475 gates and165 billboards. Key is groupId if nonzero, otherwise regionId. Keep multiple entrance coordinates but count unique jumps.

Profile v31 has mainland uint64 sets at +0x7770, stride0x1008, length at set+0x1000. Island records start at profile+0x282F8: billboard entries+0/length+0x168, smash+0x16C/length+0x3C4, jump+0x3C8/length+0x440. Island accessors compare low32-bit IDs. Profile loading at VA0xA078F0 takes two stack arguments. Completion-query body VA0x03215650 takes type and uint64 ID. Observe pointers; never change results.

Main-map event template body VA0x032C4EA0 (original entry0x4F9780) takes one event pointer. The48-byte event carries two float4 rects, textures pointer, zoom, map type and active flag. Normalized-to-device matrix atVA0x149A1F0 has three16-byte rows; normalized full rect is0..1. Device GUI is1280×720 even at2560×1421, so scale to client dimensions. Never show stale map data or render during online sessions.

Oracle: inspect artifacts/map-test_small.png; log completed41/65 jumps,331/475 gates,100/165 billboards. Human collected a gate; log advanced331→332 and human confirmed marker disappearance/count decrement. All hooks MH_OK, existing speedometer continues working. Snapshot through a D3D11 staging texture works even when fullscreen WGC fails.

Gotcha: instruction mnemonics do not uniquely determine bytes. `mov ebx,esp` is89E3 in this executable, and `mov ebp,esp` is89E5 in relocated functions; guards initially assumed alternate encodings. Log actual runtime bytes and guard those exactly. No executable/DRM changes or bypasses needed.

References: [archive format](https://github.com/DGIorio/bundle_packer_unpacker), [public reconstruction for semantic hints](https://github.com/BurnoutDecomp/b5-decomp). All PC widths, offsets, types and counts independently checked against local executable/data and live runtime. No reconstructed source is distributed.
