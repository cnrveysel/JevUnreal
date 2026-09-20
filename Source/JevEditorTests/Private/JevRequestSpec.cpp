#include "Misc/AutomationTest.h"
#include "Templates/UnrealTemplate.h"
#include "Engine/GameInstance.h"
#include "JevSettings.h"
#include "JevSubsystem.h"

struct FJevCompletionCapture
{
	int32 Count = 0;
	FString Error;
	bool bSucceeded = true;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJevMissingKeyYesNoTest, "Jev.Request.MissingKeyYesNo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJevMissingKeyYesNoTest::RunTest(const FString& Parameters)
{
	UJevSettings* Settings = UJevSettings::Get();
	TGuardValue<bool> RestoreProxy(Settings->bUseProxy, false);
	TGuardValue<FString> RestoreKey(Settings->ApiKey, FString());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UJevSubsystem* Subsystem = NewObject<UJevSubsystem>(GameInstance);
	AddExpectedError(TEXT("Jev API key is missing"), EAutomationExpectedErrorFlags::Contains, 2);

	const TSharedRef<FJevCompletionCapture> Capture = MakeShared<FJevCompletionCapture>();
	const FJevYesNoResult OnDone = FJevYesNoResult::CreateLambda([Capture](FJevDecisionResult, const FString& InError)
	{
		++Capture->Count;
		Capture->Error = InError;
	});
	Subsystem->RequestYesNo(TEXT("state"), TEXT("question"), 0.f,
		OnDone);

	TestEqual(TEXT("completed once"), Capture->Count, 1);
	TestTrue(TEXT("missing key error"), Capture->Error.Contains(TEXT("API key is missing")));

	Capture->Count = 0;
	Capture->Error.Empty();
	Subsystem->RequestYesNo(TEXT("state"), TEXT("question"), 0.f, OnDone, TEXT("https://api.typesafe.ai/v1/systemone"));
	TestEqual(TEXT("override also completed once"), Capture->Count, 1);
	TestTrue(TEXT("override still requires key"), Capture->Error.Contains(TEXT("API key is missing")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJevMissingKeyGenericTest, "Jev.Request.MissingKeyGeneric",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJevMissingKeyGenericTest::RunTest(const FString& Parameters)
{
	UJevSettings* Settings = UJevSettings::Get();
	TGuardValue<bool> RestoreProxy(Settings->bUseProxy, false);
	TGuardValue<FString> RestoreKey(Settings->ApiKey, FString());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UJevSubsystem* Subsystem = NewObject<UJevSubsystem>(GameInstance);
	AddExpectedError(TEXT("Jev API key is missing"), EAutomationExpectedErrorFlags::Contains, 2);

	const TSharedRef<FJevCompletionCapture> Capture = MakeShared<FJevCompletionCapture>();
	const FJevRequestResultDelegate OnDone = FJevRequestResultDelegate::CreateLambda([Capture](FJevRequestResult Result, const FString& InError)
	{
		++Capture->Count;
		Capture->Error = InError;
		Capture->bSucceeded = Result.bSuccess;
	});
	Subsystem->RequestGeneric(TEXT("state"), TEXT("{\"decision\":{\"type\":\"noul\"}}"), FString(), FString(),
		OnDone);

	TestEqual(TEXT("completed once"), Capture->Count, 1);
	TestFalse(TEXT("result failed"), Capture->bSucceeded);
	TestTrue(TEXT("missing key error"), Capture->Error.Contains(TEXT("API key is missing")));

	Capture->Count = 0;
	Capture->Error.Empty();
	Subsystem->RequestGeneric(TEXT("state"), TEXT("{\"decision\":{\"type\":\"noul\"}}"), FString(), TEXT("https://api.typesafe.ai/v1/systemone"), OnDone);
	TestEqual(TEXT("override also completed once"), Capture->Count, 1);
	TestTrue(TEXT("override still requires key"), Capture->Error.Contains(TEXT("API key is missing")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJevMissingProxyEndpointTest, "Jev.Request.MissingProxyEndpoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJevMissingProxyEndpointTest::RunTest(const FString& Parameters)
{
	UJevSettings* Settings = UJevSettings::Get();
	TGuardValue<bool> RestoreProxy(Settings->bUseProxy, true);
	TGuardValue<FString> RestoreEndpoint(Settings->ProxyEndpoint, FString());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UJevSubsystem* Subsystem = NewObject<UJevSubsystem>(GameInstance);
	AddExpectedError(TEXT("Jev proxy endpoint is missing"), EAutomationExpectedErrorFlags::Contains, 2);

	const TSharedRef<FJevCompletionCapture> Capture = MakeShared<FJevCompletionCapture>();
	Subsystem->RequestYesNo(TEXT("state"), TEXT("question"), 0.f,
		FJevYesNoResult::CreateLambda([Capture](FJevDecisionResult, const FString& InError)
		{
			++Capture->Count;
			Capture->Error = InError;
		}));
	TestEqual(TEXT("Yes/No completed once"), Capture->Count, 1);
	TestTrue(TEXT("Yes/No proxy error"), Capture->Error.Contains(TEXT("proxy endpoint is missing")));

	Capture->Count = 0;
	Capture->Error.Empty();
	Subsystem->RequestGeneric(TEXT("state"), TEXT("{\"decision\":{\"type\":\"noul\"}}"), FString(), FString(),
		FJevRequestResultDelegate::CreateLambda([Capture](FJevRequestResult, const FString& InError)
		{
			++Capture->Count;
			Capture->Error = InError;
		}));
	TestEqual(TEXT("generic completed once"), Capture->Count, 1);
	TestTrue(TEXT("generic proxy error"), Capture->Error.Contains(TEXT("proxy endpoint is missing")));
	return true;
}
