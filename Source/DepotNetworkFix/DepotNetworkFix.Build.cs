using UnrealBuildTool;

public class DepotNetworkFix : ModuleRules
{
	public DepotNetworkFix(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"FactoryGame",
			"SML",
			"DummyHeaders"
		});
	}
}
