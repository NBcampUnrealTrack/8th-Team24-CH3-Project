// Copyright Team24. All Rights Reserved.

#include "Component/SplineFollowerComponent.h"
#include "Team24Unreal/Team24Unreal.h" //전역 카테고리를 사용하기 위한 헤더파일
#include "Actor/RoadActor.h"
#include "Components/SplineComponent.h"
#include "Vehicle/Base/Team24VehiclePawn.h"

// EngineUtils.h: TActorIterator (월드의 모든 액터 순회)
#include "EngineUtils.h"

// (Team24Unreal.h에 LogTeam24로 사용 가능한 로그 카테고리를 구현)
// DEFINE_LOG_CATEGORY_STATIC(LogTeam24, Log, All);

// 생성자
USplineFollowerComponent::USplineFollowerComponent()
{
	// 매 프레임 Tick을 받을 수 있게 함
	PrimaryComponentTick.bCanEverTick = true;
	// 처음에는 끔 - 도로를 찾은 후에야 활성화
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

//  BeginPlay

void USplineFollowerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 이 컴포넌트의 주인을 Pawn으로 캐스팅
	// 주석해제
	OwnerPawn = Cast<ATeam24VehiclePawn>(GetOwner());
	if (!OwnerPawn.IsValid())
	{
		UE_LOG(LogTeam24, Error, TEXT("Owner is not a Pawn."));
		return;
	}

	// 1. 월드에서 따라갈 RoadActor 찾기
	TargetRoad = FindBestRoadActor();
	if (!TargetRoad.IsValid())
	{
		UE_LOG(LogTeam24, Warning, TEXT("No RoadActor found in world."));
		return;
	}

	// 2. 도로의 USplineComponent 가져와서 검증
	USplineComponent* Spline = GetSpline();
	if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
	{
		UE_LOG(LogTeam24, Warning, TEXT("Road has invalid spline."));
		return;
	}

	// 3. 차량 위치를 도로에 투영해서 시작 거리 계산
	CurrentDistance = ComputeStartDistance(OwnerPawn->GetActorLocation());

	// 4. 초기 목표 속도는 최대 속도 (커브 만나면 자동 감속)
	SmoothedTargetSpeed = MaxSpeed;

	// 5. 모든 준비 완료 → Tick 활성화
	SetComponentTickEnabled(true);

	UE_LOG(LogTeam24, Log, TEXT("Following road '%s', length=%.1f, loop=%s, start=%.1f"),
		*TargetRoad->RoadName,
		Spline->GetSplineLength(),
		Spline->IsClosedLoop() ? TEXT("Y") : TEXT("N"),
		CurrentDistance);
}

//  TickComponent
void USplineFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 안전 검사
	if (!OwnerPawn.IsValid() || !TargetRoad.IsValid()) return;
	if (!GetSpline()) return;

	// 매 프레임 자주 쓸 값들 캐싱
	const FVector VehicleLoc = OwnerPawn->GetActorLocation();
	const float VehicleSpeed = OwnerPawn->GetVelocity().Size();
	const float VehicleYaw = OwnerPawn->GetActorRotation().Yaw;

	// ===== 6단계 파이프라인 =====

	// 1) 도로 위 진행 거리 갱신 + 끝 도달 여부
	if (UpdateProgressDistance(VehicleLoc, VehicleSpeed))
	{
		HandlePathCompleted();
		return;
	}

	// 1-1) 페일세이프: 도로에서 너무 멀어지면 자율주행 일시 정지
	const FVector RoadHere = GetLocationAtDistance(CurrentDistance);
	if (FVector::Dist(VehicleLoc, RoadHere) > MaxRoadDeviation)
	{
		ApplySpeedCommand(0.f, VehicleSpeed);
		ApplySteeringCommand(0.f);
		return;
	}

	// 2) 곡률 측정 (현재 + 전방)
	float CurvHere, CurvAhead;
	SampleCurvatures(CurvHere, CurvAhead);

	// 3) 조향 명령 계산 → 적용
	const FSteeringErrors Errors =
		ComputeSteeringErrors(VehicleLoc, VehicleYaw, VehicleSpeed, CurvHere);
	const float Steer = BlendSteeringInput(Errors, CurvHere);
	ApplySteeringCommand(Steer);

	// 4) 속도 명령 계산 → 적용
	const float TargetSpeed = UpdateTargetSpeed(CurvHere, CurvAhead, DeltaTime);
	ApplySpeedCommand(TargetSpeed, VehicleSpeed);
}

