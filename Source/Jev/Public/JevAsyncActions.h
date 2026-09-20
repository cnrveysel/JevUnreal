#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "JevTypes.h"
#include "JevAsyncActions.generated.h"

#define UE_API JEV_API

class UJevSubsystem;

/**
 * "Jev Yes / No" — asynchronous decision node.
 * Sends State + Question to Jev and returns a YES/NO answer with probability.
 */
UCLASS()
class UE_API UAsyncActionJevYesNo : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevYesNoSuccess, FJevDecisionResult, Result);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevError, const FString&, Error);

	UPROPERTY(BlueprintAssignable, Category="Jev|Decision")
	FOnJevYesNoSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Jev|Decision")
	FOnJevError OnError;

	/**
	 * Ask Jev a Yes / No question about the given state.
	 * Answer is YES when the Yes probability is >= 0.5.
	 * Leave Endpoint Override empty to use the Project Settings endpoint.
	 */
	UFUNCTION(BlueprintCallable, Category="Jev|Decision", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Jev Yes / No", AdvancedDisplay="TimeoutSeconds,EndpointOverride"))
	static UAsyncActionJevYesNo* JevYesNo(UObject* WorldContextObject, const FString& State, const FString& Question, float TimeoutSeconds = 0.f, const FString& EndpointOverride = TEXT(""));

	//~ UCancellableAsyncAction
	virtual void Activate() override;

private:
	void HandleResult(FJevDecisionResult Result, const FString& Error);
	bool bHasCompleted = false;

	TWeakObjectPtr<UObject> WorldContext;
	FString State;
	FString Question;
	float TimeoutSeconds = 0.f;
	FString EndpointOverride;
};

/** Returns the noul probability for State + Question without a Yes / No output. */
UCLASS()
class UE_API UAsyncActionJevProbability : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevProbabilitySuccess, FJevProbabilityResult, Result);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevProbabilityError, const FString&, Error);

	UPROPERTY(BlueprintAssignable, Category="Jev|Decision")
	FOnJevProbabilitySuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Jev|Decision")
	FOnJevProbabilityError OnError;

	/** Leave Timeout Seconds at zero and Endpoint Override empty to use Project Settings. */
	UFUNCTION(BlueprintCallable, Category="Jev|Decision", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Jev Probability", AdvancedDisplay="TimeoutSeconds,EndpointOverride"))
	static UAsyncActionJevProbability* JevProbability(UObject* WorldContextObject, const FString& State, const FString& Question, float TimeoutSeconds = 0.f, const FString& EndpointOverride = TEXT(""));

	virtual void Activate() override;

private:
	void HandleResult(FJevProbabilityResult Result, const FString& Error);
	bool bHasCompleted = false;

	TWeakObjectPtr<UObject> WorldContext;
	FString State;
	FString Question;
	float TimeoutSeconds = 0.f;
	FString EndpointOverride;
};

/**
 * "Jev Choose" — asynchronous option decision node.
 * Sends State + Question + Options to Jev and returns exactly one option.
 */
UCLASS()
class UE_API UAsyncActionJevChoose : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevChooseSuccess, FJevChooseResult, Result);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevChooseError, const FString&, Error);

	UPROPERTY(BlueprintAssignable, Category="Jev|Decision")
	FOnJevChooseSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Jev|Decision")
	FOnJevChooseError OnError;

	UFUNCTION(BlueprintCallable, Category="Jev|Decision", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Jev Choose", AdvancedDisplay="TimeoutSeconds,EndpointOverride"))
	static UAsyncActionJevChoose* JevChoose(UObject* WorldContextObject, const FString& State, const FString& Question, const TArray<FString>& Options, float TimeoutSeconds = 0.f, const FString& EndpointOverride = TEXT(""));

	virtual void Activate() override;

private:
	void HandleResult(FJevChooseResult Result, const FString& Error);
	bool bHasCompleted = false;

	TWeakObjectPtr<UObject> WorldContext;
	FString State;
	FString Question;
	TArray<FString> Options;
	float TimeoutSeconds = 0.f;
	FString EndpointOverride;
};

/**
 * "Make Jev Request" — advanced asynchronous node.
 * Sends State + raw Questions JSON and returns the raw Jev response.
 */
UCLASS()
class UE_API UAsyncActionJevRequest : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevRequestSuccess, FJevRequestResult, Result);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJevRequestError, const FString&, Error);

	UPROPERTY(BlueprintAssignable, Category="Jev|Request")
	FOnJevRequestSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Jev|Request")
	FOnJevRequestError OnError;

	/**
	 * Advanced: send a raw Jev request. Raw Questions JSON must be a JSON
	 * object such as {"decision":{"type":"noul","instructions":"..."}}.
	 * Leave Model Override / Endpoint Override empty for Project Settings values.
	 */
	UFUNCTION(BlueprintCallable, Category="Jev|Request", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Make Jev Request", AdvancedDisplay="ModelOverride,EndpointOverride"))
	static UAsyncActionJevRequest* JevRequest(UObject* WorldContextObject, const FString& State, const FString& RawQuestionsJson, const FString& ModelOverride = TEXT(""), const FString& EndpointOverride = TEXT(""));

	//~ UCancellableAsyncAction
	virtual void Activate() override;

private:
	void HandleResult(FJevRequestResult Result, const FString& Error);
	bool bHasCompleted = false;

	TWeakObjectPtr<UObject> WorldContext;
	FString State;
	FString RawQuestionsJson;
	FString ModelOverride;
	FString EndpointOverride;
};

#undef UE_API
