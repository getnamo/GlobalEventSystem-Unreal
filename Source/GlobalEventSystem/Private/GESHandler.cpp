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

void FGESHandler::CreateEvent(const FString& TargetDomain, const FString& TargetFunction)
{
	FGESEvent CreatedFunction;
	CreatedFunction.TargetDomain = TargetDomain;
	CreatedFunction.TargetFunction = TargetFunction;
	FunctionMap.Add(Key(TargetDomain, TargetFunction), CreatedFunction);
}

void FGESHandler::DeleteEvent(const FString& TargetDomain, const FString& TargetFunction)
{
	FunctionMap.Remove(Key(TargetDomain, TargetFunction));
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
		FunctionMap[KeyString].Listeners.Add(Listener);
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

void FGESHandler::ForEachListener(const FString& TargetDomain, const FString& TargetFunction, TFunction<void(const FGESEventListener&)> Callback)
{
	FString KeyString = Key(TargetDomain, TargetFunction);
	FGESEvent& Event = FunctionMap[KeyString];

	for (FGESEventListener& Listener : Event.Listeners)
	{
		//Check validity of receiver and function and call the function
		if (Listener.Receiver->IsValidLowLevel())
		{
			UFunction* BPFunction = Listener.Receiver->FindFunction(FName(*Listener.FunctionName));
			if (BPFunction != nullptr)
			{
				Callback(Listener);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FGESHandler::EmitEvent: Function not found '%s'"), *Listener.FunctionName);
			}
		}
		else
		{
			RemovalArray.Add(&Listener);
		}
	}
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

void FGESHandler::EmitEvent(const FString& TargetDomain, const FString& TargetFunction, UStruct* Struct, void* StructPtr)
{
	ForEachListener(TargetDomain, TargetFunction, [Struct, StructPtr](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, StructPtr);
	});
}

void FGESHandler::EmitEvent(const FString& TargetDomain, const FString& TargetFunction, const FString& ParamData)
{
	FString MutableParamString = ParamData;
	ForEachListener(TargetDomain, TargetFunction, [&MutableParamString](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &MutableParamString);
	});
}

void FGESHandler::EmitEvent(const FString& TargetDomain, const FString& TargetFunction, float ParamData)
{
	ForEachListener(TargetDomain, TargetFunction, [&ParamData](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
	});
}

void FGESHandler::EmitEvent(const FString& TargetDomain, const FString& TargetFunction, int32 ParamData)
{
	ForEachListener(TargetDomain, TargetFunction, [&ParamData](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
	});
}

void FGESHandler::EmitEvent(const FString& TargetDomain, const FString& TargetFunction, bool ParamData)
{
	ForEachListener(TargetDomain, TargetFunction, [&ParamData](const FGESEventListener& Listener)
	{
		Listener.Receiver->ProcessEvent(Listener.Function, &ParamData);
	});
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

