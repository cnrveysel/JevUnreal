#include "JevSubsystem.h"
#include "JevAsyncActions.h"
#include "JevHttpClient.h"
#include "JevModule.h"
#include "JevParser.h"
#include "JevSettings.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

void UJevSubsystem::Deinitialize()
{
	for (const TSharedRef<IHttpRequest>& Request : ActiveRequests)
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

void UJevSubsystem::RequestYesNo(const FString& State, const FString& Question, float TimeoutOverrideSeconds, const FJevYesNoResult& OnDone)
{
	const UJevSettings* Settings = UJevSettings::Get();
	const bool bDebug = Settings->bDebugLogging;
	const FString Endpoint = ResolveEndpoint(TEXT(""));
	const FString Model = ResolveModel(TEXT(""));

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
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FJevHttpClient::PostJson(
		Endpoint,
		Settings->bUseProxy ? FString() : Settings->ApiKey,
		Body,
		Timeout,
		bDebug,
		FJevHttpResponse::CreateLambda([WeakThis, OnDone, bDebug](const FJevRawResponse& Raw)
		{
			FJevDecisionResult Result;
			Result.RawResponse = Raw.ResponseBody;
			Result.LatencyMs = Raw.LatencyMs;

			if (!Raw.bSuccess)
			{
				OnDone.ExecuteIfBound(Result, Raw.ErrorMessage);
				return;
			}

			FString ParseError;
			const TSharedPtr<FJsonObject> Json = FJevParser::ParseJson(Raw.ResponseBody, ParseError);
			if (!Json.IsValid())
			{
				OnDone.ExecuteIfBound(Result, ParseError);
				return;
			}

			double YesProbability = 0.0;
			if (!FJevParser::ExtractYesProbability(Json.ToSharedRef(), YesProbability))
			{
				if (bDebug)
				{
					UE_LOG(LogJev, Warning, TEXT("No valid noul probability (0..1) found in response."));
				}
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
				UE_LOG(LogJev, Log, TEXT("Decision: %s | Probability: %.3f | Latency: %.0f ms"),
					bYes ? TEXT("YES") : TEXT("NO"), YesProbability, Raw.LatencyMs);
			}

			OnDone.ExecuteIfBound(Result, FString());
		}));

	ActiveRequests.Add(Request);
	Request->OnProcessRequestComplete().BindLambda([WeakThis, Request](FHttpRequestPtr, const FHttpResponsePtr&, bool)
	{
		if (UJevSubsystem* StrongThis = WeakThis.Get())
		{
			StrongThis->ActiveRequests.Remove(Request);
		}
	});
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

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Model);
	Root->SetStringField(TEXT("state"), State);
	Root->SetObjectField(TEXT("questions"), Questions.ToSharedRef());

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const TWeakObjectPtr<UJevSubsystem> WeakThis(this);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FJevHttpClient::PostJson(
		Endpoint,
		Settings->bUseProxy ? FString() : Settings->ApiKey,
		Body,
		Settings->RequestTimeoutSeconds,
		Settings->bDebugLogging,
		FJevHttpResponse::CreateLambda([OnDone](const FJevRawResponse& Raw)
		{
			FJevRequestResult Result;
			Result.bSuccess = Raw.bSuccess;
			Result.HttpStatusCode = Raw.HttpStatusCode;
			Result.RawJsonResponse = Raw.ResponseBody;
			Result.ErrorMessage = Raw.ErrorMessage;
			Result.LatencyMs = Raw.LatencyMs;
			OnDone.ExecuteIfBound(Result, Raw.ErrorMessage);
		}));

	ActiveRequests.Add(Request);
	Request->OnProcessRequestComplete().BindLambda([WeakThis, Request](FHttpRequestPtr, const FHttpResponsePtr&, bool)
	{
		if (UJevSubsystem* StrongThis = WeakThis.Get())
		{
			StrongThis->ActiveRequests.Remove(Request);
		}
	});
}
