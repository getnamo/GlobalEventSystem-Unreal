#include "GESDataTypes.h"

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
	if (Property && Property->IsValidLowLevelFast())
	{
		Property->RemoveFromRoot();
	}
	PropertyData.Empty();
	Property = nullptr;
	PropertyPtr = nullptr;
}