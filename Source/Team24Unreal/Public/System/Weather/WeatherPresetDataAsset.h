// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeatherPresetDataAsset.generated.h"

/**
 *
 */

class UPhysicalMaterial;
class UNiagaraSystem;
UCLASS()
class TEAM24UNREAL_API UWeatherPresetDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// ==========================================
	// 도로 및 파티클(환경) 세팅
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Physics")
	TObjectPtr<UPhysicalMaterial> RoadPhysicsMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Visual")
	TObjectPtr<UNiagaraSystem> WeatherParticle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Lighting")
	float DirectionalLightIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Lighting")
	FLinearColor DirectionalLightColor = FLinearColor::White;

	// ==========================================
	// 차량 타이어 물리 세팅
	// ==========================================
	// 타이어 마찰력 배율 (맑음, 비, 눈 등 환경에 맞춰 조절)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Wheel")
	float TireFrictionScale = 0.0f;

	// ==========================================================
	// 자율주행 제어 세팅 (백록담)
	// ==========================================================

	// 곡선 안전속도 계산용 마찰 배율 (1.0=원본, 낮을수록 커브를 더 천천히 통과)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Spline")
	float LateralFrictionScale = 1.0f;

	// ==========================================================
	// 센서 제어 세팅 (권남웅님 여기에 제어할 변수들 선언해주시면 됩니다.)
	// ==========================================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Sensor")
	float SensorTest = 1.0f;


};
