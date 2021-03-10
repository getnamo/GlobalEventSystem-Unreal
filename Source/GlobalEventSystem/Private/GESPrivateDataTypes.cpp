#include "GESPrivateDataTypes.h"

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

FGESEmitData::FGESEmitData()
{
	Domain = TEXT("global.default");
	Event = TEXT("");
	WorldContext = nullptr;
	bPinned = false;
}

FGESEvent::FGESEvent()
{
	PinnedData = FGESPinnedData();
}

FGESFullEmitData::FGESFullEmitData()
{
	Property = nullptr;
	PropertyPtr = nullptr;
	SpecificTarget = nullptr;
}

FGESFullEmitData::FGESFullEmitData(const FGESEmitData& Other)
{
	Domain = Other.Domain;
	Event = Other.Event;
	WorldContext = Other.WorldContext;
	bPinned = Other.bPinned;
}

FGESEvent::FGESEvent(const FGESEmitData& Other)
{
	Domain = Other.Domain;
	Event = Other.Event;
	WorldContext = Other.WorldContext;
	bPinned = Other.bPinned;
}

FGESEventListener::FGESEventListener()
{
	bIsBoundToDelegate = false;
	bIsBoundToLambda = false;
	FunctionName = TEXT("");
	Function = nullptr;
	Receiver = nullptr;
	LambdaFunction = nullptr;
}

bool FGESEventListener::LinkFunction()
{
	Function = Receiver->FindFunction(FName(*FunctionName));
	return IsValidListener();
}

bool FGESEventListener::IsValidListener() const
{
	return (Function != nullptr || bIsBoundToDelegate || bIsBoundToLambda);
}