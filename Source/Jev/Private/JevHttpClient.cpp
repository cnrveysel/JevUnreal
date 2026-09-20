#include "JevHttpClient.h"
#include "JevModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformTime.h"

TSharedRef<IHttpRequest> FJevHttpClient::PostJson(
	const FString& Url,
	const FString& ApiKey,
	const FString& JsonBody,
	float TimeoutSeconds,
	bool bDebugLogging,
	const FJevHttpResponse& OnResponse)
{
	FHttpModule& Http = FHttpModule::Get();
	const TSharedRef<IHttpRequest> Request = Http.CreateRequest();
	const double StartTime = FPlatformTime::Seconds();

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
		[OnResponse, StartTime, bDebugLogging](FHttpRequestPtr, const FHttpResponsePtr& Response, bool bConnectedSuccessfully)
		{
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

			if (bDebugLogging)
			{
				UE_LOG(LogJev, Log, TEXT("Response status: %d, latency: %.1f ms, body length: %d"), Result.HttpStatusCode, Result.LatencyMs, Result.ResponseBody.Len());
			}
			OnResponse.ExecuteIfBound(Result);
		});

	if (bDebugLogging)
	{
		UE_LOG(LogJev, Log, TEXT("Sending request to %s (timeout %.1fs, body length %d)"), *Url, TimeoutSeconds, JsonBody.Len());
	}

	Request->ProcessRequest();
	return Request;
}
