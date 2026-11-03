# Burnout Paradise Remastered modding plan

- Install: D:\Burnout Paradise Remastered; native Windows x86, D3D11.
- Scope: visual speedometer for offline driving, bottom center, kilometers per hour.
- Existing community loader: Bo98 BPR Modder; full dashboard available in matty-ross/bpr-mods-repository. A small dedicated proxy avoids unrelated mod-menu and gameplay patches.
- Load: local dinput8.dll forwards the real Windows DirectInput exports. Rendering uses MinHook and ImGui; speed reads existing HUD telemetry, mph × 1.609344, rounded.
- Profile: C:\Users\konst\AppData\Local\Criterion Games\Burnout Paradise Remastered.
- Lab: preserve existing game files and profile before installation; verify shared rendering in a preview app, then actual game telemetry, menus and driving after loader approval.
- Undo: remove only added plugin/config/log files when game is closed; restore profile from workspace backups only if needed.
- Unknowns: live HUD layout compatibility with this executable, launch/menu navigation and visual positioning at 2560×1440.

## Remaining collectible markers

- Extend existing dinput8 proxy with read-only profile observation and native main-map event observation.
- Read own game's TRIGGERS.DAT with a validating Python converter (stdlib only). Never distribute generated Collectibles.bin.
- Render own ImGui markers in the native main-map viewport, with category keys F6–F9; offline only.
- Validate current-profile completion on mainland and Big Surf Island and inspect a backbuffer capture, avoiding fullscreen WGC failure.
- Backups and installation receipts live under backups/collectibles-* and artifacts/collectibles-install-receipt.json. Preserve CenterSpeedometer.ini.
