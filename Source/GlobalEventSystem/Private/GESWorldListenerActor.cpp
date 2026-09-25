// Copyright 2019-current Getnamo. All Rights Reserved


#include "GESWorldListenerActor.h"

// Sets default values
AGESWorldListenerActor::AGESWorldListenerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	OnEndPlay = nullptr;
}

// Called when the game starts or when spawned
void AGESWorldListenerActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGESWorldListenerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//Unset when this actor wasn't spawned by a live handler, e.g. an older GES version saved one into a level
	if (OnEndPlay)
	{
		OnEndPlay();
	}
	Super::EndPlay(EndPlayReason);
}

