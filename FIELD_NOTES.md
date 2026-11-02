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
