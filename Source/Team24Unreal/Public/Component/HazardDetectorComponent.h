// Copyright Team24. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Hazard/HazardTypes.h"
#include "HazardDetectorComponent.generated.h"

/*
 * UHazardDetectorComponent
 * 차량에 붙어서 매 프레임 주행 위험을 감지하는 컴포넌트.
 *
 * 감지하는 위험 (7종):
 *   1. 미끄러짐(Slip)   - 차체 방향과 진행 방향의 각도 차이
 *   2. 횡G(LatG)        - 옆 방향 가속도 (타이어 접지 한계)
 *   3. 휘청거림(Yaw)    - 차체 회전 속도 (스핀/오버스티어)
 *   4. 경로이탈(Lane)   - 도로 중심선에서 벗어난 거리
 *   5. 전복(Rollover)   - 차체 좌우 기울기
 *   6. 급조작(Jerk)     - 가속도의 급격한 변화
 *   7. 추락(FallOff)    - 도로 밖으로 떨어지는 속도
 *
 * 동작 흐름:
 *   매 Tick → 각 위험 측정 → 진입/해제 판정(히스테리시스)
 *           → FHazardEvent 생성 → OnHazardDetected 방송
 *
 * 모든 위험은 진입/해제 기준을 따로 둬서(히스테리시스) 기준선
 * 근처에서 위험/안전이 깜빡이는 것을 방지한다.
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TEAM24UNREAL_API UHazardDetectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHazardDetectorComponent();

	/* 위험 감지 시 방송되는 델리게이트.
	 * 태욱님 데이터 로거에서 이 델리게이트를 구독해 주세요. */
	UPROPERTY(BlueprintAssignable, Category="Hazard")
	FOnHazardDetected OnHazardDetected;

	// 판정 기준: 미끄러짐 (슬립각, 단위: 도)

	/* 진입: 이 각도를 넘으면 위험 시작 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Slip")
	float SlipEnterDeg = 15.f;

	/* 해제: 이 각도 아래로 떨어지면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Slip")
	float SlipExitDeg = 10.f;

	/* 이 속도(cm/s) 미만이면 슬립 계산 안 함 (저속에선 값이 튐) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Slip")
	float MinSpeedForSlip = 150.f;

	// 판정 기준: 횡G (단위: g)

	/* 진입: 이 횡G를 넘으면 위험 시작 (0.6~0.8g가 타이어 한계 근처)
	 * 근데 우리 차는 스포츠가 기준이라 1.2g 정도로 설정함
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|LatG")
	float LatGEnter = 1.2f;

	/* 해제: 이 아래로 떨어지면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|LatG")
	float LatGExit = 0.7f;

	/* 스무딩 강도: 0에 가까울수록 더 부드럽게(노이즈 제거 강함).
	 * 0.1 = 새 값 10%만 반영 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|LatG")
	float LatGSmoothingFactor = 0.1f;

	// 판정 기준: 휘청거림 (Yaw 회전 속도, 단위: 도/초)

	/* 진입: 초당 이 각도보다 빨리 돌면 위험 시작 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Yaw")
	float YawRateEnter = 90.f;

	/* 해제: 이 아래로 떨어지면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Yaw")
	float YawRateExit = 50.f;

	// 판정 기준: 경로이탈 (횡오차, 단위: cm)

	/* 진입: 중심선에서 이만큼 벗어나면 위험 시작 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Lane")
	float LaneDepartEnter = 300.f;

	/* 해제: 이 안쪽으로 돌아오면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Lane")
	float LaneDepartExit = 150.f;

	// 판정 기준: 전복 (차체 기울기, 단위: 도)

	/* 진입: 이 각도보다 기울면 위험 시작 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Rollover")
	float RollEnter = 30.f;

	/* 해제: 이 아래로 돌아오면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Rollover")
	float RollExit = 20.f;

	// 판정 기준: 급조작 (가속도 급변, 단위: g/초)

	/* 진입: 이 값을 넘으면 위험 시작 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Jerk")
	float JerkEnter = 3.0f;

	/* 해제: 이 아래로 떨어지면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|Jerk")
	float JerkExit = 1.5f;

	// 판정 기준: 추락 (낙하 속도, 단위: cm/s)

	/* 진입: 아래로 이 속도보다 빠르게 떨어지면 위험 시작 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|FallOff")
	float FallSpeedEnter = 900.f;

	/* 해제: 낙하 속도가 이 아래로 잦아들면 위험 끝 (진입보다 낮게 — 깜빡임 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hazard|FallOff")
	float FallSpeedExit = 400.f;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 위험별 측정 함수 (Tick에서 매 프레임 호출)

	void CheckSlip();
	void CheckLatG();
	void CheckYaw();
	void CheckLaneDeparture();
	void CheckRollover();
	void CheckJerk();
	void CheckFallOff();

	// 방송 함수 (이벤트 생성 + 델리게이트 broadcast)
	// 슬립/횡G/Yaw는 전용 칸을 채워야 해서 개별 함수,
	// 나머지는 공용 함수(BroadcastSimple) 하나로 처리

	void BroadcastSlip(EHazardPhase Phase, float SlipAngleDeg, AActor* Car, float Speed);
	void BroadcastLatG(EHazardPhase Phase, float LatG, AActor* Car, float Speed);
	void BroadcastYaw(EHazardPhase Phase, float YawRate, AActor* Car, float Speed);
	void BroadcastSimple(EHazardFlags Flag, EHazardPhase Phase,
		AActor* Car, float Speed, const FColor& Color, const FString& Label, float Value);

	// 상태 기억 변수
	// 각 위험이 현재 활성 상태인지 기억 → 진입/해제를 한 번씩만 방송

	bool bSlipActive = false;
	bool bLatGActive = false;
	bool bYawActive  = false;
	bool bLaneActive = false;
	bool bRollActive = false;
	bool bJerkActive = false;
	bool bFallActive = false;

	// 계산용 캐시 변수 (이전 프레임 값 등)

	/* 직전 프레임 속도 — 가속도(속도 변화) 계산에 필요 */
	FVector PrevVelocity = FVector::ZeroVector;

	/* 스무딩된 횡G 값 (노이즈 제거 후) */
	float SmoothedLatG = 0.f;

	/* 직전 프레임 Yaw 각도 — 회전 속도 계산에 필요 */
	float PrevYaw = 0.f;

	/* PrevYaw가 초기화됐는지 (첫 프레임 가짜값 방지) */
	bool bYawInitialized = false;

	/* 급조작 계산용 — 직전 횡가속도 기억 */
	float PrevLatAccelForJerk = 0.f;

	/* PrevLatAccelForJerk가 초기화됐는지 (첫 프레임 가짜값 방지) */
	bool bJerkInitialized = false;
};
