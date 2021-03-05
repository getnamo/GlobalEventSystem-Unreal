// Copyright 2019-current Getnamo. All Rights Reserved

#pragma once

#include "Components/ActorComponent.h"
#include "GESDataTypes.h"
#include "GESBaseReceiverComponent.generated.h"

/** Convenience base class for receiving GES events in an organized way for actors.*/
UCLASS(BlueprintType, ClassGroup = "Utility", meta = (BlueprintSpawnableComponent))
class GLOBALEVENTSYSTEM_API UGESBaseReceiverComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:
	//Wildcard receiver
	//UPROPERTY(BlueprintReadWrite, Category = "GES Receiver")
	FGESOnePropertySignature OnEvent;

	//Domain, Event, and receiving function name
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	FGESLocalBind BindSettings;

	//auto-bind as soon as this component begins play
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	bool bBindOnBeginPlay;

	//Unbind the event automatically whenever gameplay ends for this component (e.g. destroyed)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	bool bUnbindOnEndPlay;

	//Unused
	/** Placeholder to output warning if you forget to specialize bind settings */
	//UFUNCTION(BlueprintCallable, Category = "GESReceiver Functions")
	//void OnEventToPlaceHolderFunction(const FString& PlaceHolderParam);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
};