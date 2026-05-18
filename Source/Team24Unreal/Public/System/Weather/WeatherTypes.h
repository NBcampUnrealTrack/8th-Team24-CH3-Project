// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeatherTypes.generated.h"
/**
 *
 */
UENUM(BlueprintType)
enum class EWeather : uint8
{
	Clear    UMETA(DisplayName = "Clear"),
	Rain     UMETA(DisplayName = "Rain"),
	Snow     UMETA(DisplayName = "Snow")
};
