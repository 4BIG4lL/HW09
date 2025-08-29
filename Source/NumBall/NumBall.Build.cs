using UnrealBuildTool;

public class NumBall : ModuleRules
{
	public NumBall(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core"
			, "CoreUObject"
			, "Engine"
			, "InputCore"
			, "EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
		
		PublicIncludePaths.AddRange(new string[]
		{
			"NumBall"
		});

	}
}
