# Depot Network Fix: how it works

## The problem

`AFGCentralStorageSubsystem` (the Dimensional Depot network) calls
`Client_ItemAmountUpdatedInCentralStorage(ItemClass, NewAmount)` every time an item amount changes. It is a
`NetMulticast, Reliable` RPC, so the server creates one reliable bunch per change on the subsystem's actor channel of
every connection.

A connection may have at most `RELIABLE_BUFFER - 1` = 511 unacknowledged reliable bunches on one channel
(`FOutBunch::FOutBunch`, `Channel->NumOutRec >= RELIABLE_BUFFER-1+bClose` sets the bunch to overflowed). The next
reliable RPC then fails in `UNetDriver::ProcessRemoteFunctionForChannelPrivate`, which logs
`Closing connection. Can't send function 'Client_ItemAmountUpdatedInCentralStorage' ... Reliable buffer overflow` and
closes the connection with `RPCReliableBufferOverflow`. The client shows "Outgoing reliable buffer overflow", or a
"Connection TIMED OUT" 30 s later if it never received the close.

A client that is loading after a join (or hitching) does not acknowledge for seconds. With depot uploaders running
fast (Enhanced Depot raises uploader rates up to 1200 items/min), hundreds of changes pile up in that time. Seen on
the IB server on 2026-11-05/06: every kick in a run was this RPC, 30-45 s after joining. The network rate (120000 vs
300000) made no difference, as the limit is a bunch count, not bandwidth.

The RPC is redundant for state: `mStoredItems` is a replicated property (`OnRep_StorageUpdated`), so clients get the
contents through normal property replication. The RPC only drives `mOnCentralStorageItemAmountUpdatedDelegate`
(UI refreshes).

## The fix

`DepotUpdateBatcher.cpp` hooks the RPC's UHT stub on the server (authority, dedicated or listen server):

- Each call is cancelled and its item's latest amount stored (`Pending`, in first-seen order).
- A core ticker flushes every 1 s: it calls the real RPC once per pending item, skipping amounts equal to the last
  one sent, while a re-entrancy flag lets those calls through the hook.
- Before each flush it reads `NumOutRec` of the subsystem's actor channel on every client connection. A flush sends at
  most `RELIABLE_BUFFER / 2 - highest queue` updates and waits entirely while any queue is at or above 256. Unsent
  items stay pending, still merged.

Single player and clients are untouched (`ShouldBatch`). `Client_NewItemAddedToCentralStorage` is rare and not
batched.

The RPC is `protected`. `FCentralStorageAccess` re-declares it public with a `using`-declaration so its address can be
taken; `&FCentralStorageAccess::Client_ItemAmountUpdatedInCentralStorage` has the base class member pointer type. This
avoids an access-transformer friend, which would recompile FactoryGame. The struct is never instantiated.

The hooked function returns `void`, so SML's Linux by-value return bug does not apply.

## Multiplayer

The uplugin sets `"RequiredOnRemote": false`: only the server needs the mod, and clients with or without it can join.
