#include "DepotNetworkFixModule.h"

#include "DepotUpdateBatcher.h"

DEFINE_LOG_CATEGORY(LogDepotNetworkFix);

void FDepotNetworkFixModule::StartupModule()
{
	if (WITH_EDITOR)
	{
		return;
	}
	DepotNetworkFix::StartBatching();
}

void FDepotNetworkFixModule::ShutdownModule()
{
	DepotNetworkFix::StopBatching();
}

IMPLEMENT_MODULE(FDepotNetworkFixModule, DepotNetworkFix)
