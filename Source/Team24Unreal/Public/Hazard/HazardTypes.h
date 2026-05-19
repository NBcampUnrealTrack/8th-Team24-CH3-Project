#pragma once

#include "CoreMinimal.h"
#include "HazardTypes.generated.h"

// 위험 종류 — 체크리스트. 여러 개 동시 체크 가능.
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EHazardFlags : uint8
{
	None            = 0        UMETA(Hidden),
	Skid            = 1 << 0,   // 미끄러짐 (슬립각 과다)
	HighLateralG    = 1 << 1,   // 횡방향 G 한계 근접
	YawInstability  = 1 << 2,   // 휘청거림 (Yaw)
	LaneDeparture   = 1 << 3,   // 경로 중심선 이탈
	RolloverRisk    = 1 << 4,   // 전복 위험 (Roll 과다)
	HarshManeuver   = 1 << 5,   // 급조작 (jerk 과다)
	FallOff         = 1 << 6,
};
ENUM_CLASS_FLAGS(EHazardFlags);

// 위험의 어느 시점인지 — 시작 / 진행 중 / 끝
UENUM(BlueprintType)
enum class EHazardPhase : uint8
{
	Enter   UMETA(DisplayName = "진입"),
	Sustain UMETA(DisplayName = "지속"),
	Exit    UMETA(DisplayName = "해제"),
};

// 위험 이벤트 1건
USTRUCT(BlueprintType)
struct FHazardEvent
{
	GENERATED_BODY()

	// 1. 언제
	UPROPERTY(BlueprintReadOnly)
	double TimeStamp = 0.0;

	// 2. 무슨 위험인지
	UPROPERTY(BlueprintReadOnly, meta = (Bitmask, BitmaskEnum = "EHazardFlags"))
	int32 ActiveFlags = 0;

	// 5. 어느 시점인지
	UPROPERTY(BlueprintReadOnly)
	EHazardPhase Phase = EHazardPhase::Enter;

	// 3. 어디서
	UPROPERTY(BlueprintReadOnly)
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Speed = 0.f;

	// 4. 얼마나 심했는지
	UPROPERTY(BlueprintReadOnly)
	float SlipAngleDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float LateralG = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float YawRate = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CrossTrackError = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float RollDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Severity = 0.f;
};

// 위험 발생 시 이 채널로 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnHazardDetected, const FHazardEvent&, HazardEvent);
