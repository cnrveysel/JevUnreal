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
