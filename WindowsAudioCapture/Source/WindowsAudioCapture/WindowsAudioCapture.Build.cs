using UnrealBuildTool;

public class WindowsAudioCapture : ModuleRules
{
    public WindowsAudioCapture(ReadOnlyTargetRules Target) : base(Target)
    {

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				//"WindowsAudioCapture/Public"
				// ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				//"WindowsAudioCapture/Private",
				// ... add other private include paths required here ...
//                 "../Plugins/FX/Niagara/Source/Niagara/Public"
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                //"ApplicationCore",
                //"CoreUObject",
                //"Engine",
                //"InputCore",
                //"RenderCore",
                //"ShaderCore",
                "Kiss_FFT",
                "Niagara",
                "NiagaraCore",
                "NiagaraShader",
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "VectorVM",
                "RHI",
                "NiagaraVertexFactories",
                "RenderCore"
                
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );


    }
}
