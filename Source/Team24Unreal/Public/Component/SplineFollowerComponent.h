// Copyright Team24. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "System/Weather/WeatherTypes.h"
#include "SplineFollowerComponent.generated.h"

class ATeam24VehiclePawn;
class ARoadActor;
class USplineComponent;

/*
 * FSteeringErrors
 * 조향 계산에 쓰이는 3가지 오차를 묶은 구조체
 *
 * 1. PositionError   - "차의 방향"과 "가야 할 지점 방향"의 각도 차이 (도)
 * 2. HeadingError    - "차의 방향"과 "도로의 방향"의 각도 차이 (도)
 * 3. CrossTrackError - 도로 중심선에서 좌/우로 벗어난 거리 (cm, 부호 있음)
 */

struct FSteeringErrors
{
	float PositionError = 0.f;
	float HeadingError = 0.f;
	float CrossTrackError = 0.f;
};

/*
 * USplineFollowerComponent
 * Pawn에 붙어서 ARoadActor의 스플라인을 따라 자율주행하게 만드는 컴포넌트.
 *
 * 동작 흐름:
 *   1. BeginPlay에서 월드의 ARoadActor를 찾음
 *   2. 매 프레임 차량의 도로 위 진행 거리(Distance) 갱신
 *   3. 그 거리 지점의 도로 방향과 곡률 측정
 *   4. 오차를 줄이는 방향으로 조향
 *   5. 곡률에 맞는 안전 속도로 가/감속
 *
 * 차량 폰(ATeam24Pawn)이 만들어지면 ApplySpeedCommand/조향 부분만 교체하면 됨
 */

UCLASS(ClassGroup=(Team24), meta=(BlueprintSpawnableComponent),
	   DisplayName="Spline Follower")
