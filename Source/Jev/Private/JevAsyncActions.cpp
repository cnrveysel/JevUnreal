#include "JevAsyncActions.h"
#include "JevSubsystem.h"
#include "JevModule.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UAsyncActionJevYesNo* UAsyncActionJevYesNo::JevYesNo(UObject* WorldContextObject, const FString& State, const FString& Question, float TimeoutSeconds, const FString& EndpointOverride)
{
	UE_LOG(LogJev, Log, TEXT("[Jev] Async node factory created"));
	UAsyncActionJevYesNo* Action = NewObject<UAsyncActionJevYesNo>();
	Action->WorldContext = WorldContextObject;
	Action->State = State;
	Action->Question = Question;
	Action->TimeoutSeconds = TimeoutSeconds;
	Action->EndpointOverride = EndpointOverride;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncActionJevYesNo::Activate()
{
	UE_LOG(LogJev, Log, TEXT("[Jev] Async node activated"));
	UJevSubsystem* Subsystem = nullptr;
	if (UObject* Context = WorldContext.Get())
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr)
		{
			Subsystem = World->GetGameInstance()->GetSubsystem<UJevSubsystem>();
		}
	}

	if (!Subsystem)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Subsystem unresolved; broadcasting error"));
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		SetReadyToDestroy();
		return;
	}

	FJevYesNoResult OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevYesNo::HandleResult);
	Subsystem->RequestYesNo(State, Question, TimeoutSeconds, OnDone);
}

void UAsyncActionJevYesNo::HandleResult(FJevDecisionResult Result, const FString& Error)
{
	if (bHasCompleted)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Ignoring duplicate completion"));
		return;
	}
	bHasCompleted = true;

	if (!Error.IsEmpty())
	{
		UE_LOG(LogJev, Log, TEXT("[Jev] Broadcasting Blueprint error"));
		OnError.Broadcast(Error);
	}
	else
	{
		UE_LOG(LogJev, Log, TEXT("[Jev] Broadcasting Blueprint success"));
		OnSuccess.Broadcast(Result);
	}
	SetReadyToDestroy();
}

UAsyncActionJevRequest* UAsyncActionJevRequest::JevRequest(UObject* WorldContextObject, const FString& State, const FString& RawQuestionsJson, const FString& ModelOverride, const FString& EndpointOverride)
{
	UE_LOG(LogJev, Log, TEXT("[Jev] Async node factory created"));
	UAsyncActionJevRequest* Action = NewObject<UAsyncActionJevRequest>();
	Action->WorldContext = WorldContextObject;
	Action->State = State;
	Action->RawQuestionsJson = RawQuestionsJson;
	Action->ModelOverride = ModelOverride;
	Action->EndpointOverride = EndpointOverride;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncActionJevRequest::Activate()
{
	UE_LOG(LogJev, Log, TEXT("[Jev] Async node activated"));
	UJevSubsystem* Subsystem = nullptr;
	if (UObject* Context = WorldContext.Get())
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr)
		{
			Subsystem = World->GetGameInstance()->GetSubsystem<UJevSubsystem>();
		}
	}

	if (!Subsystem)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Subsystem unresolved; broadcasting error"));
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		SetReadyToDestroy();
		return;
	}

	FJevRequestResultDelegate OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevRequest::HandleResult);
	Subsystem->RequestGeneric(State, RawQuestionsJson, ModelOverride, EndpointOverride, OnDone);
}

void UAsyncActionJevRequest::HandleResult(FJevRequestResult Result, const FString& Error)
{
	if (bHasCompleted)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Ignoring duplicate completion"));
		return;
	}
	bHasCompleted = true;

	if (!Error.IsEmpty())
	{
		UE_LOG(LogJev, Log, TEXT("[Jev] Broadcasting Blueprint error"));
		OnError.Broadcast(Error);
	}
	else
	{
		UE_LOG(LogJev, Log, TEXT("[Jev] Broadcasting Blueprint success"));
		OnSuccess.Broadcast(Result);
	}
	SetReadyToDestroy();
}