//  초기화 단계

ARoadActor* USplineFollowerComponent::FindBestRoadActor() const
{
	const FVector VehicleLoc = OwnerPawn->GetActorLocation();

	ARoadActor* Best = nullptr;
	float BestDist = SearchRadius;

	// TActorIterator: 월드의 모든 ARoadActor 순회
	for (TActorIterator<ARoadActor> It(GetWorld()); It; ++It)
	{
		ARoadActor* Road = *It;

		// "자율주행에 사용" 체크가 꺼져 있으면 스킵
		if (!Road->bUseForAutopilot) continue;

		USplineComponent* Spline = Road->GetSplineComponent();
		if (!Spline) continue;

		// FindLocationClosestToWorldLocation:
		// 스플라인 위에서 주어진 월드 좌표와 가장 가까운 점의 위치 반환
		const FVector ClosestOnSpline =
			Spline->FindLocationClosestToWorldLocation(VehicleLoc, ESplineCoordinateSpace::World);

		const float Dist = FVector::Dist(ClosestOnSpline, VehicleLoc);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = Road;
		}
	}

	return Best;
}

float USplineFollowerComponent::ComputeStartDistance(const FVector& VehicleLocation) const
{
	USplineComponent* Spline = GetSpline();
	if (!Spline) return 0.f;

	// FindInputKeyClosestToWorldLocation: 차량과 가장 가까운 도로 위 점의 InputKey 반환
	const float InputKey = Spline->FindInputKeyClosestToWorldLocation(VehicleLocation);

	// InputKey를 누적 거리(Distance)로 변환
	return Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
}

//  Tick 단계 함수들

bool USplineFollowerComponent::UpdateProgressDistance(
	const FVector& VehicleLocation, float VehicleSpeed)
{
	USplineComponent* Spline = GetSpline();
	if (!Spline) return true;

	// 차량 위치에 가장 가까운 도로 위 거리값을 다시 계산
	// 매 프레임 갱신하므로 차량이 잠깐 도로를 이탈해도 자동 복구됨
	const float InputKey = Spline->FindInputKeyClosestToWorldLocation(VehicleLocation);
	CurrentDistance = Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);

	// 순환 도로면 끝 개념이 없음
	if (Spline->IsClosedLoop()) return false;

	// 일반 도로: 끝까지 EndOfPathThreshold 이하로 남으면 종료
	const float TotalLength = Spline->GetSplineLength();
	return (TotalLength - CurrentDistance) < EndOfPathThreshold;
}

void USplineFollowerComponent::SampleCurvatures(
	float& OutCurvHere, float& OutCurvAhead) const
{
	OutCurvHere = EstimateCurvature(0.f);              // 현재 위치
	OutCurvAhead = EstimateCurvature(BrakePreviewDist); // 미리 보고 감속용
}

FSteeringErrors USplineFollowerComponent::ComputeSteeringErrors(
	const FVector& VehicleLocation,
	float VehicleYaw,
	float VehicleSpeed,
	float CurvHere) const
{
	FSteeringErrors Errors;

	// ---- 전방 주시 거리 결정 ----
	const float CurvNorm = FMath::Clamp(CurvHere * SharpCurveSensitivity, 0.f, 1.f);
	const float CurvScale = FMath::Lerp(1.f, SharpCurveLookAheadScale, CurvNorm);
	const float LADist = (LookAheadBase + VehicleSpeed * LookAheadSpeedFactor) * CurvScale;

	// 전방 주시 지점의 위치와 방향
	const FVector LAPos = GetLocationAtDistance(CurrentDistance + LADist);
	const FVector LADir = GetDirectionAtDistance(CurrentDistance + LADist);

	// ---- 1. 위치 오차 ----
	const FVector ToLA = (LAPos - VehicleLocation).GetSafeNormal();
	Errors.PositionError = FMath::FindDeltaAngleDegrees(
		VehicleYaw,
		FMath::Atan2(ToLA.Y, ToLA.X) * (180.f / PI));

	// ---- 2. 헤딩 오차 ----
	Errors.HeadingError = FMath::FindDeltaAngleDegrees(
		VehicleYaw,
		FMath::Atan2(LADir.Y, LADir.X) * (180.f / PI));

	// ---- 3. 횡방향 오차 ----
	const FVector RoadPosHere = GetLocationAtDistance(CurrentDistance);
	const FVector RoadDirHere = GetDirectionAtDistance(CurrentDistance);
	const FVector Offset = VehicleLocation - RoadPosHere;

	// 오프셋에서 도로 방향 성분을 뺀 수직 성분을, 도로의 "오른쪽" 방향에 투영
	// → 부호 있는 횡방향 거리 (양수=오른쪽 이탈, 음수=왼쪽 이탈)
	Errors.CrossTrackError = FVector::DotProduct(
		Offset - RoadDirHere * FVector::DotProduct(Offset, RoadDirHere),
		FVector::CrossProduct(FVector::UpVector, RoadDirHere));

	return Errors;
}

