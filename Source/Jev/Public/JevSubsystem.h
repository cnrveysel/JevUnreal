#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JevTypes.h"
#include "JevSubsystem.generated.h"

#define UE_API JEV_API

class IHttpRequest;

DECLARE_DELEGATE_TwoParams(FJevYesNoResult, FJevDecisionResult, const FString&);
DECLARE_DELEGATE_TwoParams(FJevProbabilityResultDelegate, FJevProbabilityResult, const FString&);
DECLARE_DELEGATE_TwoParams(FJevChooseResultDelegate, FJevChooseResult, const FString&);
DECLARE_DELEGATE_TwoParams(FJevRequestResultDelegate, FJevRequestResult, const FString&);

/**
 * Game instance subsystem that performs asynchronous Jev requests.
 * Blueprint-facing async actions wrap this class.
 */
UCLASS(BlueprintType)
class UE_API UJevSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ UGameInstanceSubsystem
	virtual void Deinitialize() override;

	/** Sends a Yes / No decision request. OnDone fires when the request completes. */
	void RequestYesNo(const FString& State, const FString& Question, float TimeoutOverrideSeconds, const FJevYesNoResult& OnDone, const FString& EndpointOverride = FString());

	/** Sends a noul request and returns its probability without a Yes / No output. */
	void RequestProbability(const FString& State, const FString& Question, float TimeoutOverrideSeconds, const FJevProbabilityResultDelegate& OnDone, const FString& EndpointOverride = FString());

	/** Sends a Choose decision request over the supplied options. */
	void RequestChoose(const FString& State, const FString& Question, const TArray<FString>& Options, float TimeoutOverrideSeconds, const FJevChooseResultDelegate& OnDone, const FString& EndpointOverride = FString());

	/** Sends a generic request with raw questions JSON. OnDone fires when the request completes. */
	void RequestGeneric(const FString& State, const FString& RawQuestionsJson, const FString& ModelOverride, const FString& EndpointOverride, const FJevRequestResultDelegate& OnDone);

private:
	FString ResolveEndpoint(const FString& EndpointOverride) const;
	FString ResolveModel(const FString& ModelOverride) const;

	/** Active requests, kept so weak-object lifetimes remain clean. */
	TArray<TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>> ActiveRequests;
};

#undef UE_API
