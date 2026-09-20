#include "JevSubsystem.h"
#include "JevAsyncActions.h"
#include "JevHttpClient.h"
#include "JevModule.h"
#include "JevParser.h"
#include "JevSettings.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

static FString ValidateConnectionSettings(const UJevSettings* Settings, const FString& EndpointOverride)
{
	if (Settings->bUseProxy)
	{
		if (Settings->ProxyEndpoint.IsEmpty() && EndpointOverride.IsEmpty())
		{
			return TEXT("Jev proxy endpoint is missing; configure Proxy Endpoint or supply an endpoint override");
		}
	}
	else if (Settings->ApiKey.IsEmpty())
	{
		return TEXT("Jev API key is missing; configure it in Project Settings or enable Use Proxy");
	}
	return FString();
}

void UJevSubsystem::Deinitialize()
{
	// Cancellation may invoke the completion delegate immediately, which removes
	// requests from ActiveRequests. Iterate a snapshot to keep that safe.
	const TArray<TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>> RequestsToCancel = ActiveRequests;
	for (const TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>& Request : RequestsToCancel)
	{
		if (Request->GetStatus() == EHttpRequestStatus::Processing)
		{
			Request->CancelRequest();
		}
	}
	ActiveRequests.Empty();

	Super::Deinitialize();
}

FString UJevSubsystem::ResolveEndpoint(const FString& EndpointOverride) const
{
	const UJevSettings* Settings = UJevSettings::Get();
	if (!EndpointOverride.IsEmpty())
	{
		return EndpointOverride;
	}
	if (Settings->bUseProxy && !Settings->ProxyEndpoint.IsEmpty())
	{
		return Settings->ProxyEndpoint;
	}
	return Settings->Endpoint;
}

FString UJevSubsystem::ResolveModel(const FString& ModelOverride) const
{
	return ModelOverride.IsEmpty() ? UJevSettings::Get()->Model : ModelOverride;
}

