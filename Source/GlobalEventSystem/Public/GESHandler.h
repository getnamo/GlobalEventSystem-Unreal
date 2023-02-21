#pragma once
#include "UObject/Object.h"
#include "UObject/UnrealType.h"
#include "GESWorldListenerActor.h"
#include "GESDataTypes.h"
#include "GESHandlerDataTypes.h"

//Text macro to handle TEXT("") emits
#if !defined(GES_RAW_TEXT)
	#if PLATFORM_TCHAR_IS_CHAR16
		#define GES_RAW_TEXT char16_t*
	#else
		#define GES_RAW_TEXT wchar_t*
	#endif
#endif

/** 
GESHandler Class usable in C++ with care. Private API may be a bit too exposed atm.
*/

class GLOBALEVENTSYSTEM_API FGESHandler
{
public:

	//Get the Global (default) handler to call all other functions
	static TSharedPtr<FGESHandler> DefaultHandler();

	/**
	*   Clear all listeners and reset state
	*/
	static void Clear();

	/**
	*	Create an event in TargetDomain.TargetFunction. Does nothing if already existing.
	*/
	void CreateEvent(const FString& Domain, const FString& Event, bool bPinned = false);
	
	/**
	*	Delete an event in TargetDomain.TargetFunction. Does nothing if missing.
	*/
	void DeleteEvent(const FString& Domain, const FString& Event);

	/** 
	*	Delete an event in with DomainAndEvent defined as a single string. Does nothing if missing.
	*/
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
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda);
	
	/**
	* Stop listening to an event in TargetDomain.TargetFunction
	*/
	void RemoveListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener);

	/**
	* Stop listening to all events for given receiver
	*/
	void RemoveAllListenersForReceiver(UObject* ReceiverWCO);

	/**
	*	Listen to an event in TargetDomain.TargetFunction via passed in lambda
	*/
	void RemoveLambdaListener(FGESEventContext EventInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda);

	/** 
	*	Remove lambda by function id string. Used in case of lambdas without ref to initial function.
	*/
	void RemoveLambdaListener(FGESEventContext EventInfo, const FString& LambdaName);

	//overloaded emits
	void EmitEvent(const FGESEmitContext& EmitData, UStruct* Struct, void* StructPtr);
	void EmitEvent(const FGESEmitContext& EmitData, const FString& ParamData);
	void EmitEvent(const FGESEmitContext& EmitData, UObject* ParamData);
	void EmitEvent(const FGESEmitContext& EmitData, float ParamData);
	void EmitEvent(const FGESEmitContext& EmitData, int32 ParamData);
	void EmitEvent(const FGESEmitContext& EmitData, bool ParamData);
	void EmitEvent(const FGESEmitContext& EmitData, const FName& ParamData);
	bool EmitEvent(const FGESEmitContext& EmitData);
	//GES_RAW_TEXT supports passing in TEXT("") macros
	void EmitEvent(const FGESEmitContext& EmitData, const GES_RAW_TEXT RawStringMessage);

	//processed means the pointers have been filled
	bool EmitPropertyEvent(const FGESPropertyEmitContext& FullEmitData);

	//overloaded lambda binds
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(UStruct* Struct, void* StructPtr)> ReceivingLambda);
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(const FString&)> ReceivingLambda);
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(UObject*)> ReceivingLambda);
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(float)> ReceivingLambda);
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(const FName&)> ReceivingLambda);
	FString AddLambdaListener(FGESEventContext EventInfo, TFunction<void(void)> ReceivingLambda);

	//needed unique names due to ambiguity clash with float
	FString AddLambdaListenerInt(FGESEventContext EventInfo, TFunction<void(int32)> ReceivingLambda);
	FString AddLambdaListenerBool(FGESEventContext EventInfo, TFunction<void(bool)> ReceivingLambda);

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

	//internal helper for in-context data filling for listeners
	void EmitToListenersWithData(const FGESPropertyEmitContext& EmitData, TFunction<void(const FGESEventListener&)> DataFillCallback);
	//internal emitter to each listener
	bool EmitToListenerWithData(const FGESPropertyEmitContext& EmitData, const FGESEventListener& Listener,
		TFunction<void(const FGESEventListener&)>& DataFillCallback);

	//internal overloads
	void EmitSubPropertyEvent(const FGESPropertyEmitContext& EmitData);

	//can check function signature vs e.g. FString
	static bool FirstParamIsCppType(UFunction* Function, const FString& TypeString);
	static bool FirstParamIsSubclassOf(UFunction* Function, FFieldClass* ClassType);
	static FString ListenerLogString(const FGESEventListener& Listener);
	static FString EventLogString(const FGESEvent& Event);
	static FString EmitEventLogString(const FGESEmitContext& EmitData);
	static void FunctionParameters(UFunction* Function, TArray<FProperty*>& OutParamProperties);

	//this function logs warnings otherwise
	static bool FunctionHasValidParams(UFunction* Function, FFieldClass* ClassType, const FGESEmitContext& EmitData, const FGESEventListener& Listener);

	//Key == TargetDomain.TargetFunction
	TMap<FString, FGESEvent> EventMap;
	TMap<UObject*, TArray<FGESEventListenerWithContext>> ReceiverMap;
	TArray<FGESEventListener*> RemovalArray;

	//Toggles
	FGESGlobalOptions Options;

	TMap<UWorld*, AGESWorldListenerActor*> WorldMap;
};