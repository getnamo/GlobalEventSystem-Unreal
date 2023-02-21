#include "GESHandler.h"
#include "GlobalEventSystemBPLibrary.h"
#include "Engine/World.h"

TSharedPtr<FGESHandler> FGESHandler::PrivateDefaultHandler = MakeShareable(new FGESHandler());

void FGESHandler::Clear()
{
	PrivateDefaultHandler = MakeShareable(new FGESHandler());
}

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
	return Listener.ReceiverWCO->GetName() + TEXT(":") + Listener.FunctionName;
}

FString FGESHandler::EventLogString(const FGESEvent& Event)
{
	return Event.Domain + TEXT(".") + Event.Event;
}

FString FGESHandler::EmitEventLogString(const FGESEmitContext& EmitData)
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

bool FGESHandler::FunctionHasValidParams(UFunction* Function, FFieldClass* ClassType, const FGESEmitContext& EmitData, const FGESEventListener& Listener)
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
	DeleteEvent(Key(Domain, Event));
}

void FGESHandler::DeleteEvent(const FString& DomainAndEvent)
{
	//ensure any pinned data gets cleaned up on event deletion
	if (EventMap.Contains(DomainAndEvent))
	{
		FGESEvent& Event = EventMap[DomainAndEvent];
		if (Event.bPinned)
		{
			Event.PinnedData.CleanupPinnedData();
		}
	}

	//remove the event
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

		//TODO: check receivermap logic
		FGESEventListenerWithContext ListenContext;
		ListenContext.Domain = Domain;
		ListenContext.Event = EventName;
		FGESMinimalEventListener Minimal;
		Minimal.FunctionName = Listener.FunctionName;
		Minimal.ReceiverWCO = Listener.ReceiverWCO;
		ListenContext.Listener = Minimal;

		if (!ReceiverMap.Contains(Listener.ReceiverWCO))
		{
			TArray<FGESEventListenerWithContext> Array;
			ReceiverMap.Add(Listener.ReceiverWCO, Array);
		}
		ReceiverMap[Listener.ReceiverWCO].Add(ListenContext);

		//if it's pinned re-emit it immediately to this listener
		if (Event.bPinned) 
		{
			FGESPropertyEmitContext EmitData;
			
			EmitData.Domain = Domain;
			EmitData.Event = EventName;

			EmitData.Property = Event.PinnedData.Property;
			EmitData.PropertyPtr = Event.PinnedData.PropertyPtr;
			EmitData.bPinned = Event.bPinned;
			EmitData.SpecificTarget = (FGESEventListener*)&Listener;	//this immediate call should only be calling our listener
			EmitData.WorldContext = Event.WorldContext;
			
			//did we fail to emit?
			if (!EmitPropertyEvent(EmitData))
			{
				//did the event get removed due to being stale? The listener may still be valid so re-run this add listener loop
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
		if (Listener.ReceiverWCO->IsValidLowLevelFast())
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener Warning: \n%s does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.ReceiverWCO->GetFullName(), *Listener.FunctionName, *Domain, *EventName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddListener: (invalid object) does not have the function '%s'. Attempted to bind to GESEvent %s.%s"), *Listener.FunctionName, *Domain, *EventName);
		}
	}
}

FString FGESHandler::AddLambdaListener(FGESEventContext Context, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda)
{
	if (Context.WorldContext == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::AddLambdaListener No valid world context provided. Not added."));
		return TEXT("Invalid");
	}
	FGESEventListener Listener;
	Listener.bIsBoundToLambda = true;
	Listener.LambdaFunction = ReceivingLambda;
	Listener.ReceiverWCO = Context.WorldContext;

	//name is derived from WCO + lambda pointer address
	FString FunctionPtr = FString::Printf(TEXT("%d"), (void*)&ReceivingLambda);
	Listener.FunctionName = Listener.ReceiverWCO->GetName() + TEXT(".lambda.") + FunctionPtr;

	AddListener(Context.Domain, Context.Event, Listener);

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

FString FGESHandler::AddLambdaListenerInt(FGESEventContext EventInfo, TFunction<void(int32)> ReceivingLambda)
{
	return AddLambdaListener(EventInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			int32 Value = 0;
			UGlobalEventSystemBPLibrary::Conv_PropToInt(Data, Value);

			ReceivingLambda(Value);
		});
}

FString FGESHandler::AddLambdaListenerBool(FGESEventContext EventInfo, TFunction<void(bool)> ReceivingLambda)
{
	return AddLambdaListener(EventInfo,
		[ReceivingLambda](const FGESWildcardProperty& Data)
		{
			bool Value = false;
			UGlobalEventSystemBPLibrary::Conv_PropToBool(Data, Value);
			ReceivingLambda(Value);
		});
}

