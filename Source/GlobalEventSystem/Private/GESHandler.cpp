#include "GESHandler.h"
#include "GlobalEventSystemBPLibrary.h"
#include "Engine/World.h"

TSharedPtr<FGESHandler> FGESHandler::PrivateDefaultHandler = MakeShareable(new FGESHandler());

bool FGESHandler::FirstParamIsCppType(UFunction* Function, const FString& TypeString)
{
	TArray<FProperty*> Properties;
	FunctionParameters(Function, Properties);
	if (Properties.Num() == 0)
	{
		return false;
	}

	const FString& FirstParam = Properties[0]->GetCPPType();
	return (FirstParam == TypeString);
}

bool FGESHandler::FirstParamIsSubclassOf(UFunction* Function, FFieldClass* ClassType)
{
	TArray<FProperty*> Properties;
	FunctionParameters(Function, Properties);
	if (Properties.Num() == 0)
	{
		return false;
	}
	return Properties[0]->GetClass()->IsChildOf(ClassType);
}

FString FGESHandler::ListenerLogString(const FGESEventListener& Listener)
{
	return Listener.Receiver->GetName() + TEXT(":") + Listener.FunctionName;
}

FString FGESHandler::EventLogString(const FGESEvent& Event)
{
	return Event.Domain + TEXT(".") + Event.Event;
}

FString FGESHandler::EmitEventLogString(const FGESEmitData& EmitData)
{
	return EmitData.Domain + TEXT(".") + EmitData.Event;
}

void FGESHandler::FunctionParameters(UFunction* Function, TArray<FProperty*>& OutParamProperties)
{
	TFieldIterator<FProperty> Iterator(Function);

	while (Iterator && (Iterator->PropertyFlags & CPF_Parm))
	{
		FProperty* Prop = *Iterator;
		OutParamProperties.Add(Prop);
		++Iterator;
	}
}

bool FGESHandler::FunctionHasValidParams(UFunction* Function, FFieldClass* ClassType, const FGESEmitData& EmitData, const FGESEventListener& Listener)
{
	if (FirstParamIsSubclassOf(Function, ClassType))
	{
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent %s skipped listener %s due to function not having a matching %s signature."),
			*EmitEventLogString(EmitData),
			*ListenerLogString(Listener),
			*ClassType->GetName());
		return false;
	}
}

TSharedPtr<FGESHandler> FGESHandler::DefaultHandler()
{
	return FGESHandler::PrivateDefaultHandler;
}

void FGESHandler::CreateEvent(const FString& Domain, const FString& Event, bool bPinned /*= false*/)
{
	FGESEvent CreatedFunction;
	CreatedFunction.Domain = Domain;
	CreatedFunction.Event = Event;
	CreatedFunction.bPinned = bPinned;
	EventMap.Add(Key(Domain, Event), CreatedFunction);
}

void FGESHandler::DeleteEvent(const FString& Domain, const FString& Event)
{
	EventMap.Remove(Key(Domain, Event));
}

void FGESHandler::DeleteEvent(const FString& DomainAndEvent)
{
	EventMap.Remove(DomainAndEvent);
}

bool FGESHandler::HasEvent(const FString& Domain, const FString& Event)
{
	return EventMap.Contains(Key(Domain, Event));
}

void FGESHandler::UnpinEvent(const FString& Domain, const FString& EventName)
{
	FString KeyString = Key(Domain, EventName);
	if (EventMap.Contains(KeyString))
	{
		FGESEvent& Event = EventMap[KeyString];
		Event.bPinned = false;
		//Event.PinnedData.Property->RemoveFromRoot();
		//Event.PinnedData.PropertyData.Empty();  not sure if safe to delete instead of rebuilding on next pin
	}
}