void UJevSubsystem::RequestYesNo(const FString& State, const FString& Question, float TimeoutOverrideSeconds, const FJevYesNoResult& OnDone, const FString& EndpointOverride)
{
	const UJevSettings* Settings = UJevSettings::Get();
	const bool bDebug = Settings->bDebugLogging;
	const FString Endpoint = ResolveEndpoint(EndpointOverride);
	const FString Model = ResolveModel(TEXT(""));

	const FString ConnectionError = ValidateConnectionSettings(Settings, EndpointOverride);
	if (!ConnectionError.IsEmpty())
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] %s"), *ConnectionError);
		OnDone.ExecuteIfBound(FJevDecisionResult(), ConnectionError);
		return;
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Model);
	Root->SetStringField(TEXT("state"), State);

	const TSharedRef<FJsonObject> QuestionDef = MakeShared<FJsonObject>();
	QuestionDef->SetStringField(TEXT("type"), TEXT("noul"));
	QuestionDef->SetStringField(TEXT("instructions"), Question);

	const TSharedRef<FJsonObject> Questions = MakeShared<FJsonObject>();
	Questions->SetObjectField(TEXT("decision"), QuestionDef);
	Root->SetObjectField(TEXT("questions"), Questions);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const TWeakObjectPtr<UJevSubsystem> WeakThis(this);
	const float Timeout = TimeoutOverrideSeconds > 0.f ? TimeoutOverrideSeconds : Settings->RequestTimeoutSeconds;

	const TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FJevHttpClient::PostJson(
		Endpoint,
		Settings->bUseProxy ? FString() : Settings->ApiKey,
		Body,
		Timeout,
		bDebug,
		FJevHttpResponse::CreateLambda([WeakThis, OnDone, bDebug](const FJevRawResponse& Raw, FHttpRequestPtr CompletedRequest)
		{
			if (UJevSubsystem* StrongThis = WeakThis.Get())
			{
				if (CompletedRequest.IsValid())
				{
					StrongThis->ActiveRequests.Remove(CompletedRequest);
					UE_LOG(LogJev, Verbose, TEXT("[Jev] ActiveRequests remove (%d remaining)"), StrongThis->ActiveRequests.Num());
				}
			}
			else
			{
				return;
			}

			FJevDecisionResult Result;
			Result.RawResponse = Raw.ResponseBody;
			Result.LatencyMs = Raw.LatencyMs;

			if (!Raw.bSuccess)
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Request failed: %s"), *Raw.ErrorMessage);
				OnDone.ExecuteIfBound(Result, Raw.ErrorMessage);
				return;
			}

			FString ParseError;
			const TSharedPtr<FJsonObject> Json = FJevParser::ParseJson(Raw.ResponseBody, ParseError);
			if (!Json.IsValid())
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Response parse failed: %s"), *ParseError);
				OnDone.ExecuteIfBound(Result, ParseError);
				return;
			}

			double YesProbability = 0.0;
			if (!FJevParser::ExtractYesProbability(Json.ToSharedRef(), YesProbability))
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Parser failure: no valid noul probability"));
				OnDone.ExecuteIfBound(Result, TEXT("Response contained no valid Yes probability"));
				return;
			}

			bool bYes = false;
			double Confidence = 0.0;
			FJevParser::NormalizeYesNo(YesProbability, bYes, Confidence);

			Result.Answer = bYes ? EJevYesNo::Yes : EJevYesNo::No;
			Result.YesProbability = static_cast<float>(YesProbability);
			Result.Confidence = static_cast<float>(Confidence);

			if (bDebug)
			{
				UE_LOG(LogJev, Log, TEXT("[Jev] Decision: %s | Probability: %.3f | Latency: %.0f ms"),
					bYes ? TEXT("YES") : TEXT("NO"), YesProbability, Raw.LatencyMs);
			}

			OnDone.ExecuteIfBound(Result, FString());
		}));

	if (!Request.IsValid())
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] No active request to track"));
		return;
	}

	ActiveRequests.Add(Request);
	UE_LOG(LogJev, Verbose, TEXT("[Jev] ActiveRequests add (%d active)"), ActiveRequests.Num());
}

static FString MakeChoiceOptionKey(int32 OptionIndex)
{
	return FString::Printf(TEXT("option_%d"), OptionIndex);
}

static bool ParseChoiceOptionKey(const FString& Choice, int32 OptionCount, int32& OutOptionIndex)
{
	for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
	{
		if (Choice.Equals(MakeChoiceOptionKey(OptionIndex), ESearchCase::CaseSensitive))
		{
			OutOptionIndex = OptionIndex;
			return true;
		}
	}
	return false;
}


