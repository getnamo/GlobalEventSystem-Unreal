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


struct FGESEventListener
{
	//ReceiverTarget.FunctionName
	UObject* ReceiverWCO;	//WorldContextObject
	FString FunctionName;

	// Opt A) Bound UFunction, valid after calling LinkFunction
	UFunction* Function;

	// Opt B) Bound to a delegate
	bool bIsBoundToDelegate;
	FGESOnePropertySignature OnePropertyFunctionDelegate;

	// Opt C) Bound to a lambda function
	bool bIsBoundToLambda;
	TFunction<void(const FGESWildcardProperty&)> LambdaFunction;

	FGESEventListener();
	bool LinkFunction();
	bool IsValidListener() const;

	bool operator ==(FGESEventListener const& Other)
	{
		return (Other.ReceiverWCO == ReceiverWCO) && (Other.FunctionName == FunctionName);
	}
};

//Event specialization with pinned and listener data
struct FGESEvent : FGESEmitContext
{
	//If pinned an event will emit the moment you add a listener if it has been already fired once
	FGESPinnedData PinnedData;

	TArray<FGESEventListener> Listeners;

	FGESEvent();
	FGESEvent(const FGESEmitContext& Other);
};

//Emit specialization with property pointers
struct FGESPropertyEmitContext : FGESEmitContext
{
	FProperty* Property;
	void* PropertyPtr;

	//NB: if we want a callback or pin emit
	FGESEventListener* SpecificTarget;

	FGESPropertyEmitContext();
	FGESPropertyEmitContext(const FGESEmitContext& Other);
};