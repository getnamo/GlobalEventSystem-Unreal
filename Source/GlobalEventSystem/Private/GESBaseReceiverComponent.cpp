#include "GESBaseReceiverComponent.h"
#include "GlobalEventSystemBPLibrary.h"

UGESBaseReceiverComponent::UGESBaseReceiverComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	bBindOnBeginPlay = true;
	bUnbindOnEndPlay = true;

	BindSettings.ReceivingFunction = TEXT("OnEvent(component)");
}

/*void UGESBaseReceiverComponent::OnEventToPlaceHolderFunction(const FString& PlaceHolderParam)
{
	UE_LOG(LogTemp, Warning, TEXT("PlaceHolder function received event (override BindSettings.ReceivingFunction to match your own receiver)"));
}*/

void UGESBaseReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bBindOnBeginPlay)
	{
		//special case
		if (BindSettings.ReceivingFunction == TEXT("OnEvent(component)"))
		{
			UGlobalEventSystemBPLibrary::GESBindEventToWildcardDelegate(this, OnEvent, BindSettings.Domain, BindSettings.Event);
		}
		else
		{
			UGlobalEventSystemBPLibrary::GESBindEvent(this, BindSettings.Domain, BindSettings.Event, BindSettings.ReceivingFunction);
		}
	}
}

void UGESBaseReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bUnbindOnEndPlay)
	{
		if (BindSettings.ReceivingFunction == TEXT("OnEvent(component)"))
		{
			//UGlobalEventSystemBPLibrary::GESUnbindEvent()
		}
		else
		{
			UGlobalEventSystemBPLibrary::GESUnbindEvent(this, BindSettings.Domain, BindSettings.Event, BindSettings.ReceivingFunction);
		}
	}
	Super::EndPlay(EndPlayReason);
}