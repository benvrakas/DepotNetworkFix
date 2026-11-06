<!-- ficsit.app page texts for Depot Network Fix (DepotNetworkFix). -->

# Name

Depot Network Fix

# Mod reference

DepotNetworkFix

# SHORT DESCRIPTION

Stops servers kicking players with "Outgoing reliable buffer overflow" when Dimensional Depots are busy. Server only.

# FULL DESCRIPTION

See `FullDescription.md`.

# SOURCE

https://github.com/benvrakas/DepotNetworkFix

# NETWORK ACTIVITY TRANSPARENCY

This mod makes no network connections of its own. On a server, it changes how often the game's own Dimensional Depot
item-count messages are sent to players: merged about once a second instead of one per change. It sends nothing new.

# AI USAGE TRANSPARENCY

This mod was investigated and written with Anthropic Claude (Claude Code): finding the cause in the server logs and
the engine source, the code, the documentation and the icon (drawn by a script). benvrakas directed the work.