void FGESHandler::AddListener(const FString& Domain, const FString& EventName, const FGESEventListener& Listener)
{
	FString KeyString = Key(Domain, EventName);

	//Create event if not already created
	if (!EventMap.Contains(KeyString))
	{
		CreateEvent(Domain, EventName);
	}

	//Check passed listener validity
	if (Listener.IsValidListener())
	{
		//Actually add this valid listener to map
		FGESEvent& Event = EventMap[KeyString];
		Event.Listeners.Add(Listener);

		//if it's pinned re-emit it immediately to this listener
		if (Event.bPinned) 
		{
			FGESFullEmitData EmitData;
			
			EmitData.Domain = Domain;
			EmitData.Event = EventName;

			EmitData.Property = Event.PinnedData.Property;
			EmitData.PropertyPtr = Event.PinnedData.PropertyPtr;
			EmitData.bPinned = Event.bPinned;
			EmitData.SpecificTarget = (FGESEventListener*)&Listener;	//this immediate call should only be calling our listener
			EmitData.WorldContext = Event.WorldContext;
			
			//did we fail to emit?
			if (!EmitEvent(EmitData))
			{
				//did the event get removed due to being stale? The listener may still be valid so re-run the listener loop
				if (!HasEvent(Domain, EventName))
				{
					AddListener(Domain, EventName, Listener);
				}
			}
		}
	}
	else
	{
		//NB: validity can be violated due to delegate and lambda too
		//TODO: add warnings in case of invalid delegate/lambda function binds

		//Not valid, emit warnings
		if (Listener.Receiver->IsValidLowLevelFast())
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener Warning: \n%s does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.Receiver->GetFullName(), *Listener.FunctionName, *Domain, *EventName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener: (invalid object) does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.FunctionName, *Domain, *EventName);
		}
	}
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda)
{
	FGESEventListener Listener;
	Listener.bIsBoundToLambda = true;
	Listener.LambdaFunction = ReceivingLambda;
	Listener.Receiver = BindInfo.WorldContext;

	//name is derived from WCO + lambda pointer address
	FString FunctionPtr = FString::Printf(TEXT("%d"), (void*)&ReceivingLambda);
	Listener.FunctionName = Listener.Receiver->GetName() + TEXT(".lambda.") + FunctionPtr;

	AddListener(BindInfo.Domain, BindInfo.Event, Listener);

	return Listener.FunctionName;
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(UStruct* Struct, void* StructPtr)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			FStructProperty* StructProperty = CastField<FStructProperty>(Data.Property.Get());

			if (!StructProperty)
			{
				UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddLambdaListener callback: Expected a property structure, received %s; Receive skipped."), *Data.Property->GetName());
				return;
			}

			ReceivingLambda(StructProperty->Struct, Data.PropertyPtr);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(const FString&)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			FString Value;
			UGlobalEventSystemBPLibrary::Conv_PropToStringRef(Data, Value);
			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(UObject*)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			UObject* Value = 0;
			UGlobalEventSystemBPLibrary::Conv_PropToObject(Data, Value);
			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(float)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			float Value = 0;
			UGlobalEventSystemBPLibrary::Conv_PropToFloat(Data, Value);
			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(int32)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			int32 Value = 0;
			UGlobalEventSystemBPLibrary::Conv_PropToInt(Data, Value);

			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(bool)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			bool Value = false;
			UGlobalEventSystemBPLibrary::Conv_PropToBool(Data, Value);
			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(const FName&)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			FName Value;
			UGlobalEventSystemBPLibrary::Conv_PropToName(Data, Value);
			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListener(FGESEventContext BindInfo, TFunction<void(void)> ReceivingLambda)
{
	return AddLambdaListener(BindInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			ReceivingLambda();
		});
}

void FGESHandler::RemoveListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener)
{
	FString KeyString = Key(Domain, Event);
	if (!EventMap.Contains(KeyString))
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::RemoveListener, tried to remove a listener from an event that doesn't exist. Ignored."));
		return;
	}
	EventMap[KeyString].Listeners.Remove(Listener);
}

void FGESHandler::RemoveLambdaListener(FGESEventContext BindInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda)
{
	FGESEventListener Listener;
	Listener.bIsBoundToLambda = true;
	Listener.LambdaFunction = ReceivingLambda;
	Listener.Receiver = BindInfo.WorldContext;

	FString FunctionPtr = FString::Printf(TEXT("%d"), (void*)&ReceivingLambda);
	Listener.FunctionName = Listener.Receiver->GetName() + TEXT(".lambda.") + FunctionPtr;

	RemoveListener(BindInfo.Domain, BindInfo.Event, Listener);
}

