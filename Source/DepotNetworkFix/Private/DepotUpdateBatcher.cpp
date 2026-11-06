#include "DepotUpdateBatcher.h"

#include "Containers/Ticker.h"
#include "DepotNetworkFixModule.h"
#include "Engine/ActorChannel.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "FGCentralStorageSubsystem.h"
#include "Patching/NativeHookManager.h"
#include "Resources/FGItemDescriptor.h"

namespace
{
	using FAmountRpc = void (AFGCentralStorageSubsystem::*)(TSubclassOf<UFGItemDescriptor>, int32);

	// Makes the protected item-amount RPC addressable for the hook and for resending.
	struct FCentralStorageAccess : AFGCentralStorageSubsystem
	{
		using AFGCentralStorageSubsystem::Client_ItemAmountUpdatedInCentralStorage;
		static constexpr FAmountRpc AmountRpc = &FCentralStorageAccess::Client_ItemAmountUpdatedInCentralStorage;
	};

	constexpr FAmountRpc AmountRpc = FCentralStorageAccess::AmountRpc;
	using FAmountRpcHook = HookInvoker<FAmountRpc, AmountRpc>;
	constexpr const TCHAR* AmountRpcName = TEXT("AFGCentralStorageSubsystem::Client_ItemAmountUpdatedInCentralStorage");

	// Seconds between two sends of the merged updates.
	constexpr double FlushIntervalSeconds = 1.0;
	// Seconds between two ticker runs.
	constexpr float TickerIntervalSeconds = 0.1f;
	// Highest number of unacknowledged reliable messages on the depot's channel (any player) before sends wait.
	constexpr int32 QueueSoftLimit = RELIABLE_BUFFER / 2;
	// Seconds between two summary log lines.
	constexpr double SummaryIntervalSeconds = 60.0;

	struct FBatchState
	{
		TWeakObjectPtr<AFGCentralStorageSubsystem> Subsystem;
		TMap<TWeakObjectPtr<UClass>, int32> Pending;
		TArray<TWeakObjectPtr<UClass>> PendingOrder;
		TMap<TWeakObjectPtr<UClass>, int32> LastSent;
		double LastFlushTime = 0.0;
		double LastSummaryTime = 0.0;
		bool bHeldBack = false;

		int64 Received = 0;
		int64 Sent = 0;
		int64 Skipped = 0;
		int64 HeldBackFlushes = 0;
		int32 HighestQueue = 0;
		int32 LargestSend = 0;

		void ResetStats()
		{
			Received = Sent = Skipped = HeldBackFlushes = 0;
			HighestQueue = LargestSend = 0;
		}
	};

	FBatchState GState;
	bool GPassThrough = false;
	FDelegateHandle GHookHandle;
	FTSTicker::FDelegateHandle GTickerHandle;

	const TCHAR* NetModeName(ENetMode Mode)
	{
		switch (Mode)
		{
		case NM_DedicatedServer: return TEXT("dedicated server");
		case NM_ListenServer: return TEXT("host");
		case NM_Client: return TEXT("client");
		default: return TEXT("single player");
		}
	}

	// Returns true when Self is the server's depot subsystem in a session that other players can join.
	bool ShouldBatch(const AFGCentralStorageSubsystem* Self)
	{
		if (!Self || !Self->HasAuthority())
		{
			return false;
		}
		const UWorld* World = Self->GetWorld();
		if (!World)
		{
			return false;
		}
		const ENetMode Mode = World->GetNetMode();
		return Mode == NM_DedicatedServer || Mode == NM_ListenServer;
	}

	void Rebind(AFGCentralStorageSubsystem* Self)
	{
		if (GState.Subsystem.Get() == Self)
		{
			return;
		}
		if (GState.Pending.Num() > 0)
		{
			UE_LOG(LogDepotNetworkFix, Warning, TEXT("Dropped %d unsent depot updates of the previous world"), GState.Pending.Num());
		}
		GState.Subsystem = Self;
		GState.Pending.Reset();
		GState.PendingOrder.Reset();
		GState.LastSent.Reset();
		GState.bHeldBack = false;
		GState.LastFlushTime = FPlatformTime::Seconds();
		GState.LastSummaryTime = GState.LastFlushTime;
		GState.ResetStats();
		UE_LOG(LogDepotNetworkFix, Log, TEXT("Batching depot updates of %s (%s), at most one send per %.1f s, held back while a player has %d+ unacknowledged messages"),
			*GetNameSafe(Self), NetModeName(Self->GetWorld()->GetNetMode()), FlushIntervalSeconds, QueueSoftLimit);
	}

	void Queue(AFGCentralStorageSubsystem* Self, UClass* ItemClass, int32 NewAmount)
	{
		Rebind(Self);
		++GState.Received;
		const TWeakObjectPtr<UClass> Key(ItemClass);
		if (int32* Existing = GState.Pending.Find(Key))
		{
			*Existing = NewAmount;
			return;
		}
		GState.Pending.Add(Key, NewAmount);
		GState.PendingOrder.Add(Key);
	}

