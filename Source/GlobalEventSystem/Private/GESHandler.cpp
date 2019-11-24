#include "GESHandler.h"

TSharedPtr<FGESHandler> FGESHandler::PrivateDefaultHandler = MakeShareable(new FGESHandler());


TSharedPtr<FGESHandler> FGESHandler::DefaultHandler()
{
	/*if (!FGESHandler::PrivateDefaultHandler.IsValid())
	{
		FGESHandler::PrivateDefaultHandler = MakeShareable(new FGESHandler());
	}*/
	return FGESHandler::PrivateDefaultHandler;
}

void FGESHandler::CreateEvent(const FString& Domain, const FString& Event, bool bPinned /*= false*/)
{
	FGESEvent CreatedFunction;
	CreatedFunction.Domain = Domain;
	CreatedFunction.Event = Event;
	CreatedFunction.bPinned = bPinned;
	FunctionMap.Add(Key(Domain, Event), CreatedFunction);
}

void FGESHandler::DeleteEvent(const FString& Domain, const FString& Event)
{
	FunctionMap.Remove(Key(Domain, Event));
}

void FGESHandler::UnpinEvent(const FString& Domain, const FString& EventName)
{
	FString KeyString = Key(Domain, EventName);
	if (FunctionMap.Contains(KeyString))
	{
		FGESEvent& Event = FunctionMap[KeyString];
		Event.bPinned = false;
		Event.PinnedData.Property->RemoveFromRoot();
		//Event.PinnedData.PropertyData.Empty();
	}
}

void FGESHandler::AddListener(const FString& Domain, const FString& EventName, const FGESEventListener& Listener)
{
	FString KeyString = Key(Domain, EventName);
	if (!FunctionMap.Contains(KeyString))
	{
		CreateEvent(Domain, EventName);
	}
	if (Listener.IsValidListener())
	{
		FGESEvent& Event = FunctionMap[KeyString];
		Event.Listeners.Add(Listener);

		//if it's pinned re-emit it immediately to this listener
		if (Event.bPinned) 
		{
			FGESEmitData EmitData;
			
			EmitData.Domain = Domain;
			EmitData.Event = EventName;

			EmitData.Property = Event.PinnedData.Property;
			EmitData.PropertyPtr = Event.PinnedData.PropertyPtr;
			EmitData.bPinned = Event.bPinned;
			EmitData.SpecificTarget = (FGESEventListener*)&Listener;	//this immediate call should only be calling our listener
			
			EmitEvent(EmitData);
		}
	}
	else
	{
		if (Listener.Receiver->IsValidLowLevel())
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener Warning: \n%s does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.Receiver->GetFullName(), *Listener.FunctionName, *Domain, *EventName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener: (invalid object) does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.FunctionName, *Domain, *EventName);
		}
	}
}

void FGESHandler::RemoveListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener)
{
	FString KeyString = Key(Domain, Event);
	if (!FunctionMap.Contains(KeyString))
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::RemoveListener, tried to remove a listener from an event that doesn't exist. Ignored."));
		return;
	}
	FunctionMap[KeyString].Listeners.Remove(Listener);
}

void FGESHandler::EmitToListenersWithData(const FGESEmitData& EmitData, TFunction<void(const FGESEventListener&)> DataFillCallback)
{
	FString KeyString = Key(EmitData.Domain, EmitData.Event);
	if (!FunctionMap.Contains(KeyString))
	{
		CreateEvent(EmitData.Domain, EmitData.Event, false);
	}
	FGESEvent& Event = FunctionMap[KeyString];

	//is there a property to pin?
	if (EmitData.Property)
	{
		//Warn if we're trying to pin a new event without unpinning old one
		if (Event.bPinned && EmitData.bPinned)
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
		if (!Event.bPinned && EmitData.bPinned)
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
		if (Listener.Receiver->IsValidLowLevel())
		{
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
			if (Listener.Receiver->IsValidLowLevel())
			{
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
		UE_LOG(LogTemp, Log, TEXT("FGESHandler::EmitEvent: auto-removed %d stale listeners."), RemovalArray.Num());
		RemovalArray.Empty();
	}
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, UStruct* Struct, void* StructPtr)
{
	EmitToListenersWithData(EmitData, [Struct, StructPtr](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, StructPtr);
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, const FString& ParamData)
{
	FString MutableParamString = ParamData;
	EmitToListenersWithData(EmitData, [&MutableParamString](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &MutableParamString);
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, float ParamData)
{
	EmitToListenersWithData(EmitData, [&ParamData](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, int32 ParamData)
{
	EmitToListenersWithData(EmitData, [&ParamData](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, bool ParamData)
{
	EmitToListenersWithData(EmitData, [&ParamData](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData)
{
	UProperty* ParameterProp = EmitData.Property;
	void* PropPtr = EmitData.PropertyPtr;

	//no params specified
	if (ParameterProp == nullptr)
	{
		EmitToListenersWithData(EmitData, [&EmitData](const FGESEventListener& Listener)
		{
			TFieldIterator<UProperty> Iterator(Listener.Function);

			TArray<UProperty*> Properties;
			while (Iterator && (Iterator->PropertyFlags & CPF_Parm))
			{
				UProperty* Prop = *Iterator;
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
					*Listener.Receiver->GetFullName());
			}
		});
	}
	else if (ParameterProp->IsA<UStructProperty>())
	{
		UStructProperty* StructProperty = ExactCast<UStructProperty>(ParameterProp);
		EmitEvent(EmitData, StructProperty->Struct, PropPtr);
		return;
	}
	else if (ParameterProp->IsA<UStrProperty>())
	{
		UStrProperty* StrProperty = Cast<UStrProperty>(ParameterProp);
		FString Data = StrProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return;
	}
	else if (ParameterProp->IsA<UNumericProperty>())
	{
		UNumericProperty* NumericProperty = Cast<UNumericProperty>(ParameterProp);
		if (NumericProperty->IsFloatingPoint())
		{
			double Data = NumericProperty->GetFloatingPointPropertyValue(PropPtr);
			EmitEvent(EmitData, (float)Data);
			return;
		}
		else
		{
			int64 Data = NumericProperty->GetSignedIntPropertyValue(PropPtr);
			EmitEvent(EmitData, (int32)Data);
			return;
		}
	}
	else if (ParameterProp->IsA<UBoolProperty>())
	{
		UBoolProperty* BoolProperty = Cast<UBoolProperty>(ParameterProp);
		bool Data = BoolProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent Unsupported parameter"));
	}
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
	//todo: maybe cleanup via emitting shutdown events?
	/*for (TPair<FString, FGESEvent> Pair : FunctionMap)
	{
		if (Pair.Value.bPinned)
		{
			Pair.Value.PinnedData.CleanupPinnedData();
		}
	}*/
	FunctionMap.Empty();
}

void FGESPinnedData::CopyPropertyToPinnedBuffer()
{
	//Copy this property data to temp
	Property->AddToRoot();
	int32 Num = Property->GetSize();
	PropertyData.SetNumUninitialized(Num);
	FMemory::Memcpy(PropertyData.GetData(), PropertyPtr, Num);

	//reset pointer to new copy
	PropertyPtr = PropertyData.GetData();
}

void FGESPinnedData::CleanupPinnedData()
{
	if (Property != nullptr && Property->IsValidLowLevel())
	{
		Property->RemoveFromRoot();
	}
	PropertyData.Empty();
	Property = nullptr;
	PropertyPtr = nullptr;
}
