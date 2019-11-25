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

	/** 
	* Remove this listener from the specified GESEvent.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindEvent(UObject* WorldContextObject, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""), const FString& ReceivingFunction = TEXT(""));

	/**
	* Bind a function (to current caller) to GES event. Make sure to match your receiving function parameters to the GESEvent ones.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindEvent(UObject* WorldContextObject, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""), const FString& ReceivingFunction = TEXT(""));

	/** 
	* Emit desired event with data. Data can be any property (just not UObjects at this time). 
	* Pinning an event means it will emit to future listeners even if the event has already been
	* emitted.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GlobalEventSystem", meta = (CustomStructureParam = "ParameterData", WorldContext = "WorldContextObject"))
	static void GESEmitEventOneParam(UObject* WorldContextObject, bool bPinned = false, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""), UProperty* ParameterData = nullptr);

	/** 
	* Just emits the event with no additional data
	*/
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESEmitEvent(UObject* WorldContextObject, bool bPinned = false, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	/** 
	* If an event was pinned, this will unpin it. If you wish to re-pin a different event you need to unpin the old event first.
	*/
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnpinEvent(UObject* WorldContextObject, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	//Convert property into c++ accessible form
	DECLARE_FUNCTION(execGESEmitEventOneParam)
	{
		Stack.MostRecentProperty = nullptr;
		FGESEmitData EmitData;

		Stack.StepCompiledIn<UObjectProperty>(&EmitData.WorldContext);
		Stack.StepCompiledIn<UBoolProperty>(&EmitData.bPinned);
		Stack.StepCompiledIn<UStrProperty>(&EmitData.Domain);
		Stack.StepCompiledIn<UStrProperty>(&EmitData.Event);

		//Determine wildcard property
		Stack.Step(Stack.Object, NULL);
		UProperty* ParameterProp = Cast<UProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;

		EmitData.Property = ParameterProp;
		EmitData.PropertyPtr = PropPtr;

		P_FINISH;
		P_NATIVE_BEGIN;
		HandleEmit(EmitData);
		P_NATIVE_END;
	}


private:
	static void HandleEmit(const FGESEmitData& EmitData);
	//todo add support for array type props
};
