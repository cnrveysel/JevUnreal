#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "HttpModule.h"

/** Raw transport result used internally by the HTTP layer. */
struct FJevRawResponse
{
	bool bSuccess = false;
	int32 HttpStatusCode = 0;
	FString ResponseBody;
	FString ErrorMessage;
	float LatencyMs = 0.f;
};

DECLARE_DELEGATE_TwoParams(FJevHttpResponse, const FJevRawResponse&, FHttpRequestPtr);

/**
 * Thin asynchronous HTTP wrapper around Unreal's HTTP module.
 * Never blocks the game thread and never logs authorization data.
 */
class FJevHttpClient
{
public:
	/**
	 * Sends a POST with the given JSON body. OnResponse fires exactly once
	 * when the request completes, fails to start, or times out. Returns the
	 * live request, or null when ProcessRequest() failed (OnResponse has
	 * already fired with an error in that case).
	 */
	static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> PostJson(
		const FString& Url,
		const FString& ApiKey,
		const FString& JsonBody,
		float TimeoutSeconds,
		bool bDebugLogging,
		const FJevHttpResponse& OnResponse);
};
