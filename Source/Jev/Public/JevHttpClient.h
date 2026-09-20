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

DECLARE_DELEGATE_OneParam(FJevHttpResponse, const FJevRawResponse&);

/**
 * Thin asynchronous HTTP wrapper around Unreal's HTTP module.
 * Never blocks the game thread and never logs authorization data.
 */
class FJevHttpClient
{
public:
	/**
	 * Sends a POST with the given JSON body. OnResponse may fire from any
	 * thread safe point Unreal invokes the completion handler; the request
	 * object is kept alive until completion via a shared reference.
	 */
	static TSharedRef<IHttpRequest> PostJson(
		const FString& Url,
		const FString& ApiKey,
		const FString& JsonBody,
		float TimeoutSeconds,
		bool bDebugLogging,
		const FJevHttpResponse& OnResponse);
};