void FGESHandler::RemoveListener(const FString& Domain, const FString& Event, const FGESEventListener& Listener)
{
	FString KeyString = Key(Domain, Event);
	if (!EventMap.Contains(KeyString))
	{
		if (Options.bLogStaleRemovals)
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::RemoveListener, tried to remove a listener from an event that doesn't exist (%s.%s). Ignored."), *Domain, *Event);
		}
		return;
	}

	//Remove from main listener map
	EventMap[KeyString].Listeners.Remove(Listener);

	//Remove matched entry in receiver map
	if (ReceiverMap.Contains(Listener.ReceiverWCO))
	{
		FGESEventListenerWithContext ContextListener;
		ContextListener.Domain = Domain;
		ContextListener.Event = Event;
		ContextListener.Listener.FunctionName = Listener.FunctionName;
		ContextListener.Listener.ReceiverWCO = Listener.ReceiverWCO;
		ReceiverMap[Listener.ReceiverWCO].Remove(ContextListener);
	}
}

void FGESHandler::RemoveAllListenersForReceiver(UObject* ReceiverWCO)
{
	if (!ReceiverMap.Contains(ReceiverWCO))
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::RemoveAllListenersForReceiver, tried to remove listeners from an WCO that doesn't exist. Ignored."));
		return;
	}

	//Copy array so we can loop over
	TArray<FGESEventListenerWithContext> ReceiverArray = ReceiverMap[ReceiverWCO];
	
	for (FGESEventListenerWithContext& ListenContext: ReceiverArray)
	{
		RemoveListener(ListenContext.Domain, ListenContext.Event, FGESEventListener(ListenContext.Listener));
	}

	ReceiverMap.Remove(ReceiverWCO);
}

void FGESHandler::RemoveLambdaListener(FGESEventContext BindInfo, TFunction<void(const FGESWildcardProperty&)> ReceivingLambda)
{
	FGESEventListener Listener;
	Listener.bIsBoundToLambda = true;
	Listener.LambdaFunction = ReceivingLambda;
	Listener.ReceiverWCO = BindInfo.WorldContext;

	FString FunctionPtr = FString::Printf(TEXT("%d"), (void*)&ReceivingLambda);
	Listener.FunctionName = Listener.ReceiverWCO->GetName() + TEXT(".lambda.") + FunctionPtr;

	RemoveListener(BindInfo.Domain, BindInfo.Event, Listener);
}

void FGESHandler::RemoveLambdaListener(FGESEventContext BindInfo, const FString& LambdaName)
{
	FGESEventListener Listener;
	Listener.bIsBoundToLambda = true;
	Listener.ReceiverWCO = BindInfo.WorldContext;
	Listener.FunctionName = LambdaName;

	RemoveListener(BindInfo.Domain, BindInfo.Event, Listener);
}

