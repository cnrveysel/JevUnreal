#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JevSettings.generated.h"

#define UE_API JEV_API

/**
 * Project Settings -> Plugins -> Jev.
 *
 * SECURITY: the API key is stored in plain-text project config and embedded in
 * packaged builds. That is acceptable only for local development/prototyping.
 * For production, use a developer-controlled proxy (see docs/README) and do
 * not ship a TypeSafe API key inside the game.
 */
UCLASS(config=Engine, meta=(DisplayName="Jev"))
class UE_API UJevSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static UJevSettings* Get();

	//~ UDeveloperSettings
	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }

	/** TypeSafe (or proxy) endpoint that accepts Jev state/question requests. */
	UPROPERTY(config, EditAnywhere, Category="Connection")
	FString Endpoint = TEXT("https://api.typesafe.ai/v1/systemone");

	/** Jev model name sent with every request. */
	UPROPERTY(config, EditAnywhere, Category="Connection")
	FString Model = TEXT("jev-latest");

	/**
	 * API key sent as Bearer authorization. Only safe on your machine for
	 * prototyping. Never commit it, and prefer the proxy mode for production.
	 */
	UPROPERTY(config, EditAnywhere, Category="Connection", meta=(SensitiveData))
	FString ApiKey;

	/** Request timeout in seconds. */
	UPROPERTY(config, EditAnywhere, Category="Connection", meta=(ClampMin="1.0", ClampMax="300.0"))
	float RequestTimeoutSeconds = 30.f;

	/** When true, log request lifecycle and parsed results (never secrets). */
	UPROPERTY(config, EditAnywhere, Category="Debug")
	bool bDebugLogging = false;

	/**
	 * Route requests through your own backend instead of calling TypeSafe
	 * directly. Recommended for production; the proxy injects the real API key
	 * and the game never ships one.
	 */
	UPROPERTY(config, EditAnywhere, Category="Proxy")
	bool bUseProxy = false;

	/** Your backend endpoint. Used instead of Endpoint when bUseProxy is true. */
	UPROPERTY(config, EditAnywhere, Category="Proxy")
	FString ProxyEndpoint;
};

#undef UE_API
