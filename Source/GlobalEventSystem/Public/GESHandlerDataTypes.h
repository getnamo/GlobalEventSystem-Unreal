#pragma once

#include "GESDataTypes.h"

/** Struct to hold pinned property data */
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

struct FGESDynamicArg
{
	void* Arg01;
};

struct FGESEmitData
{
	FString Domain;
	FString Event;
	UObject* WorldContext;
	bool bPinned;

	FGESEmitData()
	{
		Domain = TEXT("global.default");
		Event = TEXT("");
		WorldContext = nullptr;
		bPinned = false;
	}
};

struct FGESEventListener
{
	UObject* Receiver;	//Used for world context
	FString FunctionName;

	/** Bound UFunction, valid after calling LinkFunction*/
	UFunction* Function;

	//Optionally we could be using an event delegate
	bool bIsBoundToDelegate;
	FGESOnePropertySignature OnePropertyFunctionDelegate;

	bool bIsBoundToLambda;
	TFunction<void(const FGESWildcardProperty&)> LambdaFunction;


	FGESEventListener();
	bool LinkFunction();
	bool IsValidListener() const;

	bool operator ==(FGESEventListener const& Other)
	{
		return (Other.Receiver == Receiver) && (Other.FunctionName == FunctionName);
	}
};

struct FGESEvent : FGESEmitData
{
	//If pinned an event will emit the moment you add a listener if it has been already fired once
	FGESPinnedData PinnedData;

	TArray<FGESEventListener> Listeners;

	FGESEvent();
	FGESEvent(const FGESEmitData& Other);
};

//With non-public derived data
struct FGESFullEmitData : FGESEmitData
{
	FProperty* Property;
	void* PropertyPtr;

	//NB: if we want a callback or pin emit
	FGESEventListener* SpecificTarget;

	FGESFullEmitData();
	FGESFullEmitData(const FGESEmitData& Other);
};