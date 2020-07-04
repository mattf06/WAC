// Fill out your copyright notice in the Description page of Project Settings.

#include "NiagaraDataInterfaceDynamicCurve.h"
#include "../Plugins/FX/Niagara/Source/Niagara/Public/NiagaraCommon.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraShader.h"
#include "NiagaraTypes.h"

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceAudioCurve"

// Global VM function names, also used by the shaders code generation methods.
static const FName SampleAudioBufferFunctionName("SampleAudioBuffer");

// Global variable prefixes, used in HLSL parameter declarations.
static const FString AudioBufferName(TEXT("AudioBuffer_"));

FNiagaraDataInterfaceProxyDynamicCurve::FNiagaraDataInterfaceProxyDynamicCurve()
    : CurveFloatRegisteredTo(nullptr)
    , NumChannelsInDownsampledBuffer(0)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::FNiagaraDataInterfaceProxyDynamicCurve"));
    VectorVMReadBuffer.Reset();
    VectorVMReadBuffer.AddZeroed(
        UNiagaraDataInterfaceDynamicCurve::MaxBufferResolution /** AUDIO_MIXER_MAX_OUTPUT_CHANNELS*/);
}

FNiagaraDataInterfaceProxyDynamicCurve::~FNiagaraDataInterfaceProxyDynamicCurve()
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::~FNiagaraDataInterfaceProxyDynamicCurve"));
    check(IsInRenderingThread());
    GPUDownsampledBuffer.Release();
}

void FNiagaraDataInterfaceProxyDynamicCurve::OnUpdateFloatCurve(UCurveFloat* Curve)
{
    //UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::OnUpdateFloatCurve(%p)"), Curve);
    CurveFloatRegisteredTo = Curve;
    if (Curve != nullptr) {
        FScopeLock ScopeLock(&DownsampleBufferLock);
        VectorVMReadBuffer.Reset();
        VectorVMReadBuffer.AddZeroed(Curve->FloatCurve.GetNumKeys());
    }
}

UNiagaraDataInterfaceDynamicCurve::UNiagaraDataInterfaceDynamicCurve(FObjectInitializer const& ObjectInitializer)
    : Super(ObjectInitializer)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::UNiagaraDataInterfaceDynamicCurve"));
    Proxy = TUniquePtr<FNiagaraDataInterfaceProxyDynamicCurve>(new FNiagaraDataInterfaceProxyDynamicCurve());
}

float FNiagaraDataInterfaceProxyDynamicCurve::SampleAudio(float NormalizedPositionInBuffer)
{
    //int32 FrameIndex = FMath::CeilToInt(NormalizedPositionInBuffer) % VectorVMReadBuffer.Num();
    NormalizedPositionInBuffer = FMath::Clamp(NormalizedPositionInBuffer, 0.0f, 1.0f);
    int32 FrameIndex = FMath::CeilToInt(
        FMath::Lerp<float>(0.0, VectorVMReadBuffer.Num() - 1, NormalizedPositionInBuffer));
    float value = VectorVMReadBuffer[FrameIndex];
    // 	UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::SampleAudio: %f, %d, %f"), NormalizedPositionInBuffer, FrameIndex, value);
    return value;
}

void UNiagaraDataInterfaceDynamicCurve::SampleAudio(FVectorVMContext& Context)
{
    const int32 NumFramesInDownsampledBuffer = GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->DownsampleAudioToBuffer();

    VectorVM::FExternalFuncInputHandler<float> InNormalizedPos(Context);
    VectorVM::FExternalFuncRegisterHandler<float> OutAmplitude(Context);

    const int32 NumChannelsInDownsampledBuffer = GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->GetNumChannels();

    for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx) {
        float Position = InNormalizedPos.Get();
        //         UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::SampleAudio(FVectorVMContext) : position = %f"), Position);
        *OutAmplitude.GetDest() = GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->SampleAudio(Position);

        InNormalizedPos.Advance();
        OutAmplitude.Advance();
    }
}

int32 FNiagaraDataInterfaceProxyDynamicCurve::GetNumChannels()
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::GetNumChannels"));
    return NumChannelsInDownsampledBuffer.GetValue();
}

void UNiagaraDataInterfaceDynamicCurve::GetNumChannels(FVectorVMContext& Context)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::GetNumChannels"));
    VectorVM::FExternalFuncRegisterHandler<int32> OutChannel(Context);

    for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx) {
        *OutChannel.GetDestAndAdvance() = GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->GetNumChannels();
    }
}

void UNiagaraDataInterfaceDynamicCurve::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::GetFunctions"));
    Super::GetFunctions(OutFunctions);

    {
        FNiagaraFunctionSignature SampleAudioBufferSignature;
        SampleAudioBufferSignature.Name = SampleAudioBufferFunctionName;
        SampleAudioBufferSignature.Inputs.Add(FNiagaraVariable(GetClass(), TEXT("Curve")));
        SampleAudioBufferSignature.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),
            TEXT("NormalizedPositionInBuffer")));
        SampleAudioBufferSignature.Outputs.Add(
            FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Amplitude")));

        SampleAudioBufferSignature.bMemberFunction = true;
        SampleAudioBufferSignature.bRequiresContext = false;
        OutFunctions.Add(SampleAudioBufferSignature);
    }
}

DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceDynamicCurve, SampleAudio);

void UNiagaraDataInterfaceDynamicCurve::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo,
    void* InstanceData, FVMExternalFunction& OutFunc)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::GetVMExternalFunction"));
    if (BindingInfo.Name == SampleAudioBufferFunctionName) {
        NDI_FUNC_BINDER(UNiagaraDataInterfaceDynamicCurve, SampleAudio)::Bind(this, OutFunc);
    } else {
        ensureMsgf(false, TEXT("Error! Function defined for this class but not bound."));
    }
}

bool UNiagaraDataInterfaceDynamicCurve::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
    int FunctionInstanceIndex, FString& OutHLSL)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::GetFunctionHLSL"));
    bool ParentRet = Super::GetFunctionHLSL(ParamInfo, FunctionInfo, FunctionInstanceIndex, OutHLSL);
    if (ParentRet) {
        return true;
    } else if (FunctionInfo.DefinitionName == SampleAudioBufferFunctionName) {
        // See SampleBuffer(float InNormalizedPosition)
        static const TCHAR* FormatBounds = TEXT(
            R"(
			void {FunctionName}(float In_NormalizedPosition, out float Out_Val)
			{
				float FrameIndex = lerp(0.0, {AudioBufferNumSamples}, In_NormalizedPosition);
				int Index = floor(FrameIndex);
				float Value = {AudioBuffer}.Load(Index);
				Out_Val = Value;
			}
		)");
        TMap<FString, FStringFormatArg> ArgsBounds = {
            { TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName) },
            { TEXT("AudioBuffer"), FStringFormatArg(AudioBufferName + ParamInfo.DataInterfaceHLSLSymbol) },
            { TEXT("AudioBufferNumSamples"), FStringFormatArg(FloatCurve != nullptr ? FloatCurve->FloatCurve.GetNumKeys() : 0) },
        };
        OutHLSL += FString::Format(FormatBounds, ArgsBounds);
        return true;
    } else {
        return false;
    }
}

void UNiagaraDataInterfaceDynamicCurve::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    FString& OutHLSL)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::GetParameterDefinitionHLSL"));
    Super::GetParameterDefinitionHLSL(ParamInfo, OutHLSL);

    static const TCHAR* FormatDeclarations = TEXT(R"(				
		Buffer<float> {AudioBufferName};
	)");

    TMap<FString, FStringFormatArg> ArgsDeclarations = {
        { TEXT("AudioBufferName"), FStringFormatArg(AudioBufferName + ParamInfo.DataInterfaceHLSLSymbol) },
    };
    OutHLSL += FString::Format(FormatDeclarations, ArgsDeclarations);
}

struct FNiagaraDataInterfaceParametersCS_DynamicCurve : public FNiagaraDataInterfaceParametersCS {
    DECLARE_INLINE_TYPE_LAYOUT(FNiagaraDataInterfaceParametersCS_DynamicCurve, NonVirtual);

    void Bind(const FNiagaraDataInterfaceGPUParamInfo& ParameterInfo, const class FShaderParameterMap& ParameterMap)
    {
        //         UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceParametersCS_DynamicCurve::Bind"));
        AudioBuffer.Bind(ParameterMap, *(AudioBufferName + ParameterInfo.DataInterfaceHLSLSymbol));
    }

    void Set(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
    {
        //         UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceParametersCS_DynamicCurve::Set"));
        check(IsInRenderingThread());

        FRHIComputeShader* ComputeShaderRHI = Context.Shader.GetComputeShader();

        FNiagaraDataInterfaceProxyDynamicCurve* NDI = (FNiagaraDataInterfaceProxyDynamicCurve*)Context.DataInterface;
        FReadBuffer& AudioBufferSRV = NDI->ComputeAndPostSRV();

        //SetShaderValue(RHICmdList, ComputeShaderRHI, NumChannels, NDI->GetNumChannels());
        RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, AudioBuffer.GetBaseIndex(), AudioBufferSRV.SRV);
    }

    LAYOUT_FIELD(FShaderResourceParameter, AudioBuffer);
};

IMPLEMENT_NIAGARA_DI_PARAMETER(UNiagaraDataInterfaceDynamicCurve, FNiagaraDataInterfaceParametersCS_DynamicCurve);

FReadBuffer& FNiagaraDataInterfaceProxyDynamicCurve::ComputeAndPostSRV()
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::ComputeAndPostSRV"));
    // Copy to GPUDownsampledBuffer:
    PostAudioToGPU();
    return GPUDownsampledBuffer;
}

#if WITH_EDITOR
void UNiagaraDataInterfaceDynamicCurve::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::PostEditChangeProperty"));
    Super::PostEditChangeProperty(PropertyChangedEvent);

    static FName FloatCurveFName = GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceDynamicCurve, FloatCurve);

    // Regenerate on save any compressed sound formats or if analysis needs to be re-done
    if (FProperty* PropertyThatChanged = PropertyChangedEvent.Property) {
        const FName& Name = PropertyThatChanged->GetFName();
        if (Name == FloatCurveFName) {
            GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->OnUpdateFloatCurve(FloatCurve);
        }
    }
}
#endif //WITH_EDITOR

