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
	Listener.ReceiverWCO = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;

	FGESHandler::DefaultHandler()->RemoveListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::GESUnbindTagEvent(UObject* WorldContextObject, FGameplayTag Tag, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FString Domain;
	FString Event;
	Conv_TagToDomainAndEvent(Tag, Domain, Event);
	GESUnbindEvent(WorldContextObject, Domain, Event, ReceivingFunction);
}

void UGlobalEventSystemBPLibrary::GESUnbindAllEventsForContext(UObject* WorldContextObject, UObject* Context /*= nullptr*/)
{
	if (Context == nullptr)
	{
		Context = WorldContextObject;
	}
	FGESHandler::DefaultHandler()->RemoveAllListenersForReceiver(Context);
}

void UGlobalEventSystemBPLibrary::GESUnbindDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.ReceiverWCO = WorldContextObject;

	if (ReceivingFunction.GetUObject()->IsValidLowLevelFast())
	{
		Listener.FunctionName = WorldContextObject->GetName() + ReceivingFunction.GetUObject()->GetName();
	}
	else
	{
		Listener.FunctionName = WorldContextObject->GetName() + TEXT(".UnboundDelegate");
	}
	Listener.OnePropertyFunctionDelegate = ReceivingFunction;
	Listener.bIsBoundToDelegate = true;

	FGESHandler::DefaultHandler()->RemoveListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::GESUnbindTagDelegate(UObject* WorldContextObject, FGameplayTag Tag, const FGESOnePropertySignature& ReceivingFunction)
{
	FString Domain;
	FString Event;
	Conv_TagToDomainAndEvent(Tag, Domain, Event);
	GESUnbindDelegate(WorldContextObject, ReceivingFunction, Domain, Event);
}