	// Returns the highest number of unacknowledged reliable messages on the depot's channel over all players.
	int32 HighestReliableQueue(AFGCentralStorageSubsystem* Subsystem)
	{
		const UWorld* World = Subsystem->GetWorld();
		const UNetDriver* Driver = World ? World->GetNetDriver() : nullptr;
		if (!Driver)
		{
			return 0;
		}
		int32 Highest = 0;
		for (UNetConnection* Connection : Driver->ClientConnections)
		{
			if (!Connection)
			{
				continue;
			}
			if (const UActorChannel* Channel = Connection->FindActorChannelRef(Subsystem))
			{
				Highest = FMath::Max(Highest, Channel->NumOutRec);
			}
		}
		return Highest;
	}

	void LogSummary(double Now)
	{
		if (Now - GState.LastSummaryTime < SummaryIntervalSeconds)
		{
			return;
		}
		if (GState.Received > 0)
		{
			UE_LOG(LogDepotNetworkFix, Log, TEXT("Last %.0f s: %lld depot updates from the game, %lld sent (largest send %d), %lld unchanged skipped, %lld sends held back, highest player queue %d of %d"),
				Now - GState.LastSummaryTime, GState.Received, GState.Sent, GState.LargestSend, GState.Skipped, GState.HeldBackFlushes,
				GState.HighestQueue, RELIABLE_BUFFER - 1);
		}
		GState.LastSummaryTime = Now;
		GState.ResetStats();
	}

	void Flush(AFGCentralStorageSubsystem* Subsystem)
	{
		const int32 PlayerQueue = HighestReliableQueue(Subsystem);
		GState.HighestQueue = FMath::Max(GState.HighestQueue, PlayerQueue);
		const int32 Budget = QueueSoftLimit - PlayerQueue;
		if (Budget <= 0)
		{
			++GState.HeldBackFlushes;
			if (!GState.bHeldBack)
			{
				GState.bHeldBack = true;
				UE_LOG(LogDepotNetworkFix, Log, TEXT("Holding %d depot updates back: a player has %d unacknowledged messages on the depot channel"),
					GState.PendingOrder.Num(), PlayerQueue);
			}
			return;
		}
		if (GState.bHeldBack)
		{
			GState.bHeldBack = false;
			UE_LOG(LogDepotNetworkFix, Log, TEXT("Sending depot updates again (player queue %d)"), PlayerQueue);
		}

		TGuardValue<bool> PassThrough(GPassThrough, true);
		int32 SentNow = 0;
		int32 Consumed = 0;
		while (Consumed < GState.PendingOrder.Num() && SentNow < Budget)
		{
			const TWeakObjectPtr<UClass> Key = GState.PendingOrder[Consumed++];
			int32 Amount = 0;
			if (!GState.Pending.RemoveAndCopyValue(Key, Amount))
			{
				continue;
			}
			UClass* ItemClass = Key.Get();
			if (!ItemClass)
			{
				continue;
			}
			const int32* Previous = GState.LastSent.Find(Key);
			if (Previous && *Previous == Amount)
			{
				++GState.Skipped;
				continue;
			}
			(Subsystem->*AmountRpc)(TSubclassOf<UFGItemDescriptor>(ItemClass), Amount);
			GState.LastSent.Add(Key, Amount);
			++SentNow;
		}
		GState.PendingOrder.RemoveAt(0, Consumed, EAllowShrinking::No);
		GState.Sent += SentNow;
		GState.LargestSend = FMath::Max(GState.LargestSend, SentNow);
		UE_LOG(LogDepotNetworkFix, Verbose, TEXT("Sent %d depot updates (player queue was %d, %d still waiting)"), SentNow, PlayerQueue, GState.PendingOrder.Num());
	}

	bool Tick(float)
	{
		AFGCentralStorageSubsystem* Subsystem = GState.Subsystem.Get();
		if (!Subsystem)
		{
			if (GState.PendingOrder.Num() > 0)
			{
				GState.Pending.Reset();
				GState.PendingOrder.Reset();
			}
			return true;
		}
		const double Now = FPlatformTime::Seconds();
		if (GState.PendingOrder.Num() > 0 && Now - GState.LastFlushTime >= FlushIntervalSeconds)
		{
			GState.LastFlushTime = Now;
			Flush(Subsystem);
		}
		LogSummary(Now);
		return true;
	}
}

namespace DepotNetworkFix
{
	void StartBatching()
	{
		FAmountRpcHook::InstallHook(AmountRpcName);
		GHookHandle = FAmountRpcHook::AddHandlerBefore(
			[](auto& Scope, AFGCentralStorageSubsystem* Self, TSubclassOf<UFGItemDescriptor> ItemClass, int32 NewAmount)
			{
				if (GPassThrough || !ShouldBatch(Self))
				{
					return;
				}
				Queue(Self, ItemClass.Get(), NewAmount);
				Scope.Cancel();
			});
		if (!GHookHandle.IsValid())
		{
			UE_LOG(LogDepotNetworkFix, Error, TEXT("Could not hook %s; depot updates are sent as in the base game"), AmountRpcName);
			return;
		}
		GTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick), TickerIntervalSeconds);
		UE_LOG(LogDepotNetworkFix, Log, TEXT("Hooked %s"), AmountRpcName);
	}

	void StopBatching()
	{
		if (GTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GTickerHandle);
			GTickerHandle.Reset();
		}
		if (GHookHandle.IsValid())
		{
			FAmountRpcHook::RemoveHandler(AmountRpcName, GHookHandle);
			GHookHandle.Reset();
		}
		GState = FBatchState();
	}
}
