// Fill out your copyright notice in the Description page of Project Settings.


#include "WindowsAudioCaptureActor.h"
#include "WindowsAudioCaptureComponent.h"

// Sets default values
AWindowsAudioCaptureActor::AWindowsAudioCaptureActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	UBillboardComponent* billboardComp = UObject::CreateDefaultSubobject<UBillboardComponent>(TEXT("Root Comp"));
	check(billboardComp);

	static ConstructorHelpers::FObjectFinder<UTexture2D> billboardSprite(TEXT("/Engine/EditorResources/AudioIcons/S_AudioComponent.S_AudioComponent"));

	billboardComp->SetSprite(billboardSprite.Object);

	SetRootComponent(billboardComp);

}

AWindowsAudioCaptureActor::~AWindowsAudioCaptureActor()
{
	if (!FAudioCaptureWorker::Runnable == NULL)
	{
		FAudioCaptureWorker::Runnable->ShutdownWorker();
	}
}

// Called when the game starts or when spawned
void AWindowsAudioCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	if (FAudioCaptureWorker::Runnable == NULL)
	{
		// Init new Worker 
		FAudioCaptureWorker::Runnable->InitializeWorker();
		GetWorld()->GetTimerManager().SetTimer(CaptureDataTimerHandler, this, &AWindowsAudioCaptureActor::onCaptureData, defaultTimerTime, true);
	}

}

void AWindowsAudioCaptureActor::onCaptureData()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("AWindowsAudioCaptureActor::onCaptureData"));
	TArray<float> data = GetFrequencyArray(defaultFreqLogBase, defaultFreqMultiplier, defaultFreqPower, defaultFreqOffset);

	if (data.Num() > 0)
	{
		float outAvgBass = 0;
		GetAverageSubBassValue(data, outAvgBass);
		data.SetNum(data.Num() / 2);

		// broadcast data to BP client(s)
		OnAudioCaptureEvent.Broadcast(data);

		// broadcast data to native client(s)
		OnAudioCaptureNativeEvent.Broadcast(data);
	}
}

// Called every frame
void AWindowsAudioCaptureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// This function will return an Array of Frequencies.
TArray<float> AWindowsAudioCaptureActor::GetFrequencyArray(float inFreqLogBase, float inFreqMultiplier, float inFreqPower, float inFreqOffset)
{
	return UWindowsAudioCaptureComponent::BP_GetFrequencyArray(inFreqLogBase, inFreqMultiplier, inFreqPower, inFreqOffset);
}

// This function will return the value of a specific frequency.
void AWindowsAudioCaptureActor::GetSpecificFrequencyValue(TArray<float> InFrequencies, int32 InWantedFrequency, float& OutFrequencyValue)
{
	UWindowsAudioCaptureComponent::BP_GetSpecificFrequencyValue(InFrequencies, InWantedFrequency, OutFrequencyValue);
}

// This function will return the average value for SubBass
void AWindowsAudioCaptureActor::GetAverageSubBassValue(TArray<float> InFrequencies, float& OutAverageSubBass)
{
	UWindowsAudioCaptureComponent::BP_GetAverageFrequencyValueInRange(InFrequencies, 20, 60, OutAverageSubBass);
}


// This function will return the average value for Bass (60 to 250hz)
void AWindowsAudioCaptureActor::GetAverageBassValue(TArray<float> InFrequencies, float& OutAverageBass)
{
	UWindowsAudioCaptureComponent::BP_GetAverageFrequencyValueInRange(InFrequencies, 60, 250, OutAverageBass);
}


