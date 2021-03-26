// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GESHandler.h"
#include "GameplayTagContainer.h"
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

	/**
	* Remove this listener from the specified GESEvent given by GameplayTag.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindTagEvent(UObject* WorldContextObject, FGameplayTag Tag, const FString& ReceivingFunction = TEXT(""));


	/** 
	* Call this on endplay to remove all events associated with this graph. If context isn't specified, graph context used (default use case).
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindAllEventsForContext(UObject* WorldContextObject, UObject* Context = nullptr);


	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges sever stoplisten", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESUnbindTagDelegate(UObject* WorldContextObject, FGameplayTag Tag, const FGESOnePropertySignature& ReceivingFunction);

	/**
	* Bind a function (to current caller) to GES event. Make sure to match your receiving function parameters to the GESEvent ones.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindEvent(UObject* WorldContextObject, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""), const FString& ReceivingFunction = TEXT(""));

	/**
	* Bind a function (to current caller) to GES event defined by a GamePlayTag
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindTagEvent(UObject* WorldContextObject, FGameplayTag DomainedEventTag, const FString& ReceivingFunction = TEXT(""));

	/**
	* Bind a function (to current caller) to GES event defined by a GamePlayTag
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindTagEventToDelegate(UObject* WorldContextObject, FGameplayTag DomainedEventTag, const FGESOnePropertySignature& ReceivingFunction);

	/**
	* Bind an event delegate to GES event. Use blueprint utility to decode UProperty.
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "ges create listen", WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESBindEventToDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	/** 
	* Emit desired event with data. Data can be any single property (wrap arrays/maps etc in a struct or object)
	* Pinning an event means it will emit to future listeners even if the event has already been
	* emitted.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GlobalEventSystem", meta = (CustomStructureParam = "ParameterData", WorldContext = "WorldContextObject"))
	static void GESEmitEventOneParam(UObject* WorldContextObject, TFieldPath<FProperty> ParameterData, bool bPinned = false, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	/** 
	* Just emits the event with no additional data
	* Pinning an event means it will emit to future listeners even if the event has already been
	* emitted.
	*/
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESEmitEvent(UObject* WorldContextObject, bool bPinned = false, const FString& Domain = TEXT("global.default"), const FString& Event = TEXT(""));

	/**
	* Just emits the event with no additional data using GameplayTags to define domain and event.
	* Pinning an event means it will emit to future listeners even if the event has already been
	* emitted.
	*/
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "GlobalEventSystem")
	static void GESEmitTagEvent(UObject* WorldContextObject, FGameplayTag DomainedEventTag, bool bPinned = false);

	/**
	* Emit desired event with data using GameplayTags to define domain and event. Data can be any single
	* property (wrap arrays/maps etc in a struct or object).
	* Pinning an event means it will emit to future listeners even if the event has already been
	* emitted.
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GlobalEventSystem", meta = (CustomStructureParam = "ParameterData", WorldContext = "WorldContextObject"))
	static void GESEmitTagEventOneParam(UObject* WorldContextObject, TFieldPath<FProperty> ParameterData, FGameplayTag DomainedEventTag, bool bPinned = false);

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
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Integer (Wildcard Property)", BlueprintAutocast), Category = "Utilities|GES")
	static bool Conv_PropToInt(const FGESWildcardProperty& InProp, int32& OutInt);

	/** Convert wildcard property into a literal float */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Float (Wildcard Property)", BlueprintAutocast), Category = "Utilities|GES")
	static bool Conv_PropToFloat(const FGESWildcardProperty& InProp, float& OutFloat);

	/** Convert wildcard property into a literal bool */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Bool (Wildcard Property)", BlueprintAutocast), Category = "Utilities|GES")
	static bool Conv_PropToBool(const FGESWildcardProperty& InProp, bool& OutBool);

	/** Convert wildcard property into a string (reference) */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To String Ref with status (Wildcard Property)"), Category = "Utilities|GES")
	static bool Conv_PropToStringRef(const FGESWildcardProperty& InProp, FString& OutString);

	/** Will still warn, but won't return a boolean for conversion status, used for auto-casting to print strings for debugging */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To String (Wildcard Property)", BlueprintAutocast), Category = "Utilities|GES")
	static FString Conv_PropToString(const FGESWildcardProperty& InProp);

	/** Convert wildcard property into a literal Name */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Name (Wildcard Property)", BlueprintAutocast), Category = "Utilities|GES")
	static bool Conv_PropToName(const FGESWildcardProperty& InProp, FName& OutName);

	/** Convert wildcard property into any struct */
	UFUNCTION(BlueprintPure, CustomThunk, meta = (DisplayName = "To Struct (Wildcard Property)", CustomStructureParam = "OutStruct", BlueprintAutocast), Category = "Utilities|GES")
	static bool Conv_PropToStruct(const FGESWildcardProperty& InProp, TFieldPath<FProperty>& OutStruct);

	/** Convert wildcard property into any Object */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Object (Wildcard Property)", BlueprintAutocast), Category = "Utilities|GES")
	static bool Conv_PropToObject(const FGESWildcardProperty& InProp, UObject*& OutObject);

	/** Convert a GameplayTag into a Domain and Event string pair */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Domain and Event (GameplayTag)", BlueprintAutocast), Category = "Utilities|GES")
	static void Conv_TagToDomainAndEvent(FGameplayTag InTag, FString& OutDomain, FString& OutEvent);

	//Convert property into c++ accessible form
	DECLARE_FUNCTION(execGESEmitEventOneParam)
	{
		Stack.MostRecentProperty = nullptr;
		FGESPropertyEmitContext EmitData;

		Stack.StepCompiledIn<FObjectProperty>(&EmitData.WorldContext);

		//Determine wildcard property
		Stack.Step(Stack.Object, NULL);
		FProperty* ParameterProp = CastField<FProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;

		EmitData.Property = ParameterProp;
		EmitData.PropertyPtr = PropPtr;

		Stack.StepCompiledIn<FBoolProperty>(&EmitData.bPinned);
		Stack.StepCompiledIn<FStrProperty>(&EmitData.Domain);
		Stack.StepCompiledIn<FStrProperty>(&EmitData.Event);

		P_FINISH;
		P_NATIVE_BEGIN;
		HandleEmit(EmitData);
		P_NATIVE_END;
	}

	DECLARE_FUNCTION(execGESEmitTagEventOneParam)
	{
		Stack.MostRecentProperty = nullptr;
		FGESPropertyEmitContext EmitData;

		Stack.StepCompiledIn<FObjectProperty>(&EmitData.WorldContext);

		//Determine wildcard property
		Stack.Step(Stack.Object, NULL);
		if (Stack.MostRecentProperty != nullptr)
		{
			EmitData.Property = CastField<FProperty>(Stack.MostRecentProperty);
			EmitData.PropertyPtr = Stack.MostRecentPropertyAddress;
		}
		
		FGameplayTag Tag;
		Stack.StepCompiledIn<FStructProperty>(&Tag);
		Conv_TagToDomainAndEvent(Tag, EmitData.Domain, EmitData.Event);

		
		Stack.StepCompiledIn<FBoolProperty>(&EmitData.bPinned);

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
		//Stack.Step(Stack.Object, NULL);
		//InProp.Property = CastField<FProperty>(Stack.MostRecentProperty);
		//InProp.PropertyPtr = Stack.MostRecentPropertyAddress;

		//Copy the out struct property address
		//Stack.StepCompiledIn<FStructProperty>(&OutProp);
		Stack.Step(Stack.Object, NULL);
		FProperty* ParameterProp = CastField<FProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;

		OutProp.Property = ParameterProp;
		OutProp.PropertyPtr = PropPtr;
		bool bDidCopy = false;

		P_FINISH;
		P_NATIVE_BEGIN;
		bDidCopy = HandlePropToStruct(InProp, OutProp);
		P_NATIVE_END;

		*(bool*)RESULT_PARAM = bDidCopy;
	}


private:
	static void HandleEmit(const FGESPropertyEmitContext& EmitData);
	static bool HandlePropToStruct(const FGESWildcardProperty& InProp, FGESWildcardProperty& FullProp);
};
