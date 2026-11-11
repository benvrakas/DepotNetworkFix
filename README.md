# Depot Network Fix

Satisfactory mod (SML), server only. Merges the Dimensional Depot's item-count updates and sends them about once a
second, held back while a player's game is behind, so busy depots no longer get players kicked with
"Outgoing reliable buffer overflow".

- How it works: [Docs/Architecture.md](Docs/Architecture.md)
- Logs: [Docs/Debugging.md](Docs/Debugging.md)
- Mod page texts: [Docs/Release](Docs/Release)

## Build

From `D:\SFMod\Tools\BuildScripts` (see its README.md):

- `sfmod editor`: editor build (needed once so other mods can still be packaged)
- `sfmod package DepotNetworkFix local`: Windows client test zip
- `sfmod package DepotNetworkFix`: release zips (Windows client, Windows and Linux server)
- `sfmod release DepotNetworkFix`: GitHub release of the packaged zips

Output: `SatisfactoryModLoader\Saved\ArchivedPlugins\DepotNetworkFix\`.

Icon: `python Tools/make_icon.py`.