void UJevSubsystem::RequestChoose(const FString& State, const FString& Question, const TArray<FString>& Options, float TimeoutOverrideSeconds, const FJevChooseResultDelegate& OnDone, const FString& EndpointOverride)
{
	const UJevSettings* Settings = UJevSettings::Get();
	const bool bDebug = Settings->bDebugLogging;
	const FString Endpoint = ResolveEndpoint(EndpointOverride);
	const FString Model = ResolveModel(TEXT(""));

	if (Options.IsEmpty() || Options.ContainsByPredicate([](const FString& Option) { return Option.IsEmpty(); }))
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] Choose request rejected: options must be nonempty and contain no empty strings"));
		OnDone.ExecuteIfBound(FJevChooseResult(), TEXT("Jev Choose requires a nonempty Options array without empty strings"));
		return;
	}

	const FString ConnectionError = ValidateConnectionSettings(Settings, EndpointOverride);
	if (!ConnectionError.IsEmpty())
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] %s"), *ConnectionError);
		OnDone.ExecuteIfBound(FJevChooseResult(), ConnectionError);
		return;
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Model);
	Root->SetStringField(TEXT("state"), State);

	const TSharedRef<FJsonObject> QuestionDef = MakeShared<FJsonObject>();
	QuestionDef->SetStringField(TEXT("type"), TEXT("choice"));
	QuestionDef->SetStringField(TEXT("instructions"), Question);
	const TSharedRef<FJsonObject> Criteria = MakeShared<FJsonObject>();
	for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
	{
		Criteria->SetStringField(MakeChoiceOptionKey(OptionIndex), Options[OptionIndex]);
	}
	QuestionDef->SetObjectField(TEXT("criteria"), Criteria);

	const TSharedRef<FJsonObject> Questions = MakeShared<FJsonObject>();
	Questions->SetObjectField(TEXT("decision"), QuestionDef);
	Root->SetObjectField(TEXT("questions"), Questions);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);
	UE_LOG(LogJev, Log, TEXT("[Jev] Choose request JSON: %s"), *Body);

	const TWeakObjectPtr<UJevSubsystem> WeakThis(this);
	const float Timeout = TimeoutOverrideSeconds > 0.f ? TimeoutOverrideSeconds : Settings->RequestTimeoutSeconds;

	const TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FJevHttpClient::PostJson(
		Endpoint,
		Settings->bUseProxy ? FString() : Settings->ApiKey,
		Body,
		Timeout,
		bDebug,
		FJevHttpResponse::CreateLambda([WeakThis, OnDone, bDebug, Options](const FJevRawResponse& Raw, FHttpRequestPtr CompletedRequest)
		{
			if (UJevSubsystem* StrongThis = WeakThis.Get())
			{
				if (CompletedRequest.IsValid())
				{
					StrongThis->ActiveRequests.Remove(CompletedRequest);
					UE_LOG(LogJev, Verbose, TEXT("[Jev] ActiveRequests remove (%d remaining)"), StrongThis->ActiveRequests.Num());
				}
			}
			else
			{
				return;
			}

			FJevChooseResult Result;
			Result.RawResponse = Raw.ResponseBody;
			Result.LatencyMs = Raw.LatencyMs;

			if (!Raw.bSuccess)
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] HTTP %d body: %s"), Raw.HttpStatusCode, *Raw.ResponseBody);
				UE_LOG(LogJev, Warning, TEXT("[Jev] Choose request failed: %s"), *Raw.ErrorMessage);
				OnDone.ExecuteIfBound(Result, Raw.ErrorMessage);
				return;
			}

			UE_LOG(LogJev, Log, TEXT("[Jev] Choose HTTP 200 body: %s"), *Raw.ResponseBody);
			FString ParseError;
			const TSharedPtr<FJsonObject> Json = FJevParser::ParseJson(Raw.ResponseBody, ParseError);
			if (!Json.IsValid())
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Choose response parse failed: %s"), *ParseError);
				OnDone.ExecuteIfBound(Result, ParseError);
				return;
			}

			FString Choice;
			double Confidence = 0.0;
			if (!FJevParser::ExtractChoice(Json.ToSharedRef(), Choice, Confidence))
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Choose parser failure: invalid choice response"));
				OnDone.ExecuteIfBound(Result, TEXT("Response contained no valid TypeSafe choice"));
				return;
			}

			int32 SelectedIndex = INDEX_NONE;
			if (!ParseChoiceOptionKey(Choice, Options.Num(), SelectedIndex))
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Choose response did not return a valid supplied option key: %s"), *Choice);
				OnDone.ExecuteIfBound(Result, TEXT("Jev did not return a valid supplied option key"));
				return;
			}

			Result.SelectedOption = Options[SelectedIndex];
			Result.SelectedIndex = SelectedIndex;
			Result.Confidence = static_cast<float>(Confidence);

			if (bDebug)
			{
				UE_LOG(LogJev, Log, TEXT("[Jev] Choose: %s | Confidence: %.3f | Latency: %.0f ms"), *Result.SelectedOption, Confidence, Raw.LatencyMs);
			}

			OnDone.ExecuteIfBound(Result, FString());
		}));

	if (!Request.IsValid())
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] No active Choose request to track"));
		return;
	}

	ActiveRequests.Add(Request);
	UE_LOG(LogJev, Verbose, TEXT("[Jev] ActiveRequests add (%d active)"), ActiveRequests.Num());
}

