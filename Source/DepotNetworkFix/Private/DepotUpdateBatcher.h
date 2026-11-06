#pragma once

namespace DepotNetworkFix
{
	// Hooks the depot's item-amount broadcast and starts the flush ticker.
	void StartBatching();

	// Removes the hook and the ticker and drops unsent updates.
	void StopBatching();
}