float USplineFollowerComponent::BlendSteeringInput(
	const FSteeringErrors& Errors, float CurvHere) const
{
	// 직선 → 헤딩 가중치 높음, 급커브 → 낮음
	const float CurvNorm = FMath::Clamp(CurvHere * SharpCurveSensitivity, 0.f, 1.f);
	const float HdgW = FMath::Lerp(HeadingWeight, HeadingWeightOnSharpCurve, CurvNorm);

	// 위치 오차와 헤딩 오차의 가중 평균
	const float YawCmd = Errors.PositionError * (1.f - HdgW)
	                   + Errors.HeadingError * HdgW;

	// 최종 핸들 입력 [-1, +1]
	return FMath::Clamp(
		YawCmd / MaxYawDelta - Errors.CrossTrackError * CrosstrackGain,
		-1.f, 1.f);
}

float USplineFollowerComponent::UpdateTargetSpeed(
	float CurvHere, float CurvAhead, float DeltaTime)
{
	// 현재 곡률, 전방 곡률 중 더 제한적인(낮은) 속도 채택
	float SpeedLimit = FMath::Min(
		ComputeCurveSpeedLimit(CurvHere),
		ComputeCurveSpeedLimit(CurvAhead));

	// 도로 자체의 SpeedLimit이 설정되어 있으면 적용
	if (TargetRoad.IsValid() && TargetRoad->SpeedLimit > 0.f)
	{
		SpeedLimit = FMath::Min(SpeedLimit, TargetRoad->SpeedLimit);
	}

	// 감속 vs 가속 시 다른 보간 속도
	const float Rate = (SpeedLimit < SmoothedTargetSpeed) ? DecelRate : AccelRate;
	SmoothedTargetSpeed = FMath::FInterpTo(SmoothedTargetSpeed, SpeedLimit, DeltaTime, Rate);

	return SmoothedTargetSpeed;
}

void USplineFollowerComponent::ApplySpeedCommand(float TargetSpeed, float CurrentSpeed)
{
	// 속도 차이를 [-1, +1] 명령으로 변환
	const float Cmd = FMath::Clamp(
		(TargetSpeed - CurrentSpeed) * ThrottleGain, -1.f, 1.f);

	// =====================================================
	//  TODO: Team24Pawn이 만들어지면 여기를 실제 입력 호출로 교체
	//        예시:
	//          ATeam24Pawn* Pawn = Cast<ATeam24Pawn>(OwnerPawn.Get());
	//          if (Cmd > CoastDeadzone)       Pawn->DoThrottle(Cmd);
	//          else if (Cmd < -CoastDeadzone) Pawn->DoBrake(-Cmd);
	//          else { Pawn->DoThrottle(0); Pawn->DoBrake(0); }
	// =====================================================

	//TODO 예시로 구현
	ATeam24VehiclePawn*Pawn = Cast<ATeam24VehiclePawn>(OwnerPawn);
	if (Cmd > CoastDeadzone)
	{
		Pawn->DoThrottle(Cmd);
	}
	else if (Cmd < -CoastDeadzone)
	{
		Pawn->DoBrake(-Cmd);
	}
	else
	{
		Pawn->DoThrottle(0); Pawn->DoBrake(0);
	}
	//

	if (bDebugLogCommands)
	{
		if (Cmd > CoastDeadzone)
		{
			UE_LOG(LogTeam24, VeryVerbose, TEXT("[SPEED] Throttle %.2f (target=%.0f, current=%.0f)"),
			   Cmd, TargetSpeed, CurrentSpeed);
		}
		else if (Cmd < -CoastDeadzone)
		{
			UE_LOG(LogTeam24, VeryVerbose, TEXT("[SPEED] Brake %.2f (target=%.0f, current=%.0f)"),
			   -Cmd, TargetSpeed, CurrentSpeed);
		}
		else
		{
			UE_LOG(LogTeam24, VeryVerbose, TEXT("[SPEED] Coast (target=%.0f, current=%.0f)"),
			   TargetSpeed, CurrentSpeed);
		}
	}
}

