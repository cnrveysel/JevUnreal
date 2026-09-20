#pragma once

#include "CoreMinimal.h"

#define UE_API JEV_API

class FJsonObject;

/**
 * Pure parsing/normalization logic, deliberately separate from HTTP
 * orchestration so it can be unit tested.
 */
struct FJevParser
{
	/** Clamps nothing: returns false when the probability is outside 0..1. */
	UE_API static bool IsValidProbability(double Value);

	/**
	 * Normalizes a valid Yes probability into an answer and confidence.
	 * YES and confidence = p when p >= 0.5, otherwise NO and 1 - p.
	 */
	UE_API static void NormalizeYesNo(double YesProbability, bool& bOutYes, double& OutConfidence);

	/**
	 * Extracts the first "noul" numeric value from a parsed JSON tree.
	 * Returns false (and leaves OutYesProbability untouched) when no valid
	 * numeric noul in 0..1 exists anywhere in the tree.
	 */
	UE_API static bool ExtractYesProbability(const TSharedRef<FJsonObject>& Root, double& OutYesProbability);

	/**
	 * Extracts the TypeSafe choice response: case-insensitive "choice",
	 * "confidence", and optional "probabilities" for the selected choice.
	 * Returns false unless a nonempty choice and valid confidence are present.
	 */
	UE_API static bool ExtractChoice(const TSharedRef<FJsonObject>& Root, FString& OutChoice, double& OutConfidence);

	/** Parses a JSON object from raw text. Returns nullptr on malformed input. */
	UE_API static TSharedPtr<FJsonObject> ParseJson(const FString& JsonText, FString& OutParseError);
};

#undef UE_API
