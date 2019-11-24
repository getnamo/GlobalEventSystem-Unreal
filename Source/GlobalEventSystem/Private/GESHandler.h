#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"

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

struct FGESEvent
{
	FString TargetDomain;
	FString TargetFunction;
	TArray<FGESEventListener> Listeners;
	//todo: add restrictions e.g. must apply interface, 
	//	should this be a callback to creator of function/domain?
	//	for refined access?

	
};

class FGESHandler
{
public:

	//Get the Global (default) handler to call all other functions
	static TSharedPtr<FGESHandler> DefaultHandler();

	/**
	*	Create an event in TargetDomain.TargetFunction. Does nothing if already existing
	*/
	void CreateEvent(const FString& TargetDomain, const FString& TargetFunction);
	
	/**
	*	Delete an event in TargetDomain.TargetFunction. Does nothing if missing
	*/
	void DeleteEvent(const FString& TargetDomain, const FString& TargetFunction);

	/** 
	* Listen to an event in TargetDomain.TargetFunction
	*/
	void AddListener(const FString& TargetDomain, const FString& TargetFunction, const FGESEventListener& Listener);
	/**
	* Stop listening to an event in TargetDomain.TargetFunction
	*/
	void RemoveListener(const FString& TargetDomain, const FString& TargetFunction, const FGESEventListener& Listener);

	/**
	* Emit event in TargetDomain.TargetFunction with Struct type parameter data.
	*/
	void ForEachListener(const FString& TargetDomain, const FString& TargetFunction, TFunction<void(const FGESEventListener&)> Callback);

	//overloaded emits
	void EmitEvent(const FString& TargetDomain, const FString& TargetFunction, UStruct* Struct, void* StructPtr);
	void EmitEvent(const FString& TargetDomain, const FString& TargetFunction, const FString& ParamData);
	void EmitEvent(const FString& TargetDomain, const FString& TargetFunction, float ParamData);
	void EmitEvent(const FString& TargetDomain, const FString& TargetFunction, int32 ParamData);
	void EmitEvent(const FString& TargetDomain, const FString& TargetFunction, bool ParamData);


	static FString Key(const FString& TargetDomain, const FString& TargetFunction);

	FGESHandler();
	~FGESHandler();

private:
	static TSharedPtr<FGESHandler> PrivateDefaultHandler;

	//Key == TargetDomain.TargetFunction
	TMap<FString, FGESEvent> FunctionMap;
	TArray<FGESEventListener*> RemovalArray;
};