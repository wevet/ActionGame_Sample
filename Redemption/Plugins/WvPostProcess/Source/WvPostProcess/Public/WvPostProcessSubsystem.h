
#pragma once

#include "CoreMinimal.h"
#include "PostProcess/PostProcessLensFlares.h" // For PostProcess delegate
#include "WvPostProcessSubsystem.generated.h"


DECLARE_MULTICAST_DELEGATE_FourParams(FPP_LensFlares, FRDGBuilder&, const FViewInfo&, const FLensFlareInputs&, FLensFlareOutputsData&);
extern RENDERER_API FPP_LensFlares PP_LensFlares;

class UPostProcessLensFlareAsset;

namespace
{
	TAutoConsoleVariable<int32> CVarLensFlareRenderBloom(TEXT("r.LensFlare.RenderBloom"), 1, TEXT(" 0: Don't mix Bloom into lens-flare\n") TEXT(" 1: Mix the Bloom into the lens-flare"), ECVF_RenderThreadSafe);
	TAutoConsoleVariable<int32> CVarLensFlareRenderFlarePass(TEXT("r.LensFlare.RenderFlare"), 1, TEXT(" 0: Don't render flare pass\n") TEXT(" 1: Render flare pass (ghosts and halos)"), ECVF_RenderThreadSafe);
	TAutoConsoleVariable<int32> CVarLensFlareRenderGlarePass(TEXT("r.LensFlare.RenderGlare"), 1, TEXT(" 0: Don't render glare pass\n") TEXT(" 1: Render flare pass (star shape)"), ECVF_RenderThreadSafe);

}

UCLASS()
class WVPOSTPROCESS_API UWvPostProcessSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	// The reference to the data asset storing the settings
	UPROPERTY(Transient)
	UPostProcessLensFlareAsset* PostProcessAsset;

	void RenderLensFlare(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FLensFlareInputs& Inputs, FLensFlareOutputsData& Outputs);
	FRDGTextureRef RenderThreshold(FRDGBuilder& GraphBuilder, FRDGTextureRef InputTexture, FIntRect& InputRect, const FViewInfo& View);
	FRDGTextureRef RenderFlare(FRDGBuilder& GraphBuilder, FRDGTextureRef InputTexture, FIntRect& InputRect, const FViewInfo& View);
	FRDGTextureRef RenderGlare(FRDGBuilder& GraphBuilder, FRDGTextureRef InputTexture, FIntRect& InputRect, const FViewInfo& View);
	FRDGTextureRef RenderBlur(FRDGBuilder& GraphBuilder, FRDGTextureRef InputTexture, const FViewInfo& View, const FIntRect& Viewport, int BlurSteps);

	// Cached blending and sampling states
	// which are re-used across render passes
	FRHIBlendState* ClearBlendState = nullptr;
	FRHIBlendState* AdditiveBlendState = nullptr;

	FRHISamplerState* BilinearClampSampler = nullptr;
	FRHISamplerState* BilinearBorderSampler = nullptr;
	FRHISamplerState* BilinearRepeatSampler = nullptr;
	FRHISamplerState* NearestRepeatSampler = nullptr;
};


