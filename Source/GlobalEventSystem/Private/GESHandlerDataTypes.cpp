#include "GESHandlerDataTypes.h"

void FGESPinnedData::CopyPropertyToPinnedBuffer()
{
	//Copy this property data to temp
	{
		//Workaround for our generated struct
		int32 Num = Property->GetSize();
		/*if (Property->IsA<FStructProperty>())
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Property);
			if (StructProp->Struct)
			{
				Num = StructProp->Struct->PropertiesSize;
			}
		}*/
		
		PropertyData.SetNumUninitialized(Num);
		FMemory::Memcpy(PropertyData.GetData(), PropertyPtr, Num);

		//reset pointer to new copy
		PropertyPtr = PropertyData.GetData();
	}
}

void FGESPinnedData::CleanupPinnedData()
{
	PropertyData.Empty();

	//Some properties are being allocated in C++, we need to clean them here
	if (bHandlePropertyDeletion)
	{
		if (Property != nullptr)
		{
			Property->SetFlags(RF_BeginDestroyed);
		}
		delete Property;
	}
	Property = nullptr;
	PropertyPtr = nullptr;
}

FGESEvent::FGESEvent()
{
	PinnedData = FGESPinnedData();
}

FGESPropertyEmitContext::FGESPropertyEmitContext()
{
	Property = nullptr;
	PropertyPtr = nullptr;
	SpecificTarget = nullptr;
	bHandleAllocation = false;
}

FGESPropertyEmitContext::FGESPropertyEmitContext(const FGESEmitContext& Other)
{
	Domain = Other.Domain;
	Event = Other.Event;
	WorldContext = Other.WorldContext;
	bPinned = Other.bPinned;

	Property = nullptr;
	PropertyPtr = nullptr;
	SpecificTarget = nullptr;
}

FGESEvent::FGESEvent(const FGESEmitContext& Other)
{
	Domain = Other.Domain;
	Event = Other.Event;
	WorldContext = Other.WorldContext;
	bPinned = Other.bPinned;
}

FGESMinimalEventListener::FGESMinimalEventListener()
{
	ReceiverWCO = nullptr;
	FunctionName = TEXT("");
}

FGESEventListener::FGESEventListener()
{
	FGESMinimalEventListener();
	Function = nullptr;
	bIsBoundToDelegate = false;
	bIsBoundToLambda = false;
	LambdaFunction = nullptr;
}

FGESEventListener::FGESEventListener(const FGESMinimalEventListener& Minimal)
{
	ReceiverWCO = Minimal.ReceiverWCO;
	FunctionName = Minimal.FunctionName;
}

bool FGESEventListener::LinkFunction()
{
	Function = ReceiverWCO->FindFunction(FName(*FunctionName));
	return IsValidListener();
}

bool FGESEventListener::IsValidListener() const
{
	return (Function != nullptr || 
		bIsBoundToDelegate ||
		(bIsBoundToLambda && LambdaFunction != nullptr));
}