void FGESHandler::RemoveLambdaListener(FGESEventContext BindInfo, const FString& LambdaName)
{
	FGESEventListener Listener;
	Listener.bIsBoundToLambda = true;
	Listener.Receiver = BindInfo.WorldContext;
	Listener.FunctionName = LambdaName;

	RemoveListener(BindInfo.Domain, BindInfo.Event, Listener);
}

void FGESHandler::EmitToListenersWithData(const FGESFullEmitData& EmitData, TFunction<void(const FGESEventListener&)> DataFillCallback)
{
	FString KeyString = Key(EmitData.Domain, EmitData.Event);
	if (!EventMap.Contains(KeyString))
	{
		CreateEvent(EmitData.Domain, EmitData.Event, false);
	}
	FGESEvent& Event = EventMap[KeyString];
	Event.WorldContext = EmitData.WorldContext;

	if (EmitData.WorldContext == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("FGESHandler::EmitToListenersWithData: Emitted event has no world context!"));
		return;
	}

	UWorld* World = EmitData.WorldContext->GetWorld();

	//Attach a world listener to each unique world
	if (!WorldMap.Contains(World))
	{
		AGESWorldListenerActor* WorldListener = World->SpawnActor<AGESWorldListenerActor>();
		WorldListener->OnEndPlay = [this, WorldListener, World]
		{
			for (const FString& EventKey : WorldListener->WorldEvents)
			{
				DeleteEvent(EventKey);
			}
			WorldListener->WorldEvents.Empty();
			WorldMap.Remove(World);
		};
		WorldMap.Add(World, WorldListener);
	}

	//ensure this event is registered
	WorldMap[World]->WorldEvents.Add(KeyString);

	//is there a property to pin?
	if (EmitData.Property)
	{
		//Warn if we're trying to pin a new event without unpinning old one
		if (EmitData.bPinned && EmitData.bPinned)
		{
			//cleanup if different
			if (EmitData.Property != Event.PinnedData.Property ||
				EmitData.PropertyPtr != Event.PinnedData.PropertyPtr)
			{
				Event.PinnedData.CleanupPinnedData();
			}
			Event.PinnedData.Property = EmitData.Property;
			Event.PinnedData.PropertyPtr = EmitData.PropertyPtr;
			Event.PinnedData.CopyPropertyToPinnedBuffer();
			//UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitToListenersWithData Emitted a pinned event to an already pinned event. Pinned data updated."));
		}
		if (!EmitData.bPinned && EmitData.bPinned)
		{
			Event.PinnedData.CleanupPinnedData();
			Event.PinnedData.Property = EmitData.Property;
			Event.PinnedData.PropertyPtr = EmitData.PropertyPtr;
			Event.PinnedData.CopyPropertyToPinnedBuffer();
		}
	}
	Event.bPinned = EmitData.bPinned;


	//only emit to this target
	if (EmitData.SpecificTarget)
	{
		FGESEventListener Listener = *EmitData.SpecificTarget;

		//Check validity of receiver and function and call the function
		if (Listener.Receiver->IsValidLowLevelFast())
		{
			if (Listener.bIsBoundToLambda && Listener.LambdaFunction != nullptr)
			{
				//this listener is handled by lambda instead
				FGESWildcardProperty Wrapper;
				Wrapper.Property = EmitData.Property;
				Wrapper.PropertyPtr = EmitData.PropertyPtr;

				Listener.LambdaFunction(Wrapper);
				return;
			}
			if (Listener.bIsBoundToDelegate)
			{
				//this listener is handled by wildcard event delegate instead
				FGESWildcardProperty Wrapper;
				Wrapper.Property = EmitData.Property;
				Wrapper.PropertyPtr = EmitData.PropertyPtr;
				Listener.OnePropertyFunctionDelegate.ExecuteIfBound(Wrapper);
				return;
			}

			UFunction* BPFunction = Listener.Receiver->FindFunction(FName(*Listener.FunctionName));
			if (BPFunction != nullptr)
			{
				DataFillCallback(Listener);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent: Function not found '%s'"), *Listener.FunctionName);
			}
		}
		else
		{
			//stale listener, remove it
			RemovalArray.Add(&Listener);
		}
	}
	//emit to all targets
	else
	{
		for (FGESEventListener& Listener : Event.Listeners)
		{
			//Check validity of receiver and function and call the function
			if (Listener.Receiver->IsValidLowLevelFast())
			{
				if (Listener.bIsBoundToLambda && Listener.LambdaFunction != nullptr)
				{
					//this listener is handled by lambda instead
					FGESWildcardProperty Wrapper;
					Wrapper.Property = EmitData.Property;
					Wrapper.PropertyPtr = EmitData.PropertyPtr;

					Listener.LambdaFunction(Wrapper);
					continue;
				}
				if (Listener.bIsBoundToDelegate)
				{
					//this listener is handled by wildcard event delegate instead
					FGESWildcardProperty Wrapper;
					Wrapper.Property = EmitData.Property;
					Wrapper.PropertyPtr = EmitData.PropertyPtr;
					Listener.OnePropertyFunctionDelegate.ExecuteIfBound(Wrapper);
					continue;
				}

				UFunction* BPFunction = Listener.Receiver->FindFunction(FName(*Listener.FunctionName));
				if (BPFunction != nullptr)
				{
					DataFillCallback(Listener);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent: Function not found '%s'"), *Listener.FunctionName);
				}
			}
			else
			{
				//stale listener, remove it
				RemovalArray.Add(&Listener);
			}
		}
	}

	//Go through stale listeners and remove them
	if (RemovalArray.Num() > 0)
	{
		for (int i = 0; i < RemovalArray.Num(); i++)
		{
			FGESEventListener Listener = *RemovalArray[i];
			Event.Listeners.Remove(Listener);
		}
		if (Options.bLogStaleRemovals)
		{
			UE_LOG(LogTemp, Log, TEXT("FGESHandler::EmitEvent: auto-removed %d stale listeners."), RemovalArray.Num());
		}
		RemovalArray.Empty();
	}
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, UStruct* Struct, void* StructPtr)
{
	bool bValidateStructs = Options.bValidateStructTypes;
	FGESFullEmitData FullEmit(EmitData);
	//TODO: fill

	EmitToListenersWithData(EmitData, [&EmitData, Struct, StructPtr, bValidateStructs](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FStructProperty::StaticClass(), EmitData, Listener))
		{
			if (bValidateStructs)
			{
				//For structs we can have different mismatching structs at this point check class types
				//optimization note: unroll the above function for structs to avoid double param lookup
				TArray<FProperty*> Properties;
				FunctionParameters(Listener.Function, Properties);
				FStructProperty* StructProperty = CastField<FStructProperty>(Properties[0]);
				if (StructProperty->Struct == Struct)
				{
					Listener.Receiver->ProcessEvent(Listener.Function, StructPtr);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent %s skipped listener %s due to function not having a matching Struct type %s signature."),
						*EmitEventLogString(EmitData),
						*ListenerLogString(Listener),
						*Struct->GetName());
				}
			}
			//No validation, e.g. vector-> rotator fill is accepted
			else
			{
				Listener.Receiver->ProcessEvent(Listener.Function, StructPtr);
			}
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, const FString& ParamData)
{
	FString MutableParamString = ParamData;
	EmitToListenersWithData(EmitData, [&EmitData, &MutableParamString](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FStrProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &MutableParamString);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, UObject* ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FObjectProperty::StaticClass(), EmitData, Listener))
		{
			FGESDynamicArg ParamWrapper;
			ParamWrapper.Arg01 = ParamData;
			Listener.Receiver->ProcessEvent(Listener.Function, &ParamWrapper);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, float ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, &ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FNumericProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, int32 ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, &ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FNumericProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, bool ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, &ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FBoolProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, const FName& ParamData)
{
	FName MutableName = ParamData;
	EmitToListenersWithData(EmitData, [&EmitData, &ParamData, &MutableName](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FNameProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &MutableName);
		}
	});
}

