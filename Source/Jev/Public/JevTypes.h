#pragma once

#include "CoreMinimal.h"
#include "JevTypes.generated.h"

/** Convenience answer for the Jev Yes / No node. */
UENUM(BlueprintType)
enum class EJevYesNo : uint8
{
	Yes,
	No
};

/** Result of a Yes / No decision. */
USTRUCT(BlueprintType)
struct FJevDecisionResult
{
	GENERATED_BODY()

	/** YES if the Yes probability was >= 0.5, otherwise NO. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	EJevYesNo Answer = EJevYesNo::No;

	/** Raw probability of YES returned by Jev, clamped-valid only (0..1). */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float YesProbability = 0.f;

	/** Confidence in the chosen answer: YesProbability when YES, 1 - YesProbability when NO. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float Confidence = 0.f;

	/** Raw JSON response body, useful for debugging. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	FString RawResponse;

	/** Round-trip latency of the HTTP request in milliseconds. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float LatencyMs = 0.f;
};

/** Raw noul probability and confidence for the Jev Probability node. */
USTRUCT(BlueprintType)
struct FJevProbabilityResult
{
	GENERATED_BODY()

	/** The noul probability returned by Jev, validated to be in 0..1. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float Probability = 0.f;

	/** Confidence in the more likely outcome: max(Probability, 1 - Probability). */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float Confidence = 0.f;

	/** Raw JSON response body, useful for debugging. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	FString RawResponse;

	/** Round-trip latency of the HTTP request in milliseconds. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float LatencyMs = 0.f;
};

/** Result of a generic Jev request. */
USTRUCT(BlueprintType)
struct FJevRequestResult
{
	GENERATED_BODY()

	/** True when the HTTP request succeeded (2xx) and the response body parsed as JSON. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	bool bSuccess = false;

	/** HTTP status code of the response (0 when the request never completed). */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	int32 HttpStatusCode = 0;

	/** Raw JSON response body. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	FString RawJsonResponse;

	/** Human-readable error when bSuccess is false. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	FString ErrorMessage;

	/** Round-trip latency of the HTTP request in milliseconds. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float LatencyMs = 0.f;
};

/** Result of a Choose decision. */
USTRUCT(BlueprintType)
struct FJevChooseResult
{
	GENERATED_BODY()

	/** Option text returned by Jev. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	FString SelectedOption;

	/** Zero-based index into the supplied Options array; INDEX_NONE when invalid. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	int32 SelectedIndex = INDEX_NONE;

	/** Confidence returned by Jev, clamped-valid only (0..1). */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float Confidence = 0.f;

	/** Raw JSON response body, useful for debugging. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	FString RawResponse;

	/** Round-trip latency of the HTTP request in milliseconds. */
	UPROPERTY(BlueprintReadOnly, Category="Jev")
	float LatencyMs = 0.f;
};
