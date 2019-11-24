// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GESHandler.h"
#include "GlobalEventSystemBPLibrary.generated.h"



/* 
* Core Global Event System functions. Call anywhere.
*/
UCLASS()
class UGlobalEventSystemBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges bind create listen event", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindEvent(UObject* WorldContextObject, const FString& TargetDomain = TEXT("global.default"), const FString& TargetFunction = TEXT(""), const FString& ReceivingFunction = TEXT(""));

	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges bind create listen event", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindEvent(UObject* WorldContextObject, const FString& TargetDomain = TEXT("global.default"), const FString& TargetFunction = TEXT(""), const FString& ReceivingFunction = TEXT(""));


	//todo: try emit auto cast to property maybe
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GlobalEventSystem", meta = (CustomStructureParam = "ParameterData"))
	static void GESEmitEvent(bool bPinned = false, const FString& TargetDomain = TEXT("global.default"), const FString& TargetFunction = TEXT(""), UProperty* ParameterData = nullptr);

	//Convert property into c++ accessible form
	DECLARE_FUNCTION(execGESEmitEvent)
	{
		Stack.MostRecentProperty = nullptr;
		FGESEmitData Emit;

		Stack.StepCompiledIn<UBoolProperty>(&Emit.bPinned);
		Stack.StepCompiledIn<UStrProperty>(&Emit.TargetDomain);
		Stack.StepCompiledIn<UStrProperty>(&Emit.TargetFunction);

		//Determine wildcard property
		Stack.Step(Stack.Object, NULL);
		UProperty* ParameterProp = Cast<UProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;

		Emit.Property = ParameterProp;
		Emit.PropertyPtr = PropPtr;

		P_FINISH;
		P_NATIVE_BEGIN;
		HandleEmit(Emit);
		P_NATIVE_END;
	}


private:
	static void HandleEmit(const FGESEmitData& EmitData);
	//todo add support for array type props
};
