# Depot Network Fix

Satisfactory mod (SML), server only. Merges the Dimensional Depot's item-count updates and sends them about once a
second, held back while a player's game is behind, so busy depots no longer get players kicked with
"Outgoing reliable buffer overflow".

- How it works: [Docs/Architecture.md](Docs/Architecture.md)
- Logs: [Docs/Debugging.md](Docs/Debugging.md)
- Mod page texts: [Docs/Release](Docs/Release)

## Build

From `D:\SFMod\Tools\BuildScripts`:

- `build-editor.bat`: editor build (needed once so other mods can still be packaged)
- `package-depotnetworkfix.bat local`: Windows client test zip
- `package-depotnetworkfix.bat`: release zip (Windows client, Windows and Linux server)

Output: `SatisfactoryModLoader\Saved\ArchivedPlugins\DepotNetworkFix\DepotNetworkFix.zip`.

Icon: `python Tools/make_icon.py`.
