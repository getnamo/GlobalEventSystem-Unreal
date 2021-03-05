#pragma once

#include "CoreMinimal.h"
#include "GESDataTypes.generated.h"

USTRUCT(BlueprintType)
struct FGESGlobalOptions
{
	GENERATED_USTRUCT_BODY()

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

/** Convenience struct used to define a bind to a GES event*/
USTRUCT(BlueprintType)
struct FGESLocalBind
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Local Bind")
	FString Domain;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Local Bind")
	FString Event;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GES Local Bind")
	FString ReceivingFunction;

	FGESLocalBind();
};

/** 
*	Wrapper for lambda bind call data minus actual receiver function.
*	Used in AddLambdaListener and remove variant.
*/
USTRUCT()
struct FGESLambdaBind
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString Domain;

	UPROPERTY()
		FString Event;

	UPROPERTY()
		UObject* WorldContext;
};

USTRUCT(BlueprintType)
struct FGESWildcardProperty
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GES Global Options")
	TFieldPath<FProperty> Property;

	void* PropertyPtr;

	//Not used, expects ptr to be pointing to a valid prop
	//UPROPERTY()
	//TArray<uint8> PropertyMemory;
};

DECLARE_DYNAMIC_DELEGATE(FGESEmptySignature);
DECLARE_DYNAMIC_DELEGATE_OneParam(FGESOnePropertySignature, const FGESWildcardProperty&, WildcardProperty);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGESOnePropertyMCSignature, const FGESWildcardProperty&, WildcardProperty);

struct FGESEventListener
{
	UObject* Receiver;	////Used for world context
	FString FunctionName;

	/** Bound UFunction, valid after calling LinkFunction*/
	UFunction* Function;

	//Optionally we could be using an event delegate
	bool bIsBoundToDelegate;
	FGESOnePropertySignature OnePropertyFunctionDelegate;

	bool bIsBoundToLambda;
	TFunction<void(const FGESWildcardProperty&)> LambdaFunction;

	FGESEventListener()
	{
		bIsBoundToDelegate = false;
		bIsBoundToLambda = false;
		FunctionName = TEXT("");
		Function = nullptr;
		Receiver = nullptr;
		LambdaFunction = nullptr;
	}

	bool LinkFunction()
	{
		Function = Receiver->FindFunction(FName(*FunctionName));
		return IsValidListener();
	}

	bool IsValidListener() const
	{
		return (Function != nullptr || bIsBoundToDelegate || bIsBoundToLambda);
	}

	bool operator ==(FGESEventListener const &Other) {
		return (Other.Receiver == Receiver) && (Other.FunctionName == FunctionName);
	}
};

struct FGESPinnedData
{
	FProperty* Property;
	void* PropertyPtr;
	TArray<uint8> PropertyData;

	FGESPinnedData()
	{
		Property = nullptr;
		PropertyPtr = nullptr;
	}
	void CopyPropertyToPinnedBuffer();
	void CleanupPinnedData();
};



struct FGESEvent
{
	//If pinned an event will emit the moment you add a listener if it has been already fired once
	bool bPinned;
	FGESPinnedData PinnedData;

	FString Domain;
	FString Event;
	TArray<FGESEventListener> Listeners;
	UObject* WorldContext;

	FGESEvent()
	{
		PinnedData = FGESPinnedData();
		WorldContext = nullptr;
	}

	//todo: add restrictions e.g. must apply interface, 
	//	should this be a callback to creator of function/domain?
	//	for refined access?
};

struct FGESEmitData
{
	bool bPinned;
	FString Domain;
	FString Event;
	FProperty* Property;
	void* PropertyPtr;
	UObject* WorldContext;

	//if we want a callback or pin emit
	FGESEventListener* SpecificTarget;

	FGESEmitData()
	{
		Property = nullptr;
		PropertyPtr = nullptr;
		SpecificTarget = nullptr;
		WorldContext = nullptr;
	}
};

struct FGESDynamicArg
{
	void* Arg01;
};

