// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GlobalEventSystemBPLibrary.h"
#include "GlobalEventSystem.h"

UGlobalEventSystemBPLibrary::UGlobalEventSystemBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

void UGlobalEventSystemBPLibrary::GESBindEvent(UObject* WorldContextObject, const FString& ReceivingFunction, const FString& TargetFunction, const FString& TargetDomain /*= TEXT("global.default")*/)
{

}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, UStruct* DataStruct, void* DataStructPtr, const FString& TargetDomain)
{

}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, const FString& Data, const FString& TargetDomain)
{

}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, double Data, const FString& TargetDomain)
{

}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, int64 Data, const FString& TargetDomain)
{

}

void UGlobalEventSystemBPLibrary::HandleEmit(const FString& TargetFunction, bool Data, const FString& TargetDomain)
{

}



void UGlobalEventSystemBPLibrary::GESEmitEvent(const FString& TargetFunction, const FString& TargetDomain /*= TEXT("global.default")*/, UProperty* ParameterData /*= nullptr*/)
{
	//this never gets called due to custom thunk
}

