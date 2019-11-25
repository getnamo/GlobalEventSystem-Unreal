// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GESWorldListenerActor.generated.h"

UCLASS()
class GLOBALEVENTSYSTEM_API AGESWorldListenerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGESWorldListenerActor();

	//use to track when the world gets torn down
	TFunction<void()> OnEndPlay;

	TSet<FString> WorldEvents;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
