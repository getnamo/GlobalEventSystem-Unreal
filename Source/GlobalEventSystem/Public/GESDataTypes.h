#pragma once

#include "CoreMinimal.h"
#include "GESDataTypes.generated.h"

/** 
* Global options for GESHandler. Used in BP library static calls.
*/
USTRUCT(BlueprintType)
struct FGESGlobalOptions
{
	GENERATED_BODY()

	/** Whether to ensure structs are exactly the same. Turn off for small performance boost. Default true.*/
	UPROPERTY(BlueprintReadWrite, Category = "GES Global Options")
	bool bValidateStructTypes;

	/** Will output logs for stale events and listeners that get removed. . Default true.*/
	UPROPERTY(BlueprintReadWrite, Category = "GES Global Options")
	bool bLogStaleRemovals;

	FGESGlobalOptions()
	{
		bValidateStructTypes = true;
		bLogStaleRemovals = true;
	}
};

/** Struct used to define a bind to a GES event by function name. (Used in GESBaseReceiverComponents) */
USTRUCT(BlueprintType)
struct FGESNameBind
{
	GENERATED_BODY()

	/** Abstract Domain name used in GES, similar to a channel concept. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Local Bind")
	FString Domain;
	
	/** Abstract event name used in GES. Unique when combined with Domain. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Local Bind")
	FString Event;

	/** Name of function receiving event. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Local Bind")
	FString ReceivingFunction;

	FGESNameBind()
	{
		Domain = TEXT("global.default");
		Event = TEXT("");
		ReceivingFunction = TEXT("");
	}
};

/** 
*	Wrapper for lambda bind call data minus actual receiver function.
*	Used in AddLambdaListener and remove variant.
*/
USTRUCT()
struct FGESEventContext
{
	GENERATED_BODY()

	/** Abstract Domain name used in GES, similar to a channel concept. */
	UPROPERTY()
	FString Domain;

	/** Abstract event name used in GES. Unique when combined with Domain. */
	UPROPERTY()
	FString Event;

	/** World context object. Used to determine max lifetime of event. */
	UPROPERTY()
	UObject* WorldContext;

	FGESEventContext()
	{
		Domain = TEXT("global.default");
		Event = TEXT("");
		WorldContext = nullptr;
	}
};

USTRUCT()
struct FGESEmitContext : public FGESEventContext
{
	GENERATED_BODY()

	/** Pinned means an emitted event state should be accessible after it has been emitted. */
	UPROPERTY()
	bool bPinned;

	FGESEmitContext()
	{
		Domain = TEXT("global.default");
		Event = TEXT("");
		WorldContext = nullptr;
		bPinned = false;
	}
};


/** 
* Wrapper struct for a wildcard property. Allows directly binding GES events to
* delegate events with GESBPLibrary conversion casting used to specialize the property.
*/
USTRUCT(BlueprintType)
struct FGESWildcardProperty
{
	GENERATED_BODY()

	/** Property wrapper. Use GESBPLibrary conversion functions to obtained specialized conversions.*/
	UPROPERTY(BlueprintReadOnly, Category = "GES Global Options")
	TFieldPath<FProperty> Property;

	void* PropertyPtr;
};

/** No param Delegate */
DECLARE_DYNAMIC_DELEGATE(FGESEmptySignature);

/** Wildcard Delegate */
DECLARE_DYNAMIC_DELEGATE_OneParam(FGESOnePropertySignature, const FGESWildcardProperty&, WildcardProperty);

/** Multicast variant of Wildcard Delegate (used in BaseReceiverComponents) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGESOnePropertyMCSignature, const FGESWildcardProperty&, WildcardProperty);