#include "Misc/AutomationTest.h"
#include "JevParser.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

static TSharedPtr<FJsonObject> ParseObject(const FString& Text)
{
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	TSharedPtr<FJsonObject> Root;
	FJsonSerializer::Deserialize(Reader, Root);
	return Root;
}

BEGIN_DEFINE_SPEC(FJevParserSpec, "Jev.Parser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FJevParserSpec)

void FJevParserSpec::Define()
{
	Describe("Normalization", [this]()
	{
		It("maps 0.84 to YES with confidence 0.84", [this]()
		{
			bool bYes = false;
			double Confidence = 0.0;
			FJevParser::NormalizeYesNo(0.84, bYes, Confidence);
			TestTrue(TEXT("should be yes"), bYes);
			TestEqual(TEXT("confidence"), Confidence, 0.84);
		});

		It("maps 0.30 to NO with confidence 0.70", [this]()
		{
			bool bYes = false;
			double Confidence = 0.0;
			FJevParser::NormalizeYesNo(0.30, bYes, Confidence);
			TestFalse(TEXT("should be no"), bYes);
			TestEqual(TEXT("confidence"), Confidence, 0.70);
		});
	});

	Describe("Probability validation", [this]()
	{
		It("rejects values outside 0..1", [this]()
		{
			TestFalse(TEXT("-0.1 invalid"), FJevParser::IsValidProbability(-0.1));
			TestFalse(TEXT("1.1 invalid"), FJevParser::IsValidProbability(1.1));
			TestTrue(TEXT("0.5 valid"), FJevParser::IsValidProbability(0.5));
		});
	});

	Describe("Response extraction", [this]()
	{
		It("extracts nested noul probability", [this]()
		{
			const TSharedPtr<FJsonObject> Root = ParseObject(TEXT("{\"questions\":{\"decision\":{\"noul\":0.84}}}"));
			double Probability = 0.0;
			TestTrue(TEXT("extracted"), FJevParser::ExtractYesProbability(Root.ToSharedRef(), Probability));
			TestEqual(TEXT("probability"), Probability, 0.84);
		});

		It("rejects malformed JSON", [this]()
		{
			FString Error;
			TestFalse(TEXT("parse fails"), FJevParser::ParseJson(TEXT("not json {"), Error).IsValid());
			TestFalse(TEXT("error set"), Error.IsEmpty());
		});

		It("rejects out-of-range noul", [this]()
		{
			const TSharedPtr<FJsonObject> Root = ParseObject(TEXT("{\"decision\":{\"noul\":1.4}}"));
			double Probability = 0.0;
			TestFalse(TEXT("invalid noul rejected"), FJevParser::ExtractYesProbability(Root.ToSharedRef(), Probability));
		});
	});
}