void USplineFollowerComponent::ApplySteeringCommand(float Steering)
{
	// =====================================================
	//  TODO: Team24Pawn이 만들어지면 여기를 실제 입력 호출로 교체
	//        예시:
	//          ATeam24Pawn* Pawn = Cast<ATeam24Pawn>(OwnerPawn.Get());
	//          Pawn->DoSteering(Steering);
	// =====================================================

	//ToDO 구현
	ATeam24VehiclePawn*Pawn = Cast<ATeam24VehiclePawn>(OwnerPawn);
	Pawn->DoSteering(Steering);
	//

	if (bDebugLogCommands)
	{
		UE_LOG(LogTeam24, VeryVerbose, TEXT("[STEER] %.3f"), Steering);
	}
}

void USplineFollowerComponent::HandlePathCompleted()
{
	// =====================================================
	//  TODO: Team24Pawn이 만들어지면 여기서 실제 정지 처리
	//        예시:
	//          ATeam24Pawn* Pawn = Cast<ATeam24Pawn>(OwnerPawn.Get());
	//          Pawn->DoThrottle(0.f);
	//          Pawn->DoBrakeStart();
	// =====================================================

	//ToDO 구현
	ATeam24VehiclePawn*Pawn = Cast<ATeam24VehiclePawn>(OwnerPawn);
	Pawn->DoThrottle(0.f);
	Pawn->DoBrakeStart();
	//
	
	UE_LOG(LogTeam24, Log, TEXT("Path completed - vehicle should stop."));
	SetComponentTickEnabled(false); // Tick 끔
}

//  도로 위 위치/방향/곡률 조회

FVector USplineFollowerComponent::GetLocationAtDistance(float Distance) const
{
	USplineComponent* Spline = GetSpline();
	if (!Spline) return FVector::ZeroVector;

	const float WrappedDist = WrapDistance(Distance);

	// USplineComponent의 핵심 함수 - 거리에서 위치를 즉시 얻음
	return Spline->GetLocationAtDistanceAlongSpline(WrappedDist, ESplineCoordinateSpace::World);
}

FVector USplineFollowerComponent::GetDirectionAtDistance(float Distance) const
{
	USplineComponent* Spline = GetSpline();
	if (!Spline) return FVector::ForwardVector;

	const float WrappedDist = WrapDistance(Distance);

	// 그 지점에서의 접선 방향 (단위 벡터)
	return Spline->GetDirectionAtDistanceAlongSpline(WrappedDist, ESplineCoordinateSpace::World);
}

float USplineFollowerComponent::EstimateCurvature(float AheadOffset) const
{
	// 두 지점에서의 방향 벡터
	const FVector D1 = GetDirectionAtDistance(CurrentDistance + AheadOffset);
	const FVector D2 = GetDirectionAtDistance(CurrentDistance + AheadOffset + CurvatureSampleSpan);

	// 단위 벡터 둘의 내적 = cos(각도) → Acos로 각도(라디안) 복원
	return FMath::Acos(FMath::Clamp(FVector::DotProduct(D1, D2), -1.f, 1.f));
}

float USplineFollowerComponent::ComputeCurveSpeedLimit(float Curvature) const
{
	if (Curvature <= KINDA_SMALL_NUMBER) return MaxSpeed;

	// 곡률 → 곡률 반경(R) 변환
	const float Radius = CurvatureSampleSpan / Curvature;

	// V = √(μ × g × R), 언리얼은 cm 단위라 g = 980 cm/s²
	return FMath::Clamp(
		FMath::Sqrt(LateralFriction * 980.f * Radius),
		MinSpeed, MaxSpeed);
}

float USplineFollowerComponent::WrapDistance(float Distance) const
{
	USplineComponent* Spline = GetSpline();
	if (!Spline) return 0.f;

	const float Length = Spline->GetSplineLength();

	if (Spline->IsClosedLoop())
	{
		// 순환 도로: 모듈로 연산으로 한 바퀴 돌아옴
		return FMath::Fmod(FMath::Fmod(Distance, Length) + Length, Length);
	}
	else
	{
		// 일반 도로: [0, Length] 범위로 클램프
		return FMath::Clamp(Distance, 0.f, Length);
	}
}

USplineComponent* USplineFollowerComponent::GetSpline() const
{
	if (!TargetRoad.IsValid()) return nullptr;
	return TargetRoad->GetSplineComponent();
}
