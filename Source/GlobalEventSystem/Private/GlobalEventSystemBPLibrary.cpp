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

void UGlobalEventSystemBPLibrary::HandleEmit(const FGESEmitData& EmitData)
{
	FGESHandler::DefaultHandler()->EmitEvent(EmitData);
}

void UGlobalEventSystemBPLibrary::GESEmitEvent(bool bPinned /*= false*/, const FString& TargetDomain /*= TEXT("global.default")*/, const FString& TargetFunction /*= TEXT("")*/, UProperty* ParameterData /*= nullptr*/)
{
	//this never gets called due to custom thunk
}

