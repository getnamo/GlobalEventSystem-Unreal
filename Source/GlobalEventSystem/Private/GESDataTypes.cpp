#include "GESDataTypes.h"

FGESNameBind::FGESNameBind()
{
	Domain = TEXT("global.default");
	Event = TEXT("");
	ReceivingFunction = TEXT("");
}


FGESEventContext::FGESEventContext()
{
	Domain = TEXT("global.default");
	Event = TEXT("");
	WorldContext = nullptr;
}