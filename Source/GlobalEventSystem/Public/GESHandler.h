#pragma once
#include "UObject/Object.h"
#include "UObject/UnrealType.h"
#include "GESWorldListenerActor.h"
#include "GESDataTypes.h"

/** 
GESHandler Class usable in C++ with care. Private API may be a bit too exposed atm.
*/

class GLOBALEVENTSYSTEM_API FGESHandler
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

	void DeleteEvent(const FString& DomainAndEvent);

	/** 
	*	Check if event exists
	*/
	bool HasEvent(const FString& Domain, const FString& Event);

	/** 
	*	Removes the pinning of the event for future listeners.
	*/
	void UnpinEvent(const FString& Domain, const FString& Event);

	/** 
	* Listen to an event in TargetDomain.TargetFunction via generic listener
	*/
	void AddListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener);

	/**
	*	Listen to an event in TargetDomain.TargetFunction via passed in lambda
	*/
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda);
	
	/**
	* Stop listening to an event in TargetDomain.TargetFunction
	*/
	void RemoveListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener);

	/**
	*	Listen to an event in TargetDomain.TargetFunction via passed in lambda
	*/
	void RemoveLambdaListener(FGESLambdaBind BindInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda);

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

	//overloaded lambda binds
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(UStruct* Struct, void* StructPtr)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(const FString&)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(UObject*)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(float)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(int32)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(bool)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(const FName&)> ReceivingLambda);
	void AddLambdaListener(FGESLambdaBind BindInfo, TFunction<void(void)> ReceivingLambda);

	/**
	* Update global options
	*/
	void SetOptions(const FGESGlobalOptions& InOptions);
	
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
	static bool FirstParamIsSubclassOf(UFunction* Function, FFieldClass* ClassType);
	static FString ListenerLogString(const FGESEventListener& Listener);
	static FString EventLogString(const FGESEvent& Event);
	static FString EmitEventLogString(const FGESEmitData& EmitData);
	static void FunctionParameters(UFunction* Function, TArray<FProperty*>& OutParamProperties);

	//this function logs warnings otherwise
	static bool FunctionHasValidParams(UFunction* Function, FFieldClass* ClassType, const FGESEmitData& EmitData, const FGESEventListener& Listener);

	//Key == TargetDomain.TargetFunction
	TMap<FString, FGESEvent> EventMap;
	TArray<FGESEventListener*> RemovalArray;

	//Toggles
	FGESGlobalOptions Options;

	TMap<UWorld*, AGESWorldListenerActor*> WorldMap;
};