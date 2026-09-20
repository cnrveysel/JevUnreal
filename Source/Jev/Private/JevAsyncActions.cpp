#include "JevAsyncActions.h"
#include "JevSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UAsyncActionJevYesNo* UAsyncActionJevYesNo::JevYesNo(UObject* WorldContextObject, const FString& State, const FString& Question, float TimeoutSeconds, const FString& EndpointOverride)
{
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
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		return;
	}

	FJevYesNoResult OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevYesNo::HandleResult);
	Subsystem->RequestYesNo(State, Question, TimeoutSeconds, OnDone);
}

void UAsyncActionJevYesNo::HandleResult(FJevDecisionResult Result, const FString& Error)
{
	if (!IsValid(this))
	{
		return;
	}
	if (!Error.IsEmpty())
	{
		OnError.Broadcast(Error);
	}
	else
	{
		OnSuccess.Broadcast(Result);
	}
}

void UAsyncActionJevYesNo::Cancel()
{
	Super::Cancel();
}

UAsyncActionJevRequest* UAsyncActionJevRequest::JevRequest(UObject* WorldContextObject, const FString& State, const FString& RawQuestionsJson, const FString& ModelOverride, const FString& EndpointOverride)
{
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
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		return;
	}

	FJevRequestResultDelegate OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevRequest::HandleResult);
	Subsystem->RequestGeneric(State, RawQuestionsJson, ModelOverride, EndpointOverride, OnDone);
}

void UAsyncActionJevRequest::HandleResult(FJevRequestResult Result, const FString& Error)
{
	if (!IsValid(this))
	{
		return;
	}
	if (!Error.IsEmpty())
	{
		OnError.Broadcast(Error);
	}
	else
	{
		OnSuccess.Broadcast(Result);
	}
}

void UAsyncActionJevRequest::Cancel()
{
	Super::Cancel();
}
