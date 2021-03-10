// Copyright 2019-current Getnamo. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GESWorldListenerActor.generated.h"

/** 
* An actor spawned per world by FGESHandler in order to track when the world
* gets torn down and we have to remove all listeners for that world.
*/
UCLASS()
class GLOBALEVENTSYSTEM_API AGESWorldListenerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGESWorldListenerActor();

	// Event to listen to in order to catch world ending
	TFunction<void()> OnEndPlay;

	TSet<FString> WorldEvents;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
