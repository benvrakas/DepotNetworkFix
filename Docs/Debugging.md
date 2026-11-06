# Debugging Depot Network Fix

Log category: `LogDepotNetworkFix`, in the server's `FactoryGame/Saved/Logs/FactoryGame.log` (dedicated server) or the
host's `%LOCALAPPDATA%\FactoryGame\Saved\Logs\FactoryGame.log`.

- `Hooked AFGCentralStorageSubsystem::Client_ItemAmountUpdatedInCentralStorage` at startup: the fix is loaded.
- `Could not hook ...` (Error): the game build changed the function; depot updates are sent as in the base game.
- `Batching depot updates of CentralStorageSubsystem (dedicated server) ...`: the first depot change of a session
  was caught.
- `Last 60 s: N depot updates from the game, M sent (largest send L), S unchanged skipped, H sends held back,
  highest player queue Q of 511`: once a minute while the depot changes. N far above M means the batching works.
  Q close to 511 would mean a player still came near the kick limit.
- `Holding N depot updates back: a player has Q unacknowledged messages on the depot channel` and
  `Sending depot updates again`: a player's game fell behind (usually while loading in) and sends waited for it.

Each flush is logged at Verbose. To see it, add to the server's `Engine.ini` (`Saved/Config/LinuxServer/Engine.ini`
on a Linux server, edited while the server is stopped):

```ini
[Core.Log]
LogDepotNetworkFix=Verbose
```

## Checking the fix

On a server with busy depot uploaders, join and stay in game. Without the mod, the server log shows
`Closing connection. Can't send function 'Client_ItemAmountUpdatedInCentralStorage' ... RPCReliableBufferOverflow`
and the player is kicked. With it, that line no longer appears and the summary lines show the merged sends.