void UNiagaraDataInterfaceDynamicCurve::PostInitProperties()
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::PostInitProperties"));
    Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject)) {
        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), /*bCanBeParameter*/ true, /*bCanBePayload*/
            false, /*bIsUserDefined*/ false);
    }

    GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->OnUpdateFloatCurve(FloatCurve);
}

void FNiagaraDataInterfaceProxyDynamicCurve::OnBeginDestroy()
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::OnBeginDestroy"));
}

void UNiagaraDataInterfaceDynamicCurve::BeginDestroy()
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::BeginDestroy"));
    GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->OnBeginDestroy();

    Super::BeginDestroy();
}

void UNiagaraDataInterfaceDynamicCurve::PostLoad()
{
    //UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::PostLoad"));
    Super::PostLoad();
    GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->OnUpdateFloatCurve(FloatCurve);
}

bool UNiagaraDataInterfaceDynamicCurve::Equals(const UNiagaraDataInterface* Other) const
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::Equals"));
    const UNiagaraDataInterfaceDynamicCurve* CastedOther = Cast<const UNiagaraDataInterfaceDynamicCurve>(Other);
    return Super::Equals(Other)
        && (CastedOther->FloatCurve == FloatCurve);
}

bool UNiagaraDataInterfaceDynamicCurve::CopyToInternal(UNiagaraDataInterface* Destination) const
{
    //     UE_LOG(WindowsAudioCaptureLog, Log, TEXT("UNiagaraDataInterfaceDynamicCurve::CopyToInternal"));
    Super::CopyToInternal(Destination);

    UNiagaraDataInterfaceDynamicCurve* CastedDestination = Cast<UNiagaraDataInterfaceDynamicCurve>(Destination);

    if (CastedDestination) {
        CastedDestination->FloatCurve = FloatCurve;
        CastedDestination->GetProxyAs<FNiagaraDataInterfaceProxyDynamicCurve>()->OnUpdateFloatCurve(FloatCurve);
    }

    return true;
}

void FNiagaraDataInterfaceProxyDynamicCurve::PostAudioToGPU()
{
    //UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::PostAudioToGPU"));
    ENQUEUE_RENDER_COMMAND(FUpdateDIAudioBuffer)
    (
        [&](FRHICommandListImmediate& RHICmdList) {
            DownsampleAudioToBuffer();
            size_t BufferSize = DownsampledBuffer.Num() * sizeof(float);
            //             UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::ENQUEUE_RENDER_COMMAND; BufferSize=%d"), BufferSize);
            if (BufferSize != 0 && !GPUDownsampledBuffer.NumBytes) {
                GPUDownsampledBuffer.Initialize(sizeof(float), DownsampledBuffer.Num(),
                    EPixelFormat::PF_R32_FLOAT, BUF_Static);
            }

            if (GPUDownsampledBuffer.NumBytes > 0) {
                float* BufferData = static_cast<float*>(RHILockVertexBuffer(
                    GPUDownsampledBuffer.Buffer, 0, BufferSize, EResourceLockMode::RLM_WriteOnly));
                FScopeLock ScopeLock(&DownsampleBufferLock);
                FPlatformMemory::Memcpy(BufferData, DownsampledBuffer.GetData(), BufferSize);
                RHIUnlockVertexBuffer(GPUDownsampledBuffer.Buffer);
            }
        });
}

int32 FNiagaraDataInterfaceProxyDynamicCurve::DownsampleAudioToBuffer()
{
    if (CurveFloatRegisteredTo != nullptr) {
        FScopeLock ScopeLock(&DownsampleBufferLock);

        check(CurveFloatRegisteredTo->FloatCurve.GetNumKeys() <= VectorVMReadBuffer.Num());

        Audio::AlignedFloatBuffer data;
        data.Reserve(CurveFloatRegisteredTo->FloatCurve.GetNumKeys());
        int32 index = 0;
        FRichCurve* dup = static_cast<FRichCurve*>(CurveFloatRegisteredTo->FloatCurve.Duplicate());

        for (FRichCurveKey key : dup->GetConstRefOfKeys()) {
            data.Add(FMath::Clamp<float>(key.Value, 0.0f, 1.0f));
        }

        delete dup;

        FMemory::Memcpy(VectorVMReadBuffer.GetData(), data.GetData(), data.Num() * sizeof(float));

        DownsampledBuffer = VectorVMReadBuffer;

        //         UE_LOG(WindowsAudioCaptureLog, Log, TEXT("FNiagaraDataInterfaceProxyDynamicCurve::DownsampleAudioToBuffer (%d, %d, %d)"),
        //             DownsampledBuffer.Num(), data.Num(), VectorVMReadBuffer.Num());
        return data.Num();
    }
    return DownsampledBuffer.Num();
}

#undef LOCTEXT_NAMESPACE
