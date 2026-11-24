using UnrealBuildTool;

public class CreepyBroskyEditorTarget : TargetRules
{
    public CreepyBroskyEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CreepyBrosky");
    }
}
