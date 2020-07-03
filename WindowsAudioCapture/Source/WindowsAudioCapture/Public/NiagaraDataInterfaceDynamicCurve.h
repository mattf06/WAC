// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "NiagaraShared.h"
#include "StaticMeshResources.h"
#include "VectorVM.h"
#include "NiagaraCommon.h"
#include "NiagaraShared.h"
#include "NiagaraDataInterface.h"
#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "DSP/MultithreadedPatching.h"
#include "NiagaraDataInterfaceAudio.h"

#include "NiagaraDataInterfaceDynamicCurve.generated.h"

/**
 * 
 */


struct FNiagaraDataInterfaceProxyDynamicCurve : public FNiagaraDataInterfaceProxy
{
	FNiagaraDataInterfaceProxyDynamicCurve();

	~FNiagaraDataInterfaceProxyDynamicCurve();

	void OnBeginDestroy();

	/**
	 * Sample vertical displacement from the oscilloscope buffer.
	 * @param NormalizedPositionInBuffer Horizontal position in the Oscilloscope buffer, from 0.0 to 1.0.
	 * @param Channel channel index.
	 * @return Amplitude at this position.
	 */
	float SampleAudio(float NormalizedPositionInBuffer);
        /**
	 * @return the number of channels in the buffer.
	 */
	int32 GetNumChannels();

	// Called when the Submix property changes.
	void OnUpdateFloatCurve(UCurveFloat* Curve);

	// Called when Resolution or Zoom are changed.
	void OnUpdateResampling(int32 InResolution, float InScopeInMilliseconds);

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

	int32 Resolution;
	float ScopeInMilliseconds;

	// The buffer we downsample PoppedBuffer to based on the Resolution property.
	Audio::AlignedFloatBuffer PopBuffer;
	Audio::AlignedFloatBuffer DownsampledBuffer;

	// Handle for the SRV used by the generated HLSL.
	FReadBuffer GPUDownsampledBuffer;
	FThreadSafeCounter NumChannelsInDownsampledBuffer;
	
	// Buffer read by VectorVM worker threads. This vector is guaranteed to not be mutated during the VectorVM tasks.
	Audio::AlignedFloatBuffer VectorVMReadBuffer;

	FCriticalSection DownsampleBufferLock;
};

/** Data Interface allowing sampling of recent audio data. */
UCLASS(EditInlineNew, Category = "Curve", meta = (DisplayName = "Audio Curve"))
class WINDOWSAUDIOCAPTURE_API UNiagaraDataInterfaceDynamicCurve : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()
public:

	DECLARE_NIAGARA_DI_PARAMETER();
	
	UPROPERTY(EditAnywhere, Category = "Curve")
	UCurveFloat * FloatCurve = nullptr;

	static const int32 MaxBufferResolution = 255;

	//VM function overrides:
	void SampleAudio(FVectorVMContext& Context);
	void GetNumChannels(FVectorVMContext& Context);

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc) override;

	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override
	{
		return true;
	}

	virtual bool RequiresDistanceFieldData() const override { return false; }

	virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;
	virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;

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

//*************************************************************************
#if 0
UCLASS(EditInlineNew, Category = "Curves", meta = (DisplayName = "Dynamic Curve for Floats"))
class WINDOWSAUDIOCAPTURE_API UNiagaraDataInterfaceDynamicCurve : public UNiagaraDataInterfaceCurveBase {
    GENERATED_UCLASS_BODY()

public:
	DECLARE_NIAGARA_DI_PARAMETER();

    UPROPERTY(EditAnywhere, Category = "Curve")
    UCurveFloat* FloatCurve = nullptr;

	enum
	{
		CurveLUTNumElems = 1,
	};

    //UObject Interface
    virtual void PostInitProperties() override;
    virtual void Serialize(FArchive& Ar) override;
    //UObject Interface End

    virtual void UpdateTimeRanges() override;
    virtual TArray<float> BuildLUT(int32 NumEntries) const override;

    virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
//     virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;

    template <typename UseLUT>
    void SampleCurve(FVectorVMContext& Context);

    virtual bool Equals(const UNiagaraDataInterface* Other) const override;

    //~ UNiagaraDataInterfaceCurveBase interface
    virtual void GetCurveData(TArray<FCurveData>& OutCurveData) override;

    virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;

    virtual int32 GetCurveNumElems() const override;

protected:
    virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;

private:
    template <typename UseLUT>
    FORCEINLINE_DEBUGGABLE float SampleCurveInternal(float X);
    static const FName SampleCurveName;
};
#endif