void UJevSubsystem::RequestGeneric(const FString& State, const FString& RawQuestionsJson, const FString& ModelOverride, const FString& EndpointOverride, const FJevRequestResultDelegate& OnDone)
{
	const UJevSettings* Settings = UJevSettings::Get();
	const FString Endpoint = ResolveEndpoint(EndpointOverride);
	const FString Model = ResolveModel(ModelOverride);

	// Validate the questions JSON before sending.
	FString ParseError;
	const TSharedPtr<FJsonObject> Questions = FJevParser::ParseJson(RawQuestionsJson, ParseError);
	if (!Questions.IsValid())
	{
		FJevRequestResult Result;
		Result.ErrorMessage = FString::Printf(TEXT("Raw Questions JSON is invalid: %s"), *ParseError);
		OnDone.ExecuteIfBound(Result, Result.ErrorMessage);
		return;
	}
	const FString ConnectionError = ValidateConnectionSettings(Settings, EndpointOverride);
	if (!ConnectionError.IsEmpty())
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] %s"), *ConnectionError);
		FJevRequestResult Result;
		Result.ErrorMessage = ConnectionError;
		OnDone.ExecuteIfBound(Result, Result.ErrorMessage);
		return;
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Model);
	Root->SetStringField(TEXT("state"), State);
	Root->SetObjectField(TEXT("questions"), Questions.ToSharedRef());

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const TWeakObjectPtr<UJevSubsystem> WeakThis(this);
	const TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FJevHttpClient::PostJson(
		Endpoint,
		Settings->bUseProxy ? FString() : Settings->ApiKey,
		Body,
		Settings->RequestTimeoutSeconds,
		Settings->bDebugLogging,
		FJevHttpResponse::CreateLambda([WeakThis, OnDone](const FJevRawResponse& Raw, FHttpRequestPtr CompletedRequest)
		{
			if (UJevSubsystem* StrongThis = WeakThis.Get())
			{
				if (CompletedRequest.IsValid())
				{
					StrongThis->ActiveRequests.Remove(CompletedRequest);
					UE_LOG(LogJev, Verbose, TEXT("[Jev] ActiveRequests remove (%d remaining)"), StrongThis->ActiveRequests.Num());
				}
			}
			else
			{
				return;
			}

			FJevRequestResult Result;
			Result.bSuccess = Raw.bSuccess;
			Result.HttpStatusCode = Raw.HttpStatusCode;
			Result.RawJsonResponse = Raw.ResponseBody;
			Result.ErrorMessage = Raw.ErrorMessage;
			Result.LatencyMs = Raw.LatencyMs;
			if (!Result.bSuccess)
			{
				UE_LOG(LogJev, Warning, TEXT("[Jev] Generic request failed: %s"), *Result.ErrorMessage);
			}
			if (Result.bSuccess)
			{
				FString ResponseParseError;
				if (!FJevParser::ParseJson(Raw.ResponseBody, ResponseParseError).IsValid())
				{
					Result.bSuccess = false;
					Result.ErrorMessage = ResponseParseError;
					UE_LOG(LogJev, Warning, TEXT("[Jev] Parser failure: %s"), *ResponseParseError);
				}
				else
				{
					UE_LOG(LogJev, Verbose, TEXT("[Jev] Generic response parsed"));
				}
			}
			OnDone.ExecuteIfBound(Result, Result.ErrorMessage);
		}));

	if (!Request.IsValid())
	{
		UE_LOG(LogJev, Verbose, TEXT("[Jev] No active request to track"));
		return;
	}

	ActiveRequests.Add(Request.ToSharedRef());
	UE_LOG(LogJev, Verbose, TEXT("[Jev] ActiveRequests add (%d active)"), ActiveRequests.Num());
}
