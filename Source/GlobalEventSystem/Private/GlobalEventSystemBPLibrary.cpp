// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GlobalEventSystemBPLibrary.h"
#include "GlobalEventSystem.h"

UGlobalEventSystemBPLibrary::UGlobalEventSystemBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UGlobalEventSystemBPLibrary::GESUnbindEvent(UObject* WorldContextObject, const FString& TargetDomain /*= TEXT("global.default")*/, const FString& TargetFunction /*= TEXT("")*/, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.Receiver = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;

	FGESHandler::DefaultHandler()->RemoveListener(TargetDomain, TargetFunction, Listener);
}

void UGlobalEventSystemBPLibrary::GESBindEvent(UObject* WorldContextObject, const FString& TargetDomain /*= TEXT("global.default")*/, const FString& TargetFunction /*= TEXT("")*/, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.Receiver = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;
	Listener.LinkFunction();	//this makes the function valid by finding a reference to it

	FGESHandler::DefaultHandler()->AddListener(TargetDomain, TargetFunction, Listener);
}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, UStruct* DataStruct, void* DataStructPtr, const FString& TargetDomain)
{
	FGESHandler::DefaultHandler()->EmitEvent(TargetDomain, TargetFunction, DataStruct, DataStructPtr);
}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, const FString& Data, const FString& TargetDomain)
{
	FGESHandler::DefaultHandler()->EmitEvent(TargetDomain, TargetFunction, Data);
}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, double Data, const FString& TargetDomain)
{
	FGESHandler::DefaultHandler()->EmitEvent(TargetDomain, TargetFunction, (float)Data);
}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, int64 Data, const FString& TargetDomain)
{
	FGESHandler::DefaultHandler()->EmitEvent(TargetDomain, TargetFunction, (int32)Data);
}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, bool Data, const FString& TargetDomain)
{
	FGESHandler::DefaultHandler()->EmitEvent(TargetDomain, TargetFunction, Data);
}

void UGlobalEventSystemBPLibrary::GESEmitEvent(const FString& TargetDomain /*= TEXT("global.default")*/, const FString& TargetFunction /*= TEXT("")*/, UProperty* ParameterData /*= nullptr*/)
{
	//this never gets called due to custom thunk
}

