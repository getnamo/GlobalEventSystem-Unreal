#include "GESHandlerDataTypes.h"

void FGESPinnedData::CopyPropertyToPinnedBuffer()
{
	//Copy this property data to temp
	//Property->AddToRoot();
	int32 Num = Property->GetSize();
	PropertyData.SetNumUninitialized(Num);
	FMemory::Memcpy(PropertyData.GetData(), PropertyPtr, Num);

	//reset pointer to new copy
	PropertyPtr = PropertyData.GetData();
}

void FGESPinnedData::CleanupPinnedData()
{
	PropertyData.Empty();
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

FGESEventListener::FGESEventListener()
{
	ReceiverWCO = nullptr;
	FunctionName = TEXT("");
	Function = nullptr;
	bIsBoundToDelegate = false;
	bIsBoundToLambda = false;
	LambdaFunction = nullptr;
}

bool FGESEventListener::LinkFunction()
{
	Function = ReceiverWCO->FindFunction(FName(*FunctionName));
	return IsValidListener();
}

bool FGESEventListener::IsValidListener() const
{
	return (Function != nullptr || bIsBoundToDelegate || bIsBoundToLambda);
}