void FGESHandler::EmitToListenersWithData(const FGESPropertyEmitContext& EmitData, TFunction<void(const FGESEventListener&)> DataFillCallback)
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
	if (!World->IsValidLowLevelFast())
	{
		UE_LOG(LogTemp, Error, TEXT("FGESHandler::EmitToListenersWithData: Emitted event has no world!"));
		return;
	}

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

			//For now always clear receiver map if any world ends
			ReceiverMap.Empty();

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
		if (Event.bPinned && EmitData.bPinned)
		{
			//cleanup if different
			if (EmitData.Property != Event.PinnedData.Property ||
				EmitData.PropertyPtr != Event.PinnedData.PropertyPtr)
			{
				Event.PinnedData.CleanupPinnedData();
			}
			
			Event.PinnedData.bHandlePropertyDeletion = EmitData.bHandleAllocation;
			Event.PinnedData.Property = EmitData.Property;

			//only copy if ptrs are different or nullptr
			if (EmitData.PropertyPtr != Event.PinnedData.PropertyPtr || 
				Event.PinnedData.PropertyPtr == nullptr)
			{
				Event.PinnedData.PropertyPtr = EmitData.PropertyPtr;
				Event.PinnedData.CopyPropertyToPinnedBuffer();
			}
		
			//UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitToListenersWithData Emitted a pinned event to an already pinned event. Pinned data updated."));
		}
		if (!Event.bPinned && EmitData.bPinned)
		{
			Event.PinnedData.CleanupPinnedData();
			Event.PinnedData.bHandlePropertyDeletion = EmitData.bHandleAllocation;
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

		//stale listener, remove it
		if (!Listener.ReceiverWCO->IsValidLowLevelFast())
		{
			RemovalArray.Add(&Listener);
		}
		else
		{
			//potential issue: this opt bypasses specialization via datafillcallback
			EmitToListenerWithData(EmitData, Listener, DataFillCallback);
		}
	}
	//emit to all targets
	else
	{
		for (FGESEventListener& Listener : Event.Listeners)
		{
			//stale listener, remove it
			if (!Listener.ReceiverWCO->IsValidLowLevelFast())
			{
				RemovalArray.Add(&Listener);
			}
			else
			{
				//potential issue: this opt bypasses specialization via datafillcallback
				EmitToListenerWithData(EmitData, Listener, DataFillCallback);
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

bool FGESHandler::EmitToListenerWithData(const FGESPropertyEmitContext& EmitData, const FGESEventListener& Listener, TFunction<void(const FGESEventListener&)>& DataFillCallback)
{
	if (Listener.ReceiverWCO->IsValidLowLevelFast())
	{
		if (Listener.bIsBoundToLambda && Listener.LambdaFunction != nullptr)
		{
			//Opt1) this listener is handled by lambda
			FGESWildcardProperty Wrapper;
			Wrapper.Property = EmitData.Property;
			Wrapper.PropertyPtr = EmitData.PropertyPtr;

			Listener.LambdaFunction(Wrapper);
			return true;
		}
		if (Listener.bIsBoundToDelegate)
		{
			//Opt2) this listener is handled by wildcard event delegate
			FGESWildcardProperty Wrapper;
			Wrapper.Property = EmitData.Property;
			Wrapper.PropertyPtr = EmitData.PropertyPtr;
			Listener.OnePropertyFunctionDelegate.ExecuteIfBound(Wrapper);
			return true;
		}

		UFunction* BPFunction = Listener.ReceiverWCO->FindFunction(FName(*Listener.FunctionName));
		if (BPFunction != nullptr)
		{
			//Opt3) listener is handled by function bind by name
			DataFillCallback(Listener);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent: Function not found '%s'"), *Listener.FunctionName);
			return false;
		}
	}
	return false;
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, UStruct* Struct, void* StructPtr)
{
	bool bValidateStructs = Options.bValidateStructTypes;
	FGESPropertyEmitContext PropData(EmitData);
	UClass* Class = EmitData.WorldContext->GetClass();

	FField* OldProperty = Class->ChildProperties;

	FStructProperty* StructProperty = new FStructProperty(FFieldVariant(Class), TEXT("StructProperty"), RF_NoFlags);
	StructProperty->Struct = (UScriptStruct*)Struct;
	StructProperty->ElementSize = Struct->GetStructureSize();

	//undo what we just did so it won't be traversed because of init
	Class->ChildProperties = OldProperty;

	//Store our struct data in a buffer we can reference
	TArray<uint8> Buffer;
	int32 Size = Struct->GetStructureSize();
	Buffer.SetNum(Size);

	//StructProperty->CopyCompleteValue(Buffer.GetData(), StructPtr);
	FPlatformMemory::Memcpy(Buffer.GetData(), StructPtr, Size);

	PropData.Property = StructProperty;
	PropData.PropertyPtr = Buffer.GetData();
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData, &Struct, &Buffer, bValidateStructs](const FGESEventListener& Listener)
	{
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent struct Emit called"));

		if (FunctionHasValidParams(Listener.Function, FStructProperty::StaticClass(), PropData, Listener))
		{
			if (bValidateStructs)
			{
				UE_LOG(LogTemp, Warning, TEXT("Validation emit"));

				//For structs we can have different mismatching structs at this point check class types
				//optimization note: unroll the above function for structs to avoid double param lookup
				TArray<FProperty*> Properties;
				FunctionParameters(Listener.Function, Properties);
				FStructProperty* SubStructProperty = CastField<FStructProperty>(Properties[0]);
				if (SubStructProperty->Struct == Struct)
				{
					Listener.ReceiverWCO->ProcessEvent(Listener.Function, PropData.PropertyPtr);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent %s skipped listener %s due to function not having a matching Struct type %s signature."),
						*EmitEventLogString(PropData),
						*ListenerLogString(Listener),
						*Struct->GetName());
				}
			}
			//No validation, e.g. vector-> rotator fill is accepted
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No validation emit"));
				Listener.ReceiverWCO->ProcessEvent(Listener.Function, (void*)Buffer.GetData()); //PropData.PropertyPtr); //
			}
		}
	});


	if (!EmitData.bPinned)
	{
		delete StructProperty;
	}
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, const FString& ParamData)
{
	FGESPropertyEmitContext PropData(EmitData);

	//We have no property context, make a new property
	FStrProperty* StrProperty = 
		new FStrProperty(FFieldVariant(EmitData.WorldContext->GetClass()),
			TEXT("StringValue"),
			EObjectFlags::RF_Public | EObjectFlags::RF_LoadCompleted);

	//Wrap our FString into a buffer we can share
	TArray<uint8> Buffer;
	Buffer.SetNum(ParamData.GetAllocatedSize());

	StrProperty->SetPropertyValue_InContainer(Buffer.GetData(), ParamData);

	PropData.Property = StrProperty;
	PropData.PropertyPtr = Buffer.GetData();
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData, ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FStrProperty::StaticClass(), PropData, Listener))
		{
			Listener.ReceiverWCO->ProcessEvent(Listener.Function, PropData.PropertyPtr);// (void*)*MutableString); // (void*)&ParamData);
		}
	});

	if (!EmitData.bPinned)
	{
		delete StrProperty;
	}
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, UObject* ParamData)
{
	FGESPropertyEmitContext PropData(EmitData);

	FObjectProperty* ObjectProperty =
		new FObjectProperty(FFieldVariant(EmitData.WorldContext->GetClass()),
			TEXT("ObjectValue"),
			EObjectFlags::RF_Public | EObjectFlags::RF_LoadCompleted);

	//wrapper required to avoid copied pointer to become the first function
	FGESDynamicArg ParamWrapper;
	ParamWrapper.Arg01 = ParamData;

	PropData.Property = ObjectProperty;
	PropData.PropertyPtr = (void*)&ParamWrapper;
	
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData, ParamWrapper](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FObjectProperty::StaticClass(), PropData, Listener))
		{
			Listener.ReceiverWCO->ProcessEvent(Listener.Function, (void*)&ParamWrapper);// PropData.PropertyPtr);
		}
	});

	if (!EmitData.bPinned)
	{
		delete ObjectProperty;
	}
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, float ParamData)
{
	FGESPropertyEmitContext PropData(EmitData);

	FGESWildcardProperty WrapperProperty;

	FFloatProperty* FloatProperty =
		new FFloatProperty(FFieldVariant(EmitData.WorldContext->GetClass()),//WrapperProperty),
			TEXT("FloatValue"),
			EObjectFlags::RF_Public | EObjectFlags::RF_LoadCompleted);

	PropData.Property = FloatProperty;
	PropData.PropertyPtr = &ParamData;// Buffer.GetData();
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData, &ParamData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FNumericProperty::StaticClass(), PropData, Listener))
		{
			Listener.ReceiverWCO->ProcessEvent(Listener.Function, PropData.PropertyPtr);// PropData.PropertyPtr);
		}
	});

	if (!EmitData.bPinned)
	{
		delete FloatProperty;
	}
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, int32 ParamData)
{
	FGESPropertyEmitContext PropData(EmitData);

	FIntProperty* IntProperty =
		new FIntProperty(FFieldVariant(EmitData.WorldContext->GetClass()),
			TEXT("IntValue"),
			EObjectFlags::RF_Public | EObjectFlags::RF_LoadCompleted);

	PropData.Property = IntProperty;
	PropData.PropertyPtr = &ParamData;
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FNumericProperty::StaticClass(), PropData, Listener))
		{
			Listener.ReceiverWCO->ProcessEvent(Listener.Function, PropData.PropertyPtr);
		}
	});

	if (!EmitData.bPinned)
	{
		delete IntProperty;
	}
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, bool ParamData)
{
	FGESPropertyEmitContext PropData(EmitData);

	FBoolProperty* BoolProperty =
		new FBoolProperty(FFieldVariant(EmitData.WorldContext->GetClass()),
			TEXT("BoolValue"),
			EObjectFlags::RF_Public | EObjectFlags::RF_LoadCompleted);

	PropData.Property = BoolProperty;
	PropData.PropertyPtr = &ParamData;
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, FBoolProperty::StaticClass(), PropData, Listener))
		{
			Listener.ReceiverWCO->ProcessEvent(Listener.Function, PropData.PropertyPtr);
		}
	});

	if (!EmitData.bPinned)
	{
		delete BoolProperty;
	}
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, const FName& ParamData)
{
	FGESPropertyEmitContext PropData(EmitData);

	//We have no property context, make a new property
	FNameProperty* NameProperty =
		new FNameProperty(FFieldVariant(EmitData.WorldContext->GetClass()),
			TEXT("NameValue"),
			EObjectFlags::RF_Public | EObjectFlags::RF_LoadCompleted);

	//Wrap our FName into a buffer we can share
	TArray<uint8> Buffer;
	Buffer.SetNum(ParamData.StringBufferSize);

	NameProperty->SetPropertyValue_InContainer(Buffer.GetData(), ParamData);

	PropData.Property = NameProperty;
	PropData.PropertyPtr = Buffer.GetData();
	if (PropData.bPinned)
	{
		PropData.bHandleAllocation = true;
	}

	EmitToListenersWithData(PropData, [&PropData, ParamData](const FGESEventListener& Listener)
		{
			if (FunctionHasValidParams(Listener.Function, FStrProperty::StaticClass(), PropData, Listener))
			{
				Listener.ReceiverWCO->ProcessEvent(Listener.Function, PropData.PropertyPtr);
			}
		});

	if (!EmitData.bPinned)
	{
		delete NameProperty;
	}
}

