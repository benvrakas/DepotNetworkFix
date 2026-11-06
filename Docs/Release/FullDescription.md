## Depot Network Fix

Getting kicked from your server with **"Outgoing reliable buffer overflow"**, or a **"Connection TIMED OUT"** about
half a minute after joining? If your factory feeds a lot of items into Dimensional Depots, this is probably why.

Every time the amount of an item in your depots changes, the server tells every player about it, one message per
change, and each message has to be confirmed by the player's game. A busy factory changes depot amounts many times a
second, and faster still with mods that speed up depot uploaders. While your game is busy, for example loading the
world right after you join, it can't confirm messages fast enough. Once 511 of them are waiting, the server gives up
on you and closes the connection.

This mod stops that. The server keeps the latest amount of each item and sends them together about once a second
instead of one message per change, and it holds back when a player's game is already behind. Players are no longer
kicked, and the numbers you see still update every second or so.

### What changes

- Depot item counts on the depot screens and in the build menu update about once a second instead of instantly.
- Nothing else. What's actually in your depots, and what you can build or upload with them, is unchanged. Players
  joining later still get the full depot contents from the game as usual.

### Who needs it

**Only the server**: your dedicated server, or the player hosting the game. Players joining don't need to install it,
and can join with or without it.

It does nothing in single player.

### Performance

It uses less network traffic than the base game, not more: a few merged messages a second instead of one per change.

### Source

[github.com/benvrakas/DepotNetworkFix](https://github.com/benvrakas/DepotNetworkFix). Bug reports and questions go to
its [issues](https://github.com/benvrakas/DepotNetworkFix/issues).
