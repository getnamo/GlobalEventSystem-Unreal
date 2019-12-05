// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GlobalEventSystemBPLibrary.h"
#include "GlobalEventSystem.h"

UGlobalEventSystemBPLibrary::UGlobalEventSystemBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UGlobalEventSystemBPLibrary::GESUnbindEvent(UObject* WorldContextObject, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.Receiver = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;

	FGESHandler::DefaultHandler()->RemoveListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::GESBindEvent(UObject* WorldContextObject, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.Receiver = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;
	Listener.LinkFunction();	//this makes the function valid by finding a reference to it

	FGESHandler::DefaultHandler()->AddListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::GESBindEventToWildcardDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.Receiver = WorldContextObject;
	Listener.OnePropertyFunctionDelegate = ReceivingFunction;
	Listener.bIsBoundToDelegate = true;

	FGESHandler::DefaultHandler()->AddListener(Domain, Event, Listener);
}

//void UGlobalEventSystemBPLibrary::GESBindEventToDelegate(UObject* WorldContextObject, const FScriptDelegate& ReceivingFunction, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/)
/*{
	UE_LOG(LogTemp, Log, TEXT("GESBindEventToDelegate"));
}*/

void UGlobalEventSystemBPLibrary::HandleEmit(const FGESEmitData& EmitData)
{
	FGESHandler::DefaultHandler()->EmitEvent(EmitData);
}

void UGlobalEventSystemBPLibrary::GESEmitEventOneParam(UObject* WorldContextObject, bool bPinned /*= false*/, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/, UProperty* ParameterData /*= nullptr*/)
{
	//this never gets called due to custom thunk
}

void UGlobalEventSystemBPLibrary::GESEmitEvent(UObject* WorldContextObject, bool bPinned /*= false*/, const FString& Domain /*= TEXT("global.default")*/, const FString& EventName /*= TEXT("")*/)
{
	FGESEmitData EmitData;
	EmitData.bPinned = bPinned;
	EmitData.Domain = Domain;
	EmitData.Event = EventName;
	EmitData.WorldContext = WorldContextObject;
	FGESHandler::DefaultHandler()->EmitEvent(EmitData);
}

void UGlobalEventSystemBPLibrary::GESUnpinEvent(UObject* WorldContextObject, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/)
{
	FGESHandler::DefaultHandler()->UnpinEvent(Domain, Event);
}

void UGlobalEventSystemBPLibrary::SetGESOptions(const FGESGlobalOptions& InOptions)
{
	FGESHandler::DefaultHandler()->SetOptions(InOptions);
}

bool UGlobalEventSystemBPLibrary::Conv_PropToInt(const FGESWildcardProperty& InProp, int32& OutInt)
{
	if (InProp.Property->IsA<UNumericProperty>())
	{
		UNumericProperty* Property = Cast<UNumericProperty>(InProp.Property);
		if (Property->IsFloatingPoint())
		{
			OutInt = Property->GetSignedIntPropertyValue(InProp.PropertyPtr);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToInt %s is not an integer number, float truncated to int."), *InProp.Property->GetName());
			OutInt = Property->GetFloatingPointPropertyValue(InProp.PropertyPtr);
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToInt %s is not an integer."), *InProp.Property->GetName());
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToFloat(const FGESWildcardProperty& InProp, float& OutFloat)
{
	if (InProp.Property->IsA<UNumericProperty>())
	{
		UNumericProperty* Property = Cast<UNumericProperty>(InProp.Property);
		if (Property->IsFloatingPoint())
		{
			OutFloat = Property->GetFloatingPointPropertyValue(InProp.PropertyPtr);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToFloat %s is not a floating number, converted int to float."), *InProp.Property->GetName());
			OutFloat = Property->GetSignedIntPropertyValue(InProp.PropertyPtr);
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToFloat %s is not a float."), *InProp.Property->GetName());
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToBool(const FGESWildcardProperty& InProp, bool& OutBool)
{
	if (InProp.Property->IsA<UBoolProperty>())
	{
		UBoolProperty* Property = Cast<UBoolProperty>(InProp.Property);
		OutBool = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToBool %s is not a bool."), *InProp.Property->GetName());
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToStringRef(const FGESWildcardProperty& InProp, FString& OutString)
{
	if (InProp.Property->IsA<UStrProperty>())
	{
		UStrProperty* Property = Cast<UStrProperty>(InProp.Property);
		OutString = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToString %s is not an FString, attempted best conversion for display purposes."), *InProp.Property->GetName());

		//Convert logic
		if (InProp.Property->IsA<UNumericProperty>())
		{
			UNumericProperty* Property = Cast<UNumericProperty>(InProp.Property);
			if (Property->IsFloatingPoint())
			{
				OutString = FString::SanitizeFloat(Property->GetFloatingPointPropertyValue(InProp.PropertyPtr));
			}
			else
			{
				OutString = FString::FromInt(Property->GetSignedIntPropertyValue(InProp.PropertyPtr));
			}
		}
		else if (InProp.Property->IsA<UBoolProperty>())
		{
			UBoolProperty* Property = Cast<UBoolProperty>(InProp.Property);
			if (Property->GetPropertyValue(InProp.PropertyPtr))
			{
				OutString = TEXT("True");
			}
			else
			{
				OutString = TEXT("False");
			}
		}
		else if (InProp.Property->IsA<UNameProperty>())
		{
			UNameProperty* Property = Cast<UNameProperty>(InProp.Property);
			OutString = Property->GetPropertyValue(InProp.PropertyPtr).ToString();
		}
		else if (InProp.Property->IsA<UObjectProperty>())
		{
			UObjectProperty* Property = Cast<UObjectProperty>(InProp.Property);
			UObject* Object = Property->GetPropertyValue(InProp.PropertyPtr);
			OutString = Object->GetName() + TEXT(", type: ") + Object->GetClass()->GetName();
		}
		else if (InProp.Property->IsA<UStructProperty>())
		{
			UStructProperty* Property = Cast<UStructProperty>(InProp.Property);
			OutString = Property->GetName() + TEXT(", type: ") + Property->Struct->GetName();
		}
		return false;
	}
}

FString UGlobalEventSystemBPLibrary::Conv_PropToString(const FGESWildcardProperty& InProp)
{
	FString OutString;
	Conv_PropToStringRef(InProp, OutString);
	return OutString;
}

bool UGlobalEventSystemBPLibrary::Conv_PropToName(const FGESWildcardProperty& InProp, FName& OutName)
{
	if (InProp.Property->IsA<UNameProperty>())
	{
		UNameProperty* Property = Cast<UNameProperty>(InProp.Property);
		OutName = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToName %s is not an FName."), *InProp.Property->GetName());
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToStruct(const FGESWildcardProperty& InProp, UProperty*& OutStruct)
{
	if (InProp.Property->IsA<UStructProperty>())
	{
		UStructProperty* StructProperty = Cast<UStructProperty>(InProp.Property);
		OutStruct = (UProperty*)InProp.PropertyPtr;
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToStruct %s is not a Struct."), *InProp.Property->GetName());
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToObject(const FGESWildcardProperty& InProp, UObject*& OutObject)
{
	if (InProp.Property->IsA<UNumericProperty>())
	{
		UObjectProperty* Property = Cast<UObjectProperty>(InProp.Property);
		OutObject = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToObject %s is not an Object."), *InProp.Property->GetName());
		return false;
	}
}

