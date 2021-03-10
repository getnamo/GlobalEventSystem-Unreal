// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GESHandler.h"
#include "GlobalEventSystemBPLibrary.generated.h"

/* 
* Core Global Event System functions. Call anywhere.
*/
UCLASS()
class GLOBALEVENTSYSTEM_API UGlobalEventSystemBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** 
	* Remove this listener from the specified GESEvent.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindEvent(UObject* WorldContextObject, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""), const FString& ReceivingFunction = TEXT(""));

	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindWildcardDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));


	/**
	* Bind a function (to current caller) to GES event. Make sure to match your receiving function parameters to the GESEvent ones.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindEvent(UObject* WorldContextObject, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""), const FString& ReceivingFunction = TEXT(""));

	/**
	* Bind an event delegate to GES event. Use blueprint utility to decode UProperty.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindEventToWildcardDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	/** 
	* Emit desired event with data. Data can be any property (just not UObjects at this time). 
	* Pinning an event means it will emit to future listeners even if the event has already been
	* emitted.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GlobalEventSystem", meta = (CustomStructureParam = "ParameterData", WorldContext = "WorldContextObject"))
	static void GESEmitEventOneParam(UObject* WorldContextObject, TFieldPath<FProperty> ParameterData, bool bPinned = false, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

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

	/**
	* GES Options are global and affect things like logging and param verification (performance options)
	*/
	UFUNCTION(BlueprintCallable, Category = "GlobalEventSystemOptions")
	static void SetGESOptions(const FGESGlobalOptions& InOptions);

	//Wildcard conversions, used in wildcard event delegates from GESBindEventToWildcardDelegate

	/** Convert wildcard property into a literal int */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Integer (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToInt(const FGESWildcardProperty& InProp, int32& OutInt);

	/** Convert wildcard property into a literal float */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Float (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToFloat(const FGESWildcardProperty& InProp, float& OutFloat);

	/** Convert wildcard property into a literal bool */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Bool (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToBool(const FGESWildcardProperty& InProp, bool& OutBool);

	/** Convert wildcard property into a string (reference) */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To String By Ref (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToStringRef(const FGESWildcardProperty& InProp, FString& OutString);

	/** Will still warn, but won't return a boolean for conversion status, used for auto-casting to print strings for debugging */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To String (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static FString Conv_PropToString(const FGESWildcardProperty& InProp);

	/** Convert wildcard property into a literal Name */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Name (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToName(const FGESWildcardProperty& InProp, FName& OutName);

	/** Convert wildcard property into any struct */
	UFUNCTION(BlueprintPure, CustomThunk, meta = (DisplayName = "To Struct (Wildcard Property)", CustomStructureParam = "OutStruct", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToStruct(const FGESWildcardProperty& InProp, TFieldPath<FProperty>& OutStruct);

	/** Convert wildcard property into any Object */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Object (Wildcard Property)", BlueprintAutocast), Category = "Utilities|SocketIO")
	static bool Conv_PropToObject(const FGESWildcardProperty& InProp, UObject*& OutObject);

	//Convert property into c++ accessible form
	DECLARE_FUNCTION(execGESEmitEventOneParam)
	{
		Stack.MostRecentProperty = nullptr;
		FGESFullEmitData EmitData;

		Stack.StepCompiledIn<FObjectProperty>(&EmitData.WorldContext);

		//Determine wildcard property
		Stack.Step(Stack.Object, NULL);
		FProperty* ParameterProp = CastField<FProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;

		EmitData.Property = ParameterProp;
		EmitData.PropertyPtr = PropPtr;

		Stack.StepCompiledIn<FBoolProperty>(&EmitData.EmitData.bPinned);
		Stack.StepCompiledIn<FStrProperty>(&EmitData.EmitData.Domain);
		Stack.StepCompiledIn<FStrProperty>(&EmitData.EmitData.Event);

		P_FINISH;
		P_NATIVE_BEGIN;
		HandleEmit(EmitData);
		P_NATIVE_END;
	}

	DECLARE_FUNCTION(execConv_PropToStruct)
	{
		Stack.MostRecentProperty = nullptr;
		FGESWildcardProperty InProp;
		FGESWildcardProperty OutProp;
		
		//Determine copy wildcard property variables
		Stack.StepCompiledIn<FStructProperty>(&InProp);		

		//Copy the out struct property address
		Stack.Step(Stack.Object, NULL);
		FProperty* ParameterProp = CastField<FProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;

		OutProp.Property = ParameterProp;
		OutProp.PropertyPtr = PropPtr;
		bool bDidCopy = false;

		P_FINISH;
		P_NATIVE_BEGIN;
		bDidCopy = HandlePropToStruct(InProp, OutProp);	//todo: add return support
		P_NATIVE_END;

		*(bool*)RESULT_PARAM = bDidCopy;
	}


private:
	static void HandleEmit(const FGESEmitData& EmitData);
	static bool HandlePropToStruct(const FGESWildcardProperty& InProp, FGESWildcardProperty& FullProp);
	//todo add support for array type props
};
