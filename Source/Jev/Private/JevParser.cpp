#include "JevParser.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FJevParser::IsValidProbability(double Value)
{
	return !FMath::IsNaN(Value) && Value >= 0.0 && Value <= 1.0;
}

void FJevParser::NormalizeYesNo(double YesProbability, bool& bOutYes, double& OutConfidence)
{
	bOutYes = YesProbability >= 0.5;
	OutConfidence = bOutYes ? YesProbability : 1.0 - YesProbability;
}

static bool FindFirstNoul(const TSharedPtr<FJsonValue>& Value, double& OutYesProbability)
{
	if (!Value.IsValid())
	{
		return false;
	}

	if (Value->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> Object = Value->AsObject();
		for (const TPair<UE::FSharedString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
		{
			if (Pair.Key.ToView().Equals(TEXT("noul"), ESearchCase::IgnoreCase))
			{
				const TSharedPtr<FJsonValue>& NoulValue = Pair.Value;
				if (NoulValue.IsValid() && NoulValue->Type == EJson::Number)
				{
					const double Candidate = NoulValue->AsNumber();
					if (FJevParser::IsValidProbability(Candidate))
					{
						OutYesProbability = Candidate;
						return true;
					}
				}
			}
			if (FindFirstNoul(Pair.Value, OutYesProbability))
			{
				return true;
			}
		}
		return false;
	}

	if (Value->Type == EJson::Array)
	{
		for (const TSharedPtr<FJsonValue>& Element : Value->AsArray())
		{
			if (FindFirstNoul(Element, OutYesProbability))
			{
				return true;
			}
		}
	}

	return false;
}

bool FJevParser::ExtractYesProbability(const TSharedRef<FJsonObject>& Root, double& OutYesProbability)
{
	TSharedPtr<FJsonValue> RootValue = MakeShared<FJsonValueObject>(Root);
	return FindFirstNoul(RootValue, OutYesProbability);
}

static TSharedPtr<FJsonValue> FindCaseInsensitive(const TSharedRef<FJsonObject>& Object, const FString& Key)
{
	if (TSharedPtr<FJsonValue> Found = Object->TryGetField(Key))
	{
		return Found;
	}

	for (const TPair<UE::FSharedString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (Pair.Key.ToView().Equals(Key, ESearchCase::IgnoreCase))
		{
			return Pair.Value;
		}
	}
	return nullptr;
}

bool FJevParser::ExtractChoice(const TSharedRef<FJsonObject>& Root, FString& OutChoice, double& OutConfidence)
{
	const TSharedPtr<FJsonValue> AnswersValue = FindCaseInsensitive(Root, TEXT("answers"));
	const TSharedPtr<FJsonObject> Answers = AnswersValue.IsValid() ? AnswersValue->AsObject() : nullptr;
	if (!Answers.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonValue> DecisionValue = FindCaseInsensitive(Answers.ToSharedRef(), TEXT("decision"));
	const TSharedPtr<FJsonObject> Decision = DecisionValue.IsValid() ? DecisionValue->AsObject() : nullptr;
	if (!Decision.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonValue> ChoiceValue = FindCaseInsensitive(Decision.ToSharedRef(), TEXT("choice"));
	const TSharedPtr<FJsonValue> ConfidenceValue = FindCaseInsensitive(Decision.ToSharedRef(), TEXT("confidence"));
	if (!ChoiceValue.IsValid() || !ConfidenceValue.IsValid())
	{
		return false;
	}

	OutChoice = ChoiceValue->AsString();
	OutConfidence = ConfidenceValue->AsNumber();
	return !OutChoice.IsEmpty() && FJevParser::IsValidProbability(OutConfidence);
}

TSharedPtr<FJsonObject> FJevParser::ParseJson(const FString& JsonText, FString& OutParseError)
{
	if (JsonText.TrimStartAndEnd().IsEmpty())
	{
		OutParseError = TEXT("Empty response body");
		return nullptr;
	}

	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutParseError = TEXT("Response body is not a JSON object");
		return nullptr;
	}

	return Root;
}
