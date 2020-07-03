// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <Curves/RichCurve.h>

#include "WindowsAudioCaptureActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWinAudioCaptureEvent, const TArray<float>&, AudioCaptureData);
DECLARE_MULTICAST_DELEGATE_OneParam(FWinAudioCaptureNativeEvent, const TArray<float>&);

///<summary>
// WindowsAudioCaptureComponent is great but it can't be used by multiple clients, because once a
// client read the audio data, another client can't have access to the same data.
// AWindowsAudioCaptureActor provides a way to capture audio and to broadcast (multi-cast) audio data
// to the clients. Mandatory for a VFX usage.
// Multi-cast delegate support Native and BP binding.
///</summary>
UCLASS()
class WINDOWSAUDIOCAPTURE_API AWindowsAudioCaptureActor : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AWindowsAudioCaptureActor();
    ~AWindowsAudioCaptureActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WindowsAudioCapture | Default Values")
    float defaultFreqLogBase = 10.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WindowsAudioCapture | Default Values")
    float defaultFreqMultiplier = 0.25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WindowsAudioCapture | Default Values")
    float defaultFreqPower = 6.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WindowsAudioCapture | Default Values")
    float defaultFreqOffset = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WindowsAudioCapture | Timer Values")
    float defaultTimerTime = 0.01;

    UPROPERTY(BlueprintAssignable, Category = "WindowsAudioCapture | Event")
    FWinAudioCaptureEvent OnAudioCaptureEvent;

    UPROPERTY(EditAnywhere, Category = "WindowsAudioCapture | Curve")
    UCurveFloat* curveAudioData = nullptr;

    FWinAudioCaptureNativeEvent OnAudioCaptureNativeEvent;

protected:
    /**
	* This function will return an Array of Frequencies. Frequency = (LogX(FreqLogBase, Frequency) * FreqMultiplier)^FreqPower + FreqOffset.  e.g.: Frequency = (LogX(10, Frequency) * 0.25)^6 + 0.0
	*
	* @param	inFreqLogBase			Log Base of the Result Frequency.	Default: 10
	* @param	inFreqMultiplier		Multiplier of the Result Frequency.	Default: 0.25
	* @param	inFreqPower				Power of the Result Frequency.		Default: 6
	* @param	inFreqOffset			Offset of the Result Frequency.		Default: 0.0
	*
	*/
    // 	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Frequency Array", Keywords = "Get Frequency Array"), Category = "WindowsAudioCapture | Frequency Array")
    static TArray<float> GetFrequencyArray(
        float inFreqLogBase = 10.0,
        float inFreqMultiplier = 0.25,
        float inFreqPower = 6.0,
        float inFreqOffset = 0.0);

    /**
	* This function will return the value of a specific frequency. It's needs a Frequency Array from the "Get Frequency Array" function.
	*
	* @param	InFrequencies		Array of float values for different frequencies from 0 to 22000. Can be get by using the "Get Frequency Array" function.
	* @param	InWantedFrequency	The specific frequency you want to keep from the Frequency Array.
	* @param	OutFrequencyValue	Float value of the requested frequency.
	*
	*/
    // 	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Specific Freq Value", Keywords = "Get Specific Freq Value"), Category = "WindowsAudioCapture | Frequency Values")
    static void GetSpecificFrequencyValue(TArray<float> InFrequencies, int32 InWantedFrequency, float& OutFrequencyValue);

    /**
	* This function will return the average value for SubBass (20 to 60hz)
	*
	* @param	InFrequencies	Array of float values for different frequencies from 0 to 22000. Can be get by using the "Get Frequency Array" function.
	* @param	OutAverageSubBass Average value of the frequencies from 20 to 60.
	*
	*/
    // 	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Average Subbass Value", Keywords = "Get Average Subbass Value"), Category = "WindowsAudioCapture | Frequency Values")
    static void GetAverageSubBassValue(TArray<float> InFrequencies, float& OutAverageSubBass);

    /**
	* This function will return the average value for Bass (60 to 250hz)
	*
	* @param	InFrequencies	Array of float values for different frequencies from 0 to 22000. Can be get by using the "Get Frequency Array" function.
	* @param	OutAverageBass	Average value of the frequencies from 60 to 250.
	*
	*/
    // 	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Average Bass Value", Keywords = "Get Average Bass Value"), Category = "WindowsAudioCapture | Frequency Values")
    static void GetAverageBassValue(TArray<float> InFrequencies, float& OutAverageBass);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UFUNCTION()
    void onCaptureData();

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

private:
    FTimerHandle CaptureDataTimerHandler;
};
