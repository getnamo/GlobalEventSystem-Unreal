#pragma once

#include "GESDataTypes.h"

/** Struct to hold pinned property data */
struct FGESPinnedData
{
	FProperty* Property;
	void* PropertyPtr;
	TArray<uint8> PropertyData;
	bool bHandlePropertyDeletion;

	FGESPinnedData()
	{
		Property = nullptr;
		PropertyPtr = nullptr;
		bHandlePropertyDeletion = false;
	}
	~FGESPinnedData()
	{
		CleanupPinnedData();
	}
	void CopyPropertyToPinnedBuffer();
	void CleanupPinnedData();
};

struct FGESDynamicArg
{
	void* Arg01;
};

//Minimal definition to define a listener (for removal)
struct FGESMinimalEventListener
{
	UObject* ReceiverWCO;	//WorldContextObject
	FString FunctionName;

	bool operator ==(FGESMinimalEventListener const& Other)
	{
		return (Other.ReceiverWCO == ReceiverWCO) && (Other.FunctionName == FunctionName);
	}
	FGESMinimalEventListener();
};

struct FGESEventListener : FGESMinimalEventListener
{
	// Opt A) Bound UFunction, valid after calling LinkFunction
	UFunction* Function;

	// Opt B) Bound to a delegate
	bool bIsBoundToDelegate;
	FGESOnePropertySignature OnePropertyFunctionDelegate;

	// Opt C) Bound to a lambda function
	bool bIsBoundToLambda;
	TFunction<void(const FGESWildcardProperty&)> LambdaFunction;

	FGESEventListener(const FGESMinimalEventListener& Minimal);
	FGESEventListener();
	bool LinkFunction();
	bool IsValidListener() const;
};

//Wrapper struct for tracking event-receiver pairs in ReceiverMap
struct FGESEventListenerWithContext
{
	FGESMinimalEventListener Listener;
	FString Domain;
	FString Event;

	FGESEventListenerWithContext()
	{
		Domain = TEXT("");
		Event = TEXT("");
	}

	bool operator ==(FGESEventListenerWithContext const& Other)
	{
		return (Other.Domain == Domain) && 
			(Other.Event == Event) &&
			(Listener.FunctionName == Other.Listener.FunctionName) &&
			(Listener.ReceiverWCO == Other.Listener.ReceiverWCO);
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
	bool bHandleAllocation;

	//NB: if we want a callback or pin emit
	FGESEventListener* SpecificTarget;

	FGESPropertyEmitContext();
	FGESPropertyEmitContext(const FGESEmitContext& Other);
};