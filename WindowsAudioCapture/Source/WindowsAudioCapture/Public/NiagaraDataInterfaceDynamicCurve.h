// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "CoreMinimal.h"
#include "DSP/MultithreadedPatching.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceAudio.h"
#include "NiagaraShared.h"
#include "StaticMeshResources.h"
#include "VectorVM.h"

#include "NiagaraDataInterfaceDynamicCurve.generated.h"

/**
 * UNiagaraDataInterfaceDynamicCurve provides curve data access to Niagara
 * This is based on the original UNiagaraDataInterfaceAudioOscilloscope
 */

struct FNiagaraDataInterfaceProxyDynamicCurve final : public FNiagaraDataInterfaceProxy {
    FNiagaraDataInterfaceProxyDynamicCurve();

    ~FNiagaraDataInterfaceProxyDynamicCurve();

    void OnBeginDestroy();

    float SampleAudio(float NormalizedPositionInBuffer);

    /**
    * @return the number of channels in the buffer.
    */
    int32 GetNumChannels();

    // Called when the Submix property changes.
    void OnUpdateFloatCurve(UCurveFloat* Curve);

    // This function enqueues a render thread command to decimate the pop audio off of the SubmixListener, downsample it, and post it to the GPUAudioBuffer.
    void PostAudioToGPU();
    FReadBuffer& ComputeAndPostSRV();

    // This function pops audio and downsamples it to our specified resolution. Returns the number of frames of audio in the downsampled buffer.
    int32 DownsampleAudioToBuffer();

    virtual int32 PerInstanceDataPassedToRenderThreadSize() const override
    {
        return 0;
    }

private:
    UCurveFloat* CurveFloatRegisteredTo;

    // The buffer we downsample PoppedBuffer to based on the Resolution property.
    Audio::AlignedFloatBuffer DownsampledBuffer;

    // Handle for the SRV used by the generated HLSL.
    FReadBuffer GPUDownsampledBuffer;
    FThreadSafeCounter NumChannelsInDownsampledBuffer;

    // Buffer read by VectorVM worker threads. This vector is guaranteed to not be mutated during the VectorVM tasks.
    Audio::AlignedFloatBuffer VectorVMReadBuffer;

    FCriticalSection DownsampleBufferLock;
};

/** Data Interface allowing curve data access. */
UCLASS(EditInlineNew, Category = "Curve", meta = (DisplayName = "Data Curve"))
class WINDOWSAUDIOCAPTURE_API UNiagaraDataInterfaceDynamicCurve final : public UNiagaraDataInterface {
    GENERATED_UCLASS_BODY()
public:
    DECLARE_NIAGARA_DI_PARAMETER();

    UPROPERTY(EditAnywhere, Category = "Curve")
    UCurveFloat* FloatCurve = nullptr; // ptr to a curve

    static const int32 MaxBufferResolution = 255;

    //VM function overrides:
    void SampleAudio(FVectorVMContext& Context);
    void GetNumChannels(FVectorVMContext& Context);

    virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
    virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData,
        FVMExternalFunction& OutFunc) override;

    virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override
    {
        return true;
    }

    virtual bool RequiresDistanceFieldData() const override { return false; }

    virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
        const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex,
        FString& OutHLSL) override;
    virtual void
    GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;

    virtual bool Equals(const UNiagaraDataInterface* Other) const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

    virtual void PostInitProperties() override;
    virtual void BeginDestroy() override;
    virtual void PostLoad() override;

protected:
    virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
};
