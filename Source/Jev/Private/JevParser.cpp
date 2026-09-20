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
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
		{
			if (Pair.Key.Equals(TEXT("noul"), ESearchCase::IgnoreCase))
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
