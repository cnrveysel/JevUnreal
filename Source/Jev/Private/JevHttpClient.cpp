#include "JevHttpClient.h"
#include "JevModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformTime.h"
#include "HAL/ThreadSafeCounter.h"

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> FJevHttpClient::PostJson(
	const FString& Url,
	const FString& ApiKey,
	const FString& JsonBody,
	float TimeoutSeconds,
	bool bDebugLogging,
	const FJevHttpResponse& OnResponse)
{
	FHttpModule& Http = FHttpModule::Get();
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http.CreateRequest();
	const double StartTime = FPlatformTime::Seconds();
	const TSharedRef<FThreadSafeCounter, ESPMode::ThreadSafe> CompletionCount = MakeShared<FThreadSafeCounter, ESPMode::ThreadSafe>();

	Request->SetVerb(TEXT("POST"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!ApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	}
	Request->SetTimeout(TimeoutSeconds);
	Request->SetContentAsString(JsonBody);

	Request->OnProcessRequestComplete().BindLambda(
		[OnResponse, StartTime, bDebugLogging, CompletionCount](FHttpRequestPtr CompletedRequest, const FHttpResponsePtr& Response, bool bConnectedSuccessfully)
		{
			if (CompletionCount->Increment() != 1)
			{
				return;
			}
			UE_LOG(LogJev, Verbose, TEXT("[Jev] HTTP completion callback entered (connected=%d)"), bConnectedSuccessfully ? 1 : 0);

			FJevRawResponse Result;
			Result.LatencyMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);

			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				Result.ErrorMessage = TEXT("Connection to Jev endpoint failed");
			}
			else
			{
				Result.HttpStatusCode = Response->GetResponseCode();
				Result.ResponseBody = Response->GetContentAsString();
				if (Result.HttpStatusCode < 200 || Result.HttpStatusCode >= 300)
				{
					Result.ErrorMessage = FString::Printf(TEXT("Jev endpoint returned HTTP %d"), Result.HttpStatusCode);
				}
				else
				{
					Result.bSuccess = true;
				}
			}
			UE_LOG(LogJev, Log, TEXT("[Jev] HTTP status %d, latency %.0f ms"), Result.HttpStatusCode, Result.LatencyMs);

			if (bDebugLogging)
			{
				UE_LOG(LogJev, Log, TEXT("[Jev] Response body length: %d"), Result.ResponseBody.Len());
			}
			OnResponse.ExecuteIfBound(Result, CompletedRequest);
		});

	UE_LOG(LogJev, Verbose, TEXT("[Jev] Starting HTTP request (timeout %.1fs, body length %d)"), TimeoutSeconds, JsonBody.Len());
	const bool bStarted = Request->ProcessRequest();
	UE_LOG(LogJev, Verbose, TEXT("[Jev] ProcessRequest started: %d"), bStarted ? 1 : 0);

	if (!bStarted)
	{
		UE_LOG(LogJev, Warning, TEXT("[Jev] HTTP request failed to start"));
		if (CompletionCount->Increment() == 1)
		{
			FJevRawResponse Result;
			Result.ErrorMessage = TEXT("Failed to start HTTP request to Jev endpoint");
			Result.LatencyMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
			OnResponse.ExecuteIfBound(Result, nullptr);
		}
		return nullptr;
	}

	if (CompletionCount->GetValue() != 0)
	{
		return nullptr;
	}

	return Request;
}
