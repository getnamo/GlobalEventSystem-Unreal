#include "GESHandler.h"
#include "Engine/World.h"

TSharedPtr<FGESHandler> FGESHandler::PrivateDefaultHandler = MakeShareable(new FGESHandler());


bool FGESHandler::FirstParamIsCppType(UFunction* Function, const FString& TypeString)
{
	TArray<UProperty*> Properties;
	FunctionParameters(Function, Properties);
	if (Properties.Num() == 0)
	{
		return false;
	}

	const FString& FirstParam = Properties[0]->GetCPPType();
	return (FirstParam == TypeString);
}

bool FGESHandler::FirstParamIsSubclassOf(UFunction* Function, UClass* ClassType)
{
	TArray<UProperty*> Properties;
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

void FGESHandler::FunctionParameters(UFunction* Function, TArray<UProperty*>& OutParamProperties)
{
	TFieldIterator<UProperty> Iterator(Function);

	while (Iterator && (Iterator->PropertyFlags & CPF_Parm))
	{
		UProperty* Prop = *Iterator;
		OutParamProperties.Add(Prop);
		++Iterator;
	}
}

bool FGESHandler::FunctionHasValidParams(UFunction* Function, UClass* ClassType, const FGESEmitData& EmitData, const FGESEventListener& Listener)
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

//From IsValidLowLevelFast, but without logs
bool FGESHandler::IsValidLowLevelNoSpam(UObject* ObjectPtr)
{
	// As DEFAULT_ALIGNMENT is defined to 0 now, I changed that to the original numerical value here
	const int32 AlignmentCheck = MIN_ALIGNMENT - 1;

	// Check 'this' pointer before trying to access any of the Object's members
	if ((ObjectPtr == nullptr) || (UPTRINT)ObjectPtr < 0x100)
	{
		return false;
	}
	if ((UPTRINT)ObjectPtr & AlignmentCheck)
	{
		return false;
	}
	if (*(void**)ObjectPtr == nullptr)
	{
		return false;
	}

	// These should all be 0.
	const UPTRINT CheckZero = (ObjectPtr->GetFlags() & ~RF_AllFlags) | ((UPTRINT)ObjectPtr->GetClass() & AlignmentCheck) | ((UPTRINT)ObjectPtr->GetOuter() & AlignmentCheck);
	if (!!CheckZero)
	{
		return false;
	}
	// These should all be non-NULL (except CDO-alignment check which should be 0)
	if (ObjectPtr->GetClass() == nullptr || ObjectPtr->GetClass()->ClassDefaultObject == nullptr || ((UPTRINT)ObjectPtr->GetClass()->ClassDefaultObject & AlignmentCheck) != 0)
	{
		return false;
	}
	// Lightweight versions of index checks.
	if (!GUObjectArray.IsValidIndex(ObjectPtr) || !ObjectPtr->GetFName().IsValidIndexFast())
	{
		return false;
	}
	return true;
}

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

bool FGESHandler::HasEvent(const FString& Domain, const FString& Event)
{
	return FunctionMap.Contains(Key(Domain, Event));
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
		if (IsValidLowLevelNoSpam(Listener.Receiver))
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
	Event.WorldContext = EmitData.WorldContext;

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
		if (IsValidLowLevelNoSpam(Listener.Receiver))
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
			if (IsValidLowLevelNoSpam(Listener.Receiver))
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
	EmitToListenersWithData(EmitData, [&EmitData, Struct, StructPtr, bValidateStructs](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, UStructProperty::StaticClass(), EmitData, Listener))
		{
			if (bValidateStructs)
			{
				//For structs we can have different mismatching structs at this point check class types
				//optimization note: unroll the above function for structs to avoid double param lookup
				TArray<UProperty*> Properties;
				FunctionParameters(Listener.Function, Properties);
				UStructProperty* StructProperty = Cast<UStructProperty>(Properties[0]);
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
		if (FunctionHasValidParams(Listener.Function, UStrProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &MutableParamString);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, UObject* ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, UObjectProperty::StaticClass(), EmitData, Listener))
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
		if (FunctionHasValidParams(Listener.Function, UNumericProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, int32 ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, &ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, UNumericProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
		}
	});
}

void FGESHandler::EmitEvent(const FGESEmitData& EmitData, bool ParamData)
{
	EmitToListenersWithData(EmitData, [&EmitData, &ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, UBoolProperty::StaticClass(), EmitData, Listener))
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
		if (FunctionHasValidParams(Listener.Function, UNameProperty::StaticClass(), EmitData, Listener))
		{
			Listener.Receiver->ProcessEvent(Listener.Function, &MutableName);
		}
	});
}

bool FGESHandler::EmitEvent(const FGESEmitData& EmitData)
{
	if ( !IsValidLowLevelNoSpam(EmitData.WorldContext) ) //|| !IsValidLowLevelNoSpam(EmitData.WorldContext->GetWorld())
	{
		//Remove this event, it's emit context is invalid
		DeleteEvent(EmitData.Domain, EmitData.Event);
		if (Options.bLogStaleRemovals)
		{
			UE_LOG(LogTemp, Log, TEXT("FGESHandler::EmitEvent stale event removed due to invalid world context. (Usually due to pinned events that haven't been unpinned."));
		}
		return false;
	}
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
					*Listener.Receiver->GetName());
			}
		});
	}
	else if (ParameterProp->IsA<UStructProperty>())
	{
		UStructProperty* StructProperty = ExactCast<UStructProperty>(ParameterProp);
		EmitEvent(EmitData, StructProperty->Struct, PropPtr);
		return true;
	}
	else if (ParameterProp->IsA<UStrProperty>())
	{
		UStrProperty* StrProperty = Cast<UStrProperty>(ParameterProp);
		FString Data = StrProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else if (ParameterProp->IsA<UObjectProperty>())
	{
		UObjectProperty* ObjectProperty = Cast<UObjectProperty>(ParameterProp);
		UObject* Data = ObjectProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else if (ParameterProp->IsA<UNumericProperty>())
	{
		UNumericProperty* NumericProperty = Cast<UNumericProperty>(ParameterProp);
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
	else if (ParameterProp->IsA<UBoolProperty>())
	{
		UBoolProperty* BoolProperty = Cast<UBoolProperty>(ParameterProp);
		bool Data = BoolProperty->GetPropertyValue(PropPtr);
		EmitEvent(EmitData, Data);
		return true;
	}
	else if (ParameterProp->IsA<UNameProperty>())
	{
		UNameProperty* NameProperty = Cast<UNameProperty>(ParameterProp);
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

void FGESHandler::SetOptions(const FGESHandlerOptions& InOptions)
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
	if (FGESHandler::IsValidLowLevelNoSpam(Property))
	{
		Property->RemoveFromRoot();
	}
	PropertyData.Empty();
	Property = nullptr;
	PropertyPtr = nullptr;
}
