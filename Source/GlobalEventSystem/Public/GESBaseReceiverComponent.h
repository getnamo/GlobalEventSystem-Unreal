// Copyright 2019-current Getnamo. All Rights Reserved

#pragma once

#include "Components/ActorComponent.h"
#include "GESDataTypes.h"
#include "GESHandlerDataTypes.h"
#include "GESBaseReceiverComponent.generated.h"

/** Convenience base class for receiving GES events in an organized way for actors.*/
UCLASS(BlueprintType, Blueprintable, ClassGroup = "Utility", meta = (BlueprintSpawnableComponent))
class GLOBALEVENTSYSTEM_API UGESBaseReceiverComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:
	//Wildcard receiver
	UPROPERTY(BlueprintAssignable, Category = "GES Receiver")
	FGESOnePropertyMCSignature OnEvent;

	//Domain, Event, and receiving function name
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	FGESNameBind BindSettings;

	//For polling after having received an event
	UPROPERTY(BlueprintReadOnly, Category = "GES Receiver")
	FGESWildcardProperty LastReceivedProperty;

	//auto-bind as soon as this component begins play
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	bool bBindOnBeginPlay;

	//Unbind the event automatically whenever gameplay ends for this component (e.g. destroyed)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	bool bUnbindOnEndPlay;

	/**
	* If event is the wildcard component one, this will pin data received for polling.
	* Turned off only for optimization generally.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Receiver")
	bool bPinInternalDataForPolling;

	/** Used to know if polling for last data will give valid results */
	UPROPERTY(BlueprintReadWrite, Category = "GES Receiver")
	bool bDidReceiveEventAtLeastOnce;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	//Only used to route signal to MC variant
	UPROPERTY()
	FGESOnePropertySignature InternalListener;

	FGESPinnedData PinnedData;

	UFUNCTION()
	void HandleInternalEvent(const FGESWildcardProperty& WildcardProperty);
};