bool FGESHandler::EmitEvent(const FGESEmitData& EmitData)
{
	FGESFullEmitData FullEmitData;
	FullEmitData = EmitData;
	//todo: process event into ptrs

	return EmitProcessedEvent(FullEmitData);
}

bool FGESHandler::EmitProcessedEvent(const FGESFullEmitData& EmitData)
{
	if (EmitData.WorldContext && !EmitData.WorldContext->IsValidLowLevelFast())
	{
		//Remove this event, it's emit context is invalid
		DeleteEvent(EmitData.Domain, EmitData.Event);
		if (Options.bLogStaleRemovals)
		{
			UE_LOG(LogTemp, Log, TEXT("FGESHandler::EmitEvent stale event removed due to invalid world context. (Usually due to pinned events that haven't been unpinned."));
		}
		return false;
	}
	FProperty* ParameterProp = EmitData.Property;
	void* PropPtr = EmitData.PropertyPtr;

	//no params specified
	if (ParameterProp == nullptr)
	{
		EmitToListenersWithData(EmitData, [&EmitData](const FGESEventListener& Listener)
			{
				//C++ lambda case
				if (Listener.bIsBoundToLambda && Listener.LambdaFunction != nullptr)
				{
					FGESWildcardProperty Wrapper;
					Wrapper.Property = EmitData.Property;
					Wrapper.PropertyPtr = EmitData.PropertyPtr;

					Listener.LambdaFunction(Wrapper);
					return;
				}
				//If the listener bound it to a wildcard event delegate, emit with nullptr
				if (Listener.bIsBoundToDelegate)
				{
					FGESWildcardProperty Wrapper;
					Wrapper.Property = EmitData.Property;
					Wrapper.PropertyPtr = EmitData.PropertyPtr;
					Listener.OnePropertyFunctionDelegate.ExecuteIfBound(Wrapper);
					return;
				}

				//Neither lambda nor wildcard delegate, process no param prop
				TFieldIterator<FProperty> Iterator(Listener.Function);

				TArray<FProperty*> Properties;
				while (Iterator && (Iterator->PropertyFlags & CPF_Parm))
				{
					FProperty* Prop = *Iterator;
					Properties.Add(Prop);
					++Iterator;
				}
				if (Properties.Num() == 0)
				{
					Listener.Receiver->ProcessEvent(Listener.Function, nullptr);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent %s tried to emit an empty event to %s receiver expecting parameters."),
						*EmitData.Event,
						*Listener.Receiver->GetName());
				}
			});
	}
	else if (ParameterProp->IsA<FStructProperty>())
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(ParameterProp);
		EmitEvent(EmitData, StructProperty->Struct, PropPtr);
		return true;
	}
	else if (ParameterProp->IsA<FStrProperty>())
	{
		FStrProperty* StrProperty = CastField<FStrProperty>(ParameterProp);
		FString Data = StrProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else if (ParameterProp->IsA<FObjectProperty>())
	{
		FObjectProperty* ObjectProperty = CastField<FObjectProperty>(ParameterProp);
		UObject* Data = ObjectProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else if (ParameterProp->IsA<FNumericProperty>())
	{
		FNumericProperty* NumericProperty = CastField<FNumericProperty>(ParameterProp);
		if (NumericProperty->IsFloatingPoint())
		{
			double Data = NumericProperty->GetFloatingPointPropertyValue(PropPtr);
			EmitEvent(EmitData, (float)Data);
			return true;
		}
		else
		{
			int64 Data = NumericProperty->GetSignedIntPropertyValue(PropPtr);
			EmitEvent(EmitData, (int32)Data);
			return true;
		}
	}
	else if (ParameterProp->IsA<FBoolProperty>())
	{
		FBoolProperty* BoolProperty = CastField<FBoolProperty>(ParameterProp);
		bool Data = BoolProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else if (ParameterProp->IsA<FNameProperty>())
	{
		FNameProperty* NameProperty = CastField<FNameProperty>(ParameterProp);
		FName Data = NameProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent Unsupported parameter"));
		return false;
	}
	return false;
}

void FGESHandler::SetOptions(const FGESGlobalOptions& InOptions)
{
	Options = InOptions;
}

FString FGESHandler::Key(const FString& Domain, const FString& Event)
{
	return Domain + TEXT(".") + Event;
}

FGESHandler::FGESHandler()
{

}

FGESHandler::~FGESHandler()
{
	//Practically not needed due to shutdown happening on program exit
	/*for (TPair<FString, FGESEvent> Pair : FunctionMap)
	{
		if (Pair.Value.bPinned)
		{
			Pair.Value.PinnedData.CleanupPinnedData();
		}
	}*/
	EventMap.Empty();
}
