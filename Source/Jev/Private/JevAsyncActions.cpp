#include "JevAsyncActions.h"
#include "JevSubsystem.h"
#include "JevModule.h"
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
	UE_LOG(LogJev, Verbose, TEXT("[Jev] Yes/No node activated"));
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
		UE_LOG(LogJev, Warning, TEXT("[Jev] No valid world context for Yes/No request"));
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		SetReadyToDestroy();
		return;
	}

	FJevYesNoResult OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevYesNo::HandleResult);
	Subsystem->RequestYesNo(State, Question, TimeoutSeconds, OnDone, EndpointOverride);
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
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Blueprint error"));
		OnError.Broadcast(Error);
	}
	else
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Blueprint success"));
		OnSuccess.Broadcast(Result);
	}
	SetReadyToDestroy();
}

UAsyncActionJevProbability* UAsyncActionJevProbability::JevProbability(UObject* WorldContextObject, const FString& State, const FString& Question, float TimeoutSeconds, const FString& EndpointOverride)
{
	UAsyncActionJevProbability* Action = NewObject<UAsyncActionJevProbability>();
	Action->WorldContext = WorldContextObject;
	Action->State = State;
	Action->Question = Question;
	Action->TimeoutSeconds = TimeoutSeconds;
	Action->EndpointOverride = EndpointOverride;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncActionJevProbability::Activate()
{
	UE_LOG(LogJev, Verbose, TEXT("[Jev] Probability node activated"));
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
		UE_LOG(LogJev, Warning, TEXT("[Jev] No valid world context for Probability request"));
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		SetReadyToDestroy();
		return;
	}

	FJevProbabilityResultDelegate OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevProbability::HandleResult);
	Subsystem->RequestProbability(State, Question, TimeoutSeconds, OnDone, EndpointOverride);
}

void UAsyncActionJevProbability::HandleResult(FJevProbabilityResult Result, const FString& Error)
{
	if (bHasCompleted)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Ignoring duplicate Probability completion"));
		return;
	}
	bHasCompleted = true;

	if (!Error.IsEmpty())
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Probability Blueprint error"));
		OnError.Broadcast(Error);
	}
	else
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Probability Blueprint success"));
		OnSuccess.Broadcast(Result);
	}
	SetReadyToDestroy();
}

UAsyncActionJevChoose* UAsyncActionJevChoose::JevChoose(UObject* WorldContextObject, const FString& State, const FString& Question, const TArray<FString>& Options, float TimeoutSeconds, const FString& EndpointOverride)
{
	UAsyncActionJevChoose* Action = NewObject<UAsyncActionJevChoose>();
	Action->WorldContext = WorldContextObject;
	Action->State = State;
	Action->Question = Question;
	Action->Options = Options;
	Action->TimeoutSeconds = TimeoutSeconds;
	Action->EndpointOverride = EndpointOverride;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncActionJevChoose::Activate()
{
	UE_LOG(LogJev, Verbose, TEXT("[Jev] Choose node activated"));
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
		UE_LOG(LogJev, Warning, TEXT("[Jev] No valid world context for Choose request"));
		OnError.Broadcast(TEXT("Jev requires a valid world context"));
		SetReadyToDestroy();
		return;
	}

	FJevChooseResultDelegate OnDone;
	OnDone.BindUObject(this, &UAsyncActionJevChoose::HandleResult);
	Subsystem->RequestChoose(State, Question, Options, TimeoutSeconds, OnDone, EndpointOverride);
}

void UAsyncActionJevChoose::HandleResult(FJevChooseResult Result, const FString& Error)
{
	if (bHasCompleted)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Ignoring duplicate Choose completion"));
		return;
	}
	bHasCompleted = true;

	if (!Error.IsEmpty())
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Choose Blueprint error"));
		OnError.Broadcast(Error);
	}
	else
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Choose Blueprint success"));
		OnSuccess.Broadcast(Result);
	}
	SetReadyToDestroy();
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
	UE_LOG(LogJev, Verbose, TEXT("[Jev] Generic node activated"));
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
		UE_LOG(LogJev, Warning, TEXT("[Jev] No valid world context for generic request"));
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
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Blueprint error"));
		OnError.Broadcast(Error);
	}
	else
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] Broadcasting Blueprint success"));
		OnSuccess.Broadcast(Result);
	}
	SetReadyToDestroy();
}
