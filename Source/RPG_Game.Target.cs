using UnrealBuildTool;
using System.Collections.Generic;

public class RPG_GameTarget : TargetRules
{
    public RPG_GameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("RPG_Game");
    }
}

