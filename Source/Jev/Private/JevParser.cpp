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
		const TSharedPtr<UE::JSON::Private::FJsonStringSet> StringSet = Object->GetStringSet();
		if (!StringSet.IsValid())
		{
			return false;
		}

		for (const UE::FSharedString& FieldName : *StringSet)
		{
			if (FieldName.ToView().Equals(TEXT("noul"), ESearchCase::IgnoreCase))
			{
				const TSharedPtr<FJsonValue> NoulValue = Object->TryGetField(FieldName);
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
			if (FindFirstNoul(Object->TryGetField(FieldName), OutYesProbability))
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

	const TSharedPtr<UE::JSON::Private::FJsonStringSet> StringSet = Object->GetStringSet();
	if (!StringSet.IsValid())
	{
		return nullptr;
	}

	for (const UE::FSharedString& FieldName : *StringSet)
	{
		if (FieldName.ToView().Equals(Key, ESearchCase::IgnoreCase))
		{
			return Object->TryGetField(FieldName);
		}
	}
	return nullptr;
}

bool FJevParser::ExtractChoice(const TSharedRef<FJsonObject>& Root, FString& OutChoice, double& OutConfidence)
{
	const TSharedPtr<FJsonValue> ChoiceValue = FindCaseInsensitive(Root, TEXT("choice"));
	const TSharedPtr<FJsonValue> ConfidenceValue = FindCaseInsensitive(Root, TEXT("confidence"));
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
