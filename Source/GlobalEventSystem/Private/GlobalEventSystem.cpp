// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GlobalEventSystem.h"

#if WITH_EDITOR
#include "Editor.h"
#include "GESHandler.h"
#endif

#define LOCTEXT_NAMESPACE "FGlobalEventSystemModule"

void FGlobalEventSystemModule::StartupModule()
{
#if WITH_EDITOR
	EndPieDelegate = FEditorDelegates::BeginPIE.AddLambda([](bool boolSent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Clearing FGESHandler"));
		FGESHandler::Clear();		
	});
#endif
}

void FGlobalEventSystemModule::ShutdownModule()
{
#if WITH_EDITOR
	FEditorDelegates::EndPIE.Remove(EndPieDelegate);
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGlobalEventSystemModule, GlobalEventSystem)