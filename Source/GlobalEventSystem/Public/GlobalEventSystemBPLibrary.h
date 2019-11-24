// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GESHandler.h"
#include "GlobalEventSystemBPLibrary.generated.h"

/* 
* Core Global Event System functions. Call anywhere
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
	static void GESEmitEvent(const FString& TargetDomain = TEXT("global.default"), const FString& TargetFunction = TEXT(""), UProperty* ParameterData = nullptr);

	//Convert property into c++ accessible form
	DECLARE_FUNCTION(execGESEmitEvent)
	{
		Stack.MostRecentProperty = nullptr;

		FString TargetDomain;
		Stack.StepCompiledIn<UStrProperty>(&TargetDomain);

		FString TargetFunction;
		Stack.StepCompiledIn<UStrProperty>(&TargetFunction);

		//Determine wildcard property
		Stack.Step(Stack.Object, NULL);
		UProperty* ParameterProp = Cast<UProperty>(Stack.MostRecentProperty);
		void* PropPtr = Stack.MostRecentPropertyAddress;
		
		if (ParameterProp->IsA<UStructProperty>())
		{
			UStructProperty* StructProperty = ExactCast<UStructProperty>(ParameterProp);
			P_FINISH;
			P_NATIVE_BEGIN;
			HandleEmit(TargetFunction, StructProperty->Struct, PropPtr, TargetDomain);
			P_NATIVE_END;
			return;
		}
		else if (ParameterProp->IsA<UStrProperty>())
		{
			UStrProperty* StrProperty = Cast<UStrProperty>(ParameterProp);
			FString Data = StrProperty->GetPropertyValue(PropPtr);
			P_FINISH;
			P_NATIVE_BEGIN;
			HandleEmit(TargetFunction, Data, TargetDomain);
			P_NATIVE_END;
			return;
		}
		else if (ParameterProp->IsA<UNumericProperty>())
		{
			UNumericProperty* NumericProperty = Cast<UNumericProperty>(ParameterProp);
			if (NumericProperty->IsFloatingPoint())
			{
				double Data = NumericProperty->GetFloatingPointPropertyValue(PropPtr);
				P_FINISH;
				P_NATIVE_BEGIN;
				HandleEmit(TargetFunction, Data, TargetDomain);
				P_NATIVE_END;
				return;
			}
			else
			{
				int64 Data = NumericProperty->GetSignedIntPropertyValue(PropPtr);
				P_FINISH;
				P_NATIVE_BEGIN;
				HandleEmit(TargetFunction, Data, TargetDomain);
				P_NATIVE_END;
				return;
			}
		}
		else if (ParameterProp->IsA<UBoolProperty>())
		{
			UBoolProperty* BoolProperty = Cast<UBoolProperty>(ParameterProp);
			bool Data = BoolProperty->GetPropertyValue(PropPtr);
			P_FINISH;
			P_NATIVE_BEGIN;
			HandleEmit(TargetFunction, Data, TargetDomain);
			P_NATIVE_END;
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GESEmitEvent Parameter type unsupported, event not emitted."));
			P_FINISH;
		}
	}


private:

	//overloaded emits
	static void HandleEmit(const FString& TargetFunction, UStruct* DataStruct, void* DataStructPtr, const FString& TargetDomain);
	static void HandleEmit(const FString& TargetFunction, const FString& Data, const FString& TargetDomain);
	static void HandleEmit(const FString& TargetFunction, double Data, const FString& TargetDomain);
	static void HandleEmit(const FString& TargetFunction, int64 Data, const FString& TargetDomain);
	static void HandleEmit(const FString& TargetFunction, bool Data, const FString& TargetDomain);
	//todo add support for array type props


};
