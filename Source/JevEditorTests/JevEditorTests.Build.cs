using UnrealBuildTool;

public class JevEditorTests : ModuleRules
{
	public JevEditorTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Jev" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Jev",
			"Json",
			"FunctionalTesting"
		});
	}
}