class TEAM24UNREAL_API USplineFollowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USplineFollowerComponent();

	/* 날씨 변경 시 호출 (Pawn의 OnWeatherChangedDelegate 콜백)
	 *  현재 날씨의 DataAsset에서 LateralFrictionScale을 읽어 적용한다. */
	void ApplyWeatherProfile(EWeather Weather);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	// 초기화 단계

	/* 월드에서 자율주행에 사용할 RoadActor 찾기
	 *  여러 개 있으면 차량과 가장 가까운 것 선택 */
	ARoadActor* FindBestRoadActor() const;

	/* 차량 위치를 도로 위에 투영하여 시작 거리 계산 */
	float ComputeStartDistance(const FVector& VehicleLocation) const;

	//  Tick 단계 (매 프레임)

	/* 차량 위치에 맞춰 CurrentDistance 갱신
	 *  반환값: 경로 끝에 도달했으면 true */
	bool UpdateProgressDistance(const FVector& VehicleLocation, float VehicleSpeed);

	/* 현재/전방 곡률을 동시에 측정 */
	void SampleCurvatures(float& OutCurvHere, float& OutCurvAhead) const;

	/** 조향에 필요한 3가지 오차 계산 */
	FSteeringErrors ComputeSteeringErrors(
		const FVector& VehicleLocation,
		float VehicleYaw,
		float VehicleSpeed,
		float CurvHere) const;

	/* 오차들을 섞어 최종 핸들 입력값 [-1, +1] 계산 */
	float BlendSteeringInput(const FSteeringErrors& Errors, float CurvHere) const;

	/* 곡률 정보로 목표 속도 갱신 (스무딩 포함) */
	float UpdateTargetSpeed(float CurvHere, float CurvAhead, float DeltaTime);

	/* 목표 속도와 현재 속도 차이로 스로틀/브레이크 명령 생성
	 *  현재는 로그로만 출력 - Team24Pawn 만들어지면 실제 입력 호출로 교체 */
	void ApplySpeedCommand(float TargetSpeed, float CurrentSpeed);

	/* 조향 명령 적용 (현재는 로그로만 출력) */
	void ApplySteeringCommand(float Steering);

	/* 경로 끝 도달 시 차량 정지 처리 */
	void HandlePathCompleted();

	/* 터널 진입/이탈 시 호출 (델리게이트 콜백) */
	UFUNCTION()
	void OnTunnelToggled(bool bInTunnel);

	//  도로 위 위치/곡률 조회 (USplineComponent 내장 함수 활용)

	/* 도로 위 특정 거리 지점의 위치 (월드 좌표) */
	FVector GetLocationAtDistance(float Distance) const;

	/* 도로 위 특정 거리 지점에서의 진행 방향 (단위 벡터) */
	FVector GetDirectionAtDistance(float Distance) const;

	/* AheadOffset 지점에서의 곡률(라디안) 추정 */
	float EstimateCurvature(float AheadOffset) const;

	/* 곡률 → 안전 속도 변환 (V = √(μ × g × R)) */
	float ComputeCurveSpeedLimit(float Curvature) const;

	/* 거리 값을 도로 길이에 맞게 보정
	 *  순환 도로면 모듈로 연산, 일반 도로면 [0, 길이] 클램프 */
	float WrapDistance(float Distance) const;

	/* TargetRoad의 USplineComponent를 가져오는 도우미 */
	USplineComponent* GetSpline() const;

	private:

	//  파라미터 (속도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Speed",
		meta=(AllowPrivateAccess="true"))
	float MaxSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Speed",
		meta=(AllowPrivateAccess="true"))
	float MinSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Speed",
		meta=(AllowPrivateAccess="true"))
	float ThrottleGain = 0.002f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Speed",
		meta=(AllowPrivateAccess="true"))
	float DecelRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Speed",
		meta=(AllowPrivateAccess="true"))
	float AccelRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Speed",
		meta=(AllowPrivateAccess="true"))
	float CoastDeadzone = 0.05f;

	//  파라미터 (조향)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(AllowPrivateAccess="true"))
	float LookAheadBase = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(AllowPrivateAccess="true"))
	float LookAheadSpeedFactor = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(AllowPrivateAccess="true"))
	float MaxYawDelta = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(ClampMin="0", ClampMax="1", AllowPrivateAccess="true"))
	float HeadingWeight = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(ClampMin="0", ClampMax="1", AllowPrivateAccess="true"))
	float HeadingWeightOnSharpCurve = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(ClampMin="0.1", ClampMax="1.0", AllowPrivateAccess="true"))
	float SharpCurveLookAheadScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(AllowPrivateAccess="true"))
	float SharpCurveSensitivity = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Steering",
		meta=(AllowPrivateAccess="true"))
	float CrosstrackGain = 0.0015f;

	//  파라미터 (곡률)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Curvature",
		meta=(ClampMin="0.1", ClampMax="2.0", AllowPrivateAccess="true"))
	float LateralFriction = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Curvature",
		meta=(AllowPrivateAccess="true"))
	float CurvatureSampleSpan = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Curvature",
		meta=(AllowPrivateAccess="true"))
	float BrakePreviewDist = 5000.f;

	/* 전방 스캔 시 몇 cm 간격으로 곡률을 측정할지 (작을수록 촘촘, 연산 늘어남) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Curvature",
		meta=(ClampMin="100", AllowPrivateAccess="true"))
	float CurvatureScanStep = 500.f;

	/* 전방 곡률 거리 가중 지수. 클수록 먼 커브를 약하게(완만한 곡선 감속).
	 *  1=선형, 2=완만한 곡선(기본), 3=가까울 때만 급격히 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Curvature",
		meta=(ClampMin="1.0", ClampMax="5.0", AllowPrivateAccess="true"))
	float CurvaturePreviewFalloff = 1.0f;

	//  파라미터 (도로 탐색)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Path",
		meta=(AllowPrivateAccess="true"))
	float SearchRadius = 5000.f;

	/* 경로 끝까지 이만큼 남으면 종료로 판단 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Path",
		meta=(AllowPrivateAccess="true"))
	float EndOfPathThreshold = 100.f;

	/* 도로에서 이만큼 벗어나면 자율주행 일시 정지 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Path",
		meta=(AllowPrivateAccess="true"))
	float MaxRoadDeviation = 1500.f;

	/* 도로 끝에 이만큼 가까워지면 미리 감속 시작 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Path",
		meta=(AllowPrivateAccess="true"))
	float EndApproachDistance = 2000.f;  // 20m 미리 감속

	//  파라미터 (터널)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Tunnel",
		meta=(AllowPrivateAccess="true"))
	float TunnelSpeedScale = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Tunnel",
		meta=(AllowPrivateAccess="true"))
	float TunnelLookAheadScale = 0.6f;

	bool bPathCompleted = false;

	// 터널 진입 전 원래 값 저장 (베이스라인)
	float BaselineMaxSpeed = 0.f;
	float BaselineLookAheadBase = 0.f;
	bool  bBaselineCached = false;

	// 날씨 진입 전 원래 LateralFriction 저장 (베이스라인)
	float BaselineLateralFriction = 0.f;
	bool  bWeatherBaselineCached = false;

	// 날씨 진입 전 원래 DecelRate 저장 (베이스라인)
	float BaselineDecelRate = 0.f;

	// 날씨 진입 전 원래 MaxSpeed 저장 (베이스라인)
	float BaselineMaxSpeedWeather = 0.f;

	// 날씨 진입 전 원래 BrakePreviewDist 저장 (베이스라인)
	float BaselineBrakePreviewDist = 0.f;

	// 날씨 진입 전 원래 EndApproachDistance 저장 (베이스라인)
	float BaselineEndApproachDistance = 0.f;

	// 날씨 진입 전 원래 MinSpeed 저장 (베이스라인)
	float BaselineMinSpeed = 0.f;

	// 터널/날씨 MaxSpeed 합성용
	// 지금 터널 안인지 (OnTunnelToggled가 갱신)
	bool  bIsInTunnelNow = false;
	// 터널 배율을 곱하기 전, 순수 날씨 기준 MaxSpeed (ApplyWeatherProfile가 갱신)
	float WeatherBaseMaxSpeed = 0.f;

	// 현재 실제 날씨 (터널 안에선 Clear로 취급하되, 원래 날씨는 여기 기억)
	EWeather CurrentWeather = EWeather::Clear;

	/* 디버그용 - 매 프레임 명령을 로그로 출력할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Autopilot|Debug",
		meta=(AllowPrivateAccess="true"))
	bool bDebugLogCommands = false;

	//  런타임 상태

	/* 따라갈 도로 액터의 약한 참조 */
	UPROPERTY()
	TWeakObjectPtr<ARoadActor> TargetRoad;

	/* 차량 폰의 약한 참조  */
	//VechiclePawn 적용완료
	UPROPERTY()
	TWeakObjectPtr<ATeam24VehiclePawn> OwnerPawn;

	/* 도로 시작점부터 차량의 현재 위치까지의 거리 (cm) */
	float CurrentDistance = 0.f;

	/* 부드럽게 보간된 목표 속도 (cm/s) */
	float SmoothedTargetSpeed = 0.f;

	/* 가장 최근 계산된 경로 횡오차 (cm, 부호 있음).
	 * const 함수에서 저장하므로 mutable. HazardDetector가 읽음 */
	mutable float LatestCrossTrackError = 0.f;

public:
	/* 현재 경로 중심선에서 벗어난 거리 (cm). 양수=오른쪽, 음수=왼쪽 */
	float GetCrossTrackError() const { return LatestCrossTrackError; }
};