void UGlobalEventSystemBPLibrary::GESBindEvent(UObject* WorldContextObject, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.ReceiverWCO = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;
	Listener.LinkFunction();	//this makes the function valid by finding a reference to it

	FGESHandler::DefaultHandler()->AddListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::GESBindTagEvent(UObject* WorldContextObject, FGameplayTag DomainedEventTag, const FString& ReceivingFunction /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.ReceiverWCO = WorldContextObject;
	Listener.FunctionName = ReceivingFunction;
	Listener.LinkFunction();	//this makes the function valid by finding a reference to it

	FString Domain;
	FString Event;
	Conv_TagToDomainAndEvent(DomainedEventTag, Domain, Event);

	FGESHandler::DefaultHandler()->AddListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::GESBindTagEventToDelegate(UObject* WorldContextObject, FGameplayTag DomainedEventTag, const FGESOnePropertySignature& ReceivingFunction)
{
	FString Domain;
	FString Event;
	Conv_TagToDomainAndEvent(DomainedEventTag, Domain, Event);
	GESBindEventToDelegate(WorldContextObject, ReceivingFunction, Domain, Event);
}

void UGlobalEventSystemBPLibrary::GESBindEventToDelegate(UObject* WorldContextObject, const FGESOnePropertySignature& ReceivingFunction, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/)
{
	FGESEventListener Listener;
	Listener.ReceiverWCO = WorldContextObject;
	if (ReceivingFunction.GetUObject()->IsValidLowLevelFast())
	{
		Listener.FunctionName = WorldContextObject->GetName() + ReceivingFunction.GetUObject()->GetName();
	}
	else
	{
		Listener.FunctionName = WorldContextObject->GetName() + TEXT(".UnboundDelegate");
	}
	Listener.OnePropertyFunctionDelegate = ReceivingFunction;
	Listener.bIsBoundToDelegate = true;

	FGESHandler::DefaultHandler()->AddListener(Domain, Event, Listener);
}

void UGlobalEventSystemBPLibrary::HandleEmit(const FGESPropertyEmitContext& FullEmitData)
{
	FGESHandler::DefaultHandler()->EmitPropertyEvent(FullEmitData);
}

void UGlobalEventSystemBPLibrary::GESEmitEventOneParam(UObject* WorldContextObject, TFieldPath<FProperty> ParameterData, bool bPinned /*= false*/, const FString& Domain /*= TEXT("global.default")*/, const FString& Event /*= TEXT("")*/)
{
	//this never gets called due to custom thunk
}

void UGlobalEventSystemBPLibrary::GESEmitEvent(UObject* WorldContextObject, bool bPinned /*= false*/, const FString& Domain /*= TEXT("global.default")*/, const FString& EventName /*= TEXT("")*/)
{
	FGESEmitContext EmitData;
	EmitData.bPinned = bPinned;
	EmitData.Domain = Domain;
	EmitData.Event = EventName;
	EmitData.WorldContext = WorldContextObject;
	FGESHandler::DefaultHandler()->EmitEvent(EmitData);
}

void UGlobalEventSystemBPLibrary::GESEmitTagEvent(UObject* WorldContextObject, FGameplayTag DomainedEventTag, bool bPinned /*= false*/)
{
	FGESEmitContext EmitData;
	EmitData.bPinned = bPinned;
	Conv_TagToDomainAndEvent(DomainedEventTag, EmitData.Domain, EmitData.Event);
	EmitData.WorldContext = WorldContextObject;
	FGESHandler::DefaultHandler()->EmitEvent(EmitData);
}

void UGlobalEventSystemBPLibrary::GESEmitTagEventOneParam(UObject* WorldContextObject, TFieldPath<FProperty> ParameterData, FGameplayTag DomainedEventTag, bool bPinned /*= false*/)
{
	//this never gets called due to custom thunk
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
	if (InProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToInt InProp is a nullptr"));
		return false;
	}

	if (InProp.Property->IsA<FNumericProperty>())
	{
		FNumericProperty* Property = CastField<FNumericProperty>(InProp.Property.Get());
		if (!Property->IsFloatingPoint())
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
	if (InProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToFloat InProp is a nullptr"));
		return false;
	}

	if (InProp.Property->IsA<FNumericProperty>())
	{
		FNumericProperty* Property = CastField<FNumericProperty>(InProp.Property.Get());
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
	if (InProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToBool InProp is a nullptr"));
		return false;
	}

	if (InProp.Property->IsA<FBoolProperty>())
	{
		FBoolProperty* Property = CastField<FBoolProperty>(InProp.Property.Get());
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
	if (InProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToStringRef InProp is a nullptr"));
		return false;
	}

	if (InProp.Property->IsA<FStrProperty>())
	{
		FStrProperty* Property = CastField<FStrProperty>(InProp.Property.Get());
		OutString = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToString %s is not an FString, attempted best conversion for display purposes."), *InProp.Property->GetName());

		//Convert logic
		if (InProp.Property->IsA<FNumericProperty>())
		{
			FNumericProperty* Property = CastField<FNumericProperty>(InProp.Property.Get());
			if (Property->IsFloatingPoint())
			{
				OutString = FString::SanitizeFloat(Property->GetFloatingPointPropertyValue(InProp.PropertyPtr));
			}
			else
			{
				OutString = FString::FromInt(Property->GetSignedIntPropertyValue(InProp.PropertyPtr));
			}
		}
		else if (InProp.Property->IsA<FBoolProperty>())
		{
			FBoolProperty* Property = CastField<FBoolProperty>(InProp.Property.Get());
			if (Property->GetPropertyValue(InProp.PropertyPtr))
			{
				OutString = TEXT("True");
			}
			else
			{
				OutString = TEXT("False");
			}
		}
		else if (InProp.Property->IsA<FNameProperty>())
		{
			FNameProperty* Property = CastField <FNameProperty>(InProp.Property.Get());
			OutString = Property->GetPropertyValue(InProp.PropertyPtr).ToString();
		}
		else if (InProp.Property->IsA<FObjectProperty>())
		{
			FObjectProperty* Property = CastField<FObjectProperty>(InProp.Property.Get());
			UObject* Object = Property->GetPropertyValue(InProp.PropertyPtr);

			if (Object->IsValidLowLevelFast())
			{
				OutString = Object->GetName() + TEXT(", type: ") + Object->GetClass()->GetName();
			}
			else
			{
				OutString = TEXT("Null Object");
			}
		}
		else if (InProp.Property->IsA<FStructProperty>())
		{
			FStructProperty* Property = CastField<FStructProperty>(InProp.Property.Get());
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
	if (InProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToName InProp is a nullptr"));
		return false;
	}

	if (InProp.Property->IsA<FNameProperty>())
	{
		FNameProperty* Property = CastField<FNameProperty>(InProp.Property.Get());
		OutName = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToName %s is not an FName."), *InProp.Property->GetName());
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToStruct(const FGESWildcardProperty& InProp, TFieldPath<FProperty>& OutStruct)
{
	//doesn't get called due to custom thunk
	return false;
}

bool UGlobalEventSystemBPLibrary::HandlePropToStruct(const FGESWildcardProperty& InProp, FGESWildcardProperty& OutProp)
{
	if (InProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::HandlePropToStruct InProp is a nullptr"));
		return false;
	}
	if (OutProp.Property == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::HandlePropToStruct OutProp is a nullptr"));
		return false;
	}

	if (InProp.Property->IsA<FStructProperty>() && OutProp.Property->IsA<FStructProperty>())
	{
		FStructProperty* InStructProp = CastField<FStructProperty>(InProp.Property.Get());
		FStructProperty* OutStructProp = CastField<FStructProperty>(OutProp.Property.Get());

		OutStructProp->CopyCompleteValue(OutProp.PropertyPtr, InProp.PropertyPtr);
		return true;
	}
	else
	{
		return false;
	}
}

bool UGlobalEventSystemBPLibrary::Conv_PropToObject(const FGESWildcardProperty& InProp, UObject*& OutObject)
{
	if (InProp.Property == nullptr)
	{
		return false;
	}

	if (InProp.Property->IsA<FObjectProperty>())
	{
		FObjectProperty* Property = CastField<FObjectProperty>(InProp.Property.Get());
		OutObject = Property->GetPropertyValue(InProp.PropertyPtr);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGlobalEventSystemBPLibrary::Conv_PropToObject %s is not an Object."), *InProp.Property->GetName());
		return false;
	}
}

void UGlobalEventSystemBPLibrary::Conv_TagToDomainAndEvent(FGameplayTag InTag, FString& OutDomain, FString& OutEvent)
{
	FString DomainAndEvent = InTag.GetTagName().ToString();

	bool bFound = DomainAndEvent.Split(TEXT("."), &OutDomain, &OutEvent, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	if (!bFound)
	{
		OutDomain = TEXT("global.default");
		OutEvent = DomainAndEvent;
	}
}

