#pragma once
#include "UObject/Object.h"
#include "UObject/UnrealType.h"

struct FGESEventListener
{
	UObject* Receiver;
	FString FunctionName;

	/** Bound UFunction, valid after calling LinkFunction*/
	UFunction* Function;

	FGESEventListener() {}

	bool LinkFunction()
	{
		Function = Receiver->FindFunction(FName(*FunctionName));
		return IsValidListener();
	}

	bool IsValidListener() const
	{
		return (Function != nullptr);
	}

	bool operator ==(FGESEventListener const &Other) {
		return (Other.Receiver == Receiver) && (Other.FunctionName == FunctionName);
	}
};

struct FGESPinnedData
{
	UProperty* Property;
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

	FGESEvent() 
	{ 
		PinnedData = FGESPinnedData();
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
	UProperty* Property;
	void* PropertyPtr;
	UObject* WorldContext;

	//if we want a callback or pin emit
	FGESEventListener* SpecificTarget;

	FGESEmitData()
	{
		Property = nullptr;
		PropertyPtr = nullptr;
		SpecificTarget = nullptr;
	}
};

struct FGESDynamicArg
{
	void* Arg01;
};

struct FGESHandlerOptions
{
	//can have performance implications, but safer
	bool bValidateStructTypes;

	//Generally logs when you re-launch a map or delete receiving actors without unbinding
	bool bLogStaleRemovals;

	FGESHandlerOptions()
	{
		bValidateStructTypes = true;
		bLogStaleRemovals = true;
	}
};

class FGESHandler
{
public:

	//Get the Global (default) handler to call all other functions
	static TSharedPtr<FGESHandler> DefaultHandler();

	/**
	*	Create an event in TargetDomain.TargetFunction. Does nothing if already existing
	*/
	void CreateEvent(const FString& Domain, const FString& Event, bool bPinned = false);
	
	/**
	*	Delete an event in TargetDomain.TargetFunction. Does nothing if missing
	*/
	void DeleteEvent(const FString& Domain, const FString& Event);

	/** 
	*	Check if event exists
	*/
	bool HasEvent(const FString& Domain, const FString& Event);

	/** 
	*	Removes the pinning of the event for future listeners.
	*/
	void UnpinEvent(const FString& Domain, const FString& Event);

	/** 
	* Listen to an event in TargetDomain.TargetFunction
	*/
	void AddListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener);
	
	/**
	* Stop listening to an event in TargetDomain.TargetFunction
	*/
	void RemoveListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener);

	/**
	* Emit event in TargetDomain.TargetFunction with Struct type parameter data.
	*/
	void EmitToListenersWithData(const FGESEmitData& EmitData, TFunction<void(const FGESEventListener&)> DataFillCallback);

	//overloaded emits
	void EmitEvent(const FGESEmitData& EmitData, UStruct* Struct, void* StructPtr);
	void EmitEvent(const FGESEmitData& EmitData, const FString& ParamData);
	void EmitEvent(const FGESEmitData& EmitData, UObject* ParamData);
	void EmitEvent(const FGESEmitData& EmitData, float ParamData);
	void EmitEvent(const FGESEmitData& EmitData, int32 ParamData);
	void EmitEvent(const FGESEmitData& EmitData, bool ParamData);
	void EmitEvent(const FGESEmitData& EmitData, const FName& ParamData);
	bool EmitEvent(const FGESEmitData& EmitData);

	/**
	* Update global options
	*/
	void SetOptions(const FGESHandlerOptions& InOptions);
	
	/** 
	* Convenience internal Key for domain and event string
	*/
	static FString Key(const FString& Domain, const FString& Event);

	FGESHandler();
	~FGESHandler();

private:
	static TSharedPtr<FGESHandler> PrivateDefaultHandler;

	//can check function signature vs e.g. FString
	static bool FirstParamIsCppType(UFunction* Function, const FString& TypeString);
	static bool FirstParamIsSubclassOf(UFunction* Function, UClass* ClassType);
	static FString ListenerLogString(const FGESEventListener& Listener);
	static FString EventLogString(const FGESEvent& Event);
	static FString EmitEventLogString(const FGESEmitData& EmitData);
	static void FunctionParameters(UFunction* Function, TArray<UProperty*>& OutParamProperties);

	//this function logs warnings otherwise
	static bool FunctionHasValidParams(UFunction* Function, UClass* ClassType, const FGESEmitData& EmitData, const FGESEventListener& Listener);

	//Key == TargetDomain.TargetFunction
	TMap<FString, FGESEvent> FunctionMap;
	TArray<FGESEventListener*> RemovalArray;

	//Toggles
	FGESHandlerOptions Options;
};