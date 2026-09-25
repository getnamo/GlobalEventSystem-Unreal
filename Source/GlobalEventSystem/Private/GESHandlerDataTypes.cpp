#include "GESHandlerDataTypes.h"

FGESPinnedData::FGESPinnedData(FGESPinnedData&& Other)
	: FGESPinnedData()
{
	*this = MoveTemp(Other);
}

FGESPinnedData& FGESPinnedData::operator=(FGESPinnedData&& Other)
{
	if (this != &Other)
	{
		CleanupPinnedData();

		Property = Other.Property;
		PropertyPtr = Other.PropertyPtr;
		bHandlePropertyDeletion = Other.bHandlePropertyDeletion;
		PinnedBuffer = Other.PinnedBuffer;
		PinnedBufferProperty = Other.PinnedBufferProperty;

		//Other no longer owns anything
		Other.Property = nullptr;
		Other.PropertyPtr = nullptr;
		Other.bHandlePropertyDeletion = false;
		Other.PinnedBuffer = nullptr;
		Other.PinnedBufferProperty = nullptr;
	}
	return *this;
}

void FGESPinnedData::CopyPropertyToPinnedBuffer()
{
	if (Property == nullptr || PropertyPtr == nullptr || PropertyPtr == PinnedBuffer)
	{
		return;
	}

	//Deep copy via the property so heap backed members (FString, TArray, etc) outlive the emitter's memory.
	//A raw memcpy here would leave the pinned value pointing at memory the emitter frees after emitting.
	void* NewBuffer = FMemory::Malloc(Property->GetSize(), Property->GetMinAlignment());
	Property->InitializeValue(NewBuffer);
	Property->CopyCompleteValue(NewBuffer, PropertyPtr);

	DestroyPinnedBuffer();
	PinnedBuffer = NewBuffer;
	PinnedBufferProperty = Property;

	//reset pointer to new copy
	PropertyPtr = PinnedBuffer;
}

void FGESPinnedData::DestroyPinnedBuffer()
{
	if (PinnedBuffer)
	{
		if (PinnedBufferProperty)
		{
			PinnedBufferProperty->DestroyValue(PinnedBuffer);
		}
		FMemory::Free(PinnedBuffer);
	}
	PinnedBuffer = nullptr;
	PinnedBufferProperty = nullptr;
}

void FGESPinnedData::CleanupPinnedData()
{
	//Destroy the value before the property that describes it
	DestroyPinnedBuffer();

	//Some properties are being allocated in C++, we need to clean them here
	if (bHandlePropertyDeletion)
	{
		delete Property;
	}
	Property = nullptr;
	PropertyPtr = nullptr;
	bHandlePropertyDeletion = false;
}

FGESEvent::FGESEvent()
{
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
	bHandleAllocation = false;
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
	Function = nullptr;
	bIsBoundToDelegate = false;
	bIsBoundToLambda = false;
	LambdaFunction = nullptr;
}

FGESEventListener::FGESEventListener(const FGESMinimalEventListener& Minimal)
	: FGESEventListener()
{
	ReceiverWCO = Minimal.ReceiverWCO;
	FunctionName = Minimal.FunctionName;
}

bool FGESEventListener::LinkFunction()
{
	UObject* Receiver = ReceiverWCO.Get();
	Function = Receiver ? Receiver->FindFunction(FName(*FunctionName)) : nullptr;
	return IsValidListener();
}

bool FGESEventListener::IsValidListener() const
{
	return (Function != nullptr || 
		bIsBoundToDelegate ||
		(bIsBoundToLambda && LambdaFunction != nullptr));
}


