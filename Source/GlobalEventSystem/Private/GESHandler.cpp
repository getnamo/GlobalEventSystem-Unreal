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

void FGESHandler::CreateEvent(const FString& TargetDomain, const FString& TargetFunction, bool bPinned /*=false*/)
{
	FGESEvent CreatedFunction;
	CreatedFunction.TargetDomain = TargetDomain;
	CreatedFunction.TargetFunction = TargetFunction;
	CreatedFunction.bPinned = bPinned;
	FunctionMap.Add(Key(TargetDomain, TargetFunction), CreatedFunction);
}

void FGESHandler::DeleteEvent(const FString& TargetDomain, const FString& TargetFunction)
{
	FunctionMap.Remove(Key(TargetDomain, TargetFunction));
}

void FGESHandler::UnpinEvent(const FString& TargetDomain, const FString& TargetFunction)
{
	FString KeyString = Key(TargetDomain, TargetFunction);
	if (FunctionMap.Contains(KeyString))
	{
		FGESEvent& Event = FunctionMap[KeyString];
		Event.bPinned = false;
		Event.PinnedData.Property->RemoveFromRoot();
		//Event.PinnedData.PropertyData.Empty();
	}
}

void FGESHandler::AddListener(const FString& TargetDomain, const FString& TargetFunction, const FGESEventListener& Listener)
{
	FString KeyString = Key(TargetDomain, TargetFunction);
	if (!FunctionMap.Contains(KeyString))
	{
		CreateEvent(TargetDomain, TargetFunction);
	}
	if (Listener.IsValidListener())
	{
		FGESEvent& Event = FunctionMap[KeyString];
		Event.Listeners.Add(Listener);

		//if it's pinned re-emit it immediately to this listener
		if (Event.bPinned) 
		{
			FGESEmitData EmitData;
			
			EmitData.TargetDomain = TargetDomain;
			EmitData.TargetFunction = TargetFunction;

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
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener Warning: \n%s does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.Receiver->GetFullName(), *Listener.FunctionName, *TargetDomain, *TargetFunction);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener: (invalid object) does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.FunctionName, *TargetDomain, *TargetFunction);
		}
	}
}

void FGESHandler::RemoveListener(const FString& TargetDomain, const FString& TargetFunction, const FGESEventListener& Listener)
{
	FString KeyString = Key(TargetDomain, TargetFunction);
	if (!FunctionMap.Contains(KeyString))
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::RemoveListener, tried to remove a listener from an event that doesn't exist. Ignored."));
		return;
	}
	FunctionMap[KeyString].Listeners.Remove(Listener);
}

void FGESHandler::EmitToListenersWithData(const FGESEmitData& EmitData, TFunction<void(const FGESEventListener&)> DataFillCallback)
{
	FString KeyString = Key(EmitData.TargetDomain, EmitData.TargetFunction);
	if (!FunctionMap.Contains(KeyString))
	{
		CreateEvent(EmitData.TargetDomain, EmitData.TargetFunction, false);
	}
	FGESEvent& Event = FunctionMap[KeyString];
	if (!Event.bPinned && EmitData.bPinned)
	{
		Event.bPinned = EmitData.bPinned;
		Event.PinnedData.Property = EmitData.Property;
		Event.PinnedData.PropertyPtr = EmitData.PropertyPtr;
		Event.PinnedData.CopyPropertyToPinnedBuffer();
	}

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

	if (ParameterProp->IsA<UStructProperty>())
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
}

FString FGESHandler::Key(const FString& TargetDomain, const FString& TargetFunction)
{
	return TargetDomain + TEXT(".") + TargetFunction;
}

FGESHandler::FGESHandler()
{

}

FGESHandler::~FGESHandler()
{
	//todo: maybe cleanup via emitting shutdown events?
	FunctionMap.Empty();
}

void FGESPinnedData::CopyPropertyToPinnedBuffer()
{
	//Copy this property data to temp
	int32 Num = Property->GetSize();
	PropertyData.SetNumUninitialized(Num);
	FMemory::Memcpy(PropertyData.GetData(), PropertyPtr, Num);

	//reset pointer to new copy
	PropertyPtr = PropertyData.GetData();
}
