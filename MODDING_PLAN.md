# Burnout Paradise Remastered modding plan

- Install: D:\Burnout Paradise Remastered; native Windows x86, D3D11.
- Scope: visual speedometer for offline driving, bottom center, kilometers per hour.
- Existing community loader: Bo98 BPR Modder; full dashboard available in matty-ross/bpr-mods-repository. A small dedicated proxy avoids unrelated mod-menu and gameplay patches.
- Load: local dinput8.dll forwards the real Windows DirectInput exports. Rendering uses MinHook and ImGui; speed reads existing HUD telemetry, mph × 1.609344, rounded.
- Profile: C:\Users\konst\AppData\Local\Criterion Games\Burnout Paradise Remastered.
- Lab: preserve existing game files and profile before installation; verify shared rendering in a preview app, then actual game telemetry, menus and driving after loader approval.
- Undo: remove only added plugin/config/log files when game is closed; restore profile from workspace backups only if needed.
- Unknowns: live HUD layout compatibility with this executable, launch/menu navigation and visual positioning at 2560×1440.