bool FGESHandler::EmitEvent(const FGESEmitContext& EmitData)
{
	FGESPropertyEmitContext FullEmitData(EmitData);

	//No param version
	return EmitPropertyEvent(FullEmitData);
}

void FGESHandler::EmitEvent(const FGESEmitContext& EmitData, const GES_RAW_TEXT RawStringMessage)
{
	EmitEvent(EmitData, FString(RawStringMessage));
}

bool FGESHandler::EmitPropertyEvent(const FGESPropertyEmitContext& EmitData)
{
	//UE_LOG(LogTemp, Log, TEXT("World is: %s"), *EmitData.WorldContext.Get()->GetName());

	if (!EmitData.WorldContext || !EmitData.WorldContext->IsValidLowLevel())
	{
		//Remove this event, it's emit context is invalid
		DeleteEvent(EmitData.Domain, EmitData.Event);
		if (Options.bLogStaleRemovals)
		{
			UE_LOG(LogTemp, Log, TEXT("FGESHandler::EmitEvent stale event removed due to invalid world context for <%s.%s>. (Usually due to pinned events that haven't been unpinned)"),
				*EmitData.Domain, *EmitData.Event);
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
				/*
				Never gets called?
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
				}*/

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
					Listener.ReceiverWCO->ProcessEvent(Listener.Function, nullptr);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent %s tried to emit an empty event to %s receiver expecting parameters."),
						*EmitData.Event,
						*Listener.ReceiverWCO->GetName());
				}
			});
	}
	else if (ParameterProp->IsA<FStructProperty>())
	{
		EmitSubPropertyEvent(EmitData);
		return true;
	}
	else if (ParameterProp->IsA<FStrProperty>())
	{
		EmitSubPropertyEvent(EmitData);
		return true;
	}
	else if (ParameterProp->IsA<FObjectProperty>())
	{
		EmitSubPropertyEvent(EmitData);
		return true;
	}
	else if (ParameterProp->IsA<FNumericProperty>())
	{
		//todo warn numeric mismatch again (int/float)
		EmitSubPropertyEvent(EmitData);
		return true;
	}
	else if (ParameterProp->IsA<FBoolProperty>())
	{
		EmitSubPropertyEvent(EmitData);
		return true;
	}
	else if (ParameterProp->IsA<FNameProperty>())
	{
		EmitSubPropertyEvent(EmitData);
		return true;
	}
	else
	{
		//Maps, Array, Sets etc unsupported atm
		UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent Unsupported parameter"));
		return false;
	}
	return false;
}

void FGESHandler::EmitSubPropertyEvent(const FGESPropertyEmitContext& EmitData)
{
	EmitToListenersWithData(EmitData, [&EmitData](const FGESEventListener& Listener)
	{
		if (FunctionHasValidParams(Listener.Function, EmitData.Property->StaticClass(), EmitData, Listener))
		{
			/*
			Never gets called?
			//Lambda Bind
			if (Listener.bIsBoundToLambda && Listener.LambdaFunction != nullptr)
			{
				FGESWildcardProperty Wrapper;
				Wrapper.Property = EmitData.Property;
				Wrapper.PropertyPtr = EmitData.PropertyPtr;

				Listener.LambdaFunction(Wrapper);
				return;
			}
			//Delegate Bind
			if (Listener.bIsBoundToDelegate)
			{
				FGESWildcardProperty Wrapper;
				Wrapper.Property = EmitData.Property;
				Wrapper.PropertyPtr = EmitData.PropertyPtr;
				Listener.OnePropertyFunctionDelegate.ExecuteIfBound(Wrapper);
				return;
			}*/

			//Standard Function Name Bind
			Listener.ReceiverWCO->ProcessEvent(Listener.Function, EmitData.PropertyPtr);
		}
	});
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
