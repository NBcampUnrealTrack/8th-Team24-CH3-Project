// Copyright Team24. All Rights Reserved.

#include "Component/SplineFollowerComponent.h"
#include "Team24Unreal/Team24Unreal.h"
#include "Actor/RoadActor.h"
#include "Components/SplineComponent.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "System/Weather/WeatherSubsystem.h"
#include "System/Weather/WeatherPresetDataAsset.h"

// EngineUtils.h: TActorIterator (월드의 모든 액터 순회)
#include "EngineUtils.h"

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
	OwnerPawn = Cast<ATeam24VehiclePawn>(GetOwner());
	if (!OwnerPawn.IsValid())
	{
		UE_LOG(LogTeam24, Error, TEXT("Owner is not a Pawn."));
		return;
	}

	// 추가: 터널 델리게이트 구독
	OwnerPawn->OnTunnelToggleDelegate.AddUObject(
		this, &USplineFollowerComponent::OnTunnelToggled);

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
}

//  TickComponent
void USplineFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 안전 검사
	if (!OwnerPawn.IsValid() || !TargetRoad.IsValid()) return;
	if (!GetSpline()) return;

	// 이미 끝 도달했으면 브레이크만 유지
	if (bPathCompleted)
	{
		return;
	}

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

	// 페일세이프: 도로에서 너무 멀어지면 자율주행 일시 정지
	// 값들은 최종적으로 정리되면 그거에 맞춰서 재수정해야함
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

	// 전방 구간 스캔 + 거리 가중.
	// 각 지점 곡률에 (1 - 거리비율)^Falloff 가중을 곱해,
	// 먼 커브는 약하게 / 가까운 커브는 강하게 반영한다.
	// → 커브에 다가갈수록 가중이 서서히 커져 '완만한 감속 곡선'을 만든다.
	float MaxWeightedCurv = 0.f;

	for (float Scan = 0.f; Scan <= BrakePreviewDist; Scan += CurvatureScanStep)
	{
		const float Curv = EstimateCurvature(Scan);

		// 거리 비율: 0(코앞) ~ 1(스캔 끝)
		const float DistRatio = (BrakePreviewDist > KINDA_SMALL_NUMBER)
			? (Scan / BrakePreviewDist) : 0.f;

		// 가중치: 가까울수록 1, 멀수록 0 (Falloff가 곡선 모양 결정)
		const float Weight = FMath::Pow(1.f - DistRatio, CurvaturePreviewFalloff);

		MaxWeightedCurv = FMath::Max(MaxWeightedCurv, Curv * Weight);
	}

	OutCurvAhead = MaxWeightedCurv;
}

FSteeringErrors USplineFollowerComponent::ComputeSteeringErrors(
	const FVector& VehicleLocation,
	float VehicleYaw,
	float VehicleSpeed,
	float CurvHere) const
{
	FSteeringErrors Errors;

	// 전방 주시 거리 결정
	const float CurvNorm = FMath::Clamp(CurvHere * SharpCurveSensitivity, 0.f, 1.f);
	const float CurvScale = FMath::Lerp(1.f, SharpCurveLookAheadScale, CurvNorm);
	const float LADist = (LookAheadBase + VehicleSpeed * LookAheadSpeedFactor) * CurvScale;

	// 전방 주시 지점의 위치와 방향
	const FVector LAPos = GetLocationAtDistance(CurrentDistance + LADist);
	const FVector LADir = GetDirectionAtDistance(CurrentDistance + LADist);

	// 1. 위치 오차
	const FVector ToLA = (LAPos - VehicleLocation).GetSafeNormal();
	Errors.PositionError = FMath::FindDeltaAngleDegrees(
		VehicleYaw,
		FMath::Atan2(ToLA.Y, ToLA.X) * (180.f / PI));

	// 2. 헤딩 오차
	Errors.HeadingError = FMath::FindDeltaAngleDegrees(
		VehicleYaw,
		FMath::Atan2(LADir.Y, LADir.X) * (180.f / PI));

	// 3. 횡방향 오차
	const FVector RoadPosHere = GetLocationAtDistance(CurrentDistance);
	const FVector RoadDirHere = GetDirectionAtDistance(CurrentDistance);
	const FVector Offset = VehicleLocation - RoadPosHere;

	// 오프셋에서 도로 방향 성분을 뺀 수직 성분을, 도로의 오른쪽 방향에 투영
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

	// 도로 끝 미리 감속 (순환 도로 아닐 때만)
	USplineComponent* Spline = GetSpline();
	if (Spline && !Spline->IsClosedLoop())
	{
		const float Remaining = Spline->GetSplineLength() - CurrentDistance;

		if (Remaining < EndApproachDistance)
		{
			// 0 ~ 1 비율 (1: 멀음, 0: 도로 끝)
			const float Ratio = FMath::Clamp(Remaining / EndApproachDistance, 0.f, 1.f);

			// 끝에 가까울수록 0에 수렴 (MinSpeed 무시)
			const float EndSpeedLimit = MaxSpeed * Ratio;

			SpeedLimit = FMath::Min(SpeedLimit, EndSpeedLimit);
		}
	}

	// 감속 vs 가속 시 다른 보간 속도
	const float Rate = (SpeedLimit < SmoothedTargetSpeed) ? DecelRate : AccelRate;
	SmoothedTargetSpeed = FMath::FInterpTo(SmoothedTargetSpeed, SpeedLimit, DeltaTime, Rate);

	return SmoothedTargetSpeed;
}

void USplineFollowerComponent::ApplySpeedCommand(float TargetSpeed, float CurrentSpeed)
{
	ATeam24VehiclePawn* Pawn = OwnerPawn.Get();
	if (!Pawn) return;

	// 속도 차이를 [-1, +1] 명령으로 변환
	const float Cmd = FMath::Clamp(
		(TargetSpeed - CurrentSpeed) * ThrottleGain, -1.f, 1.f);

	if (Cmd > CoastDeadzone)
	{
		// 가속 (DoThrottle 내부에서 브레이크 0으로 설정함)
		Pawn->DoThrottle(Cmd);
	}
	else if (Cmd < -CoastDeadzone)
	{
		// 감속 (DoBrake 내부에서 가속 0으로 설정함)
		Pawn->DoBrake(-Cmd);
	}
	else
	{
		// 데드존: 가속/브레이크 모두 떼기 (관성 주행)
		Pawn->DoThrottle(0.f);
	}
}

void USplineFollowerComponent::ApplySteeringCommand(float Steering)
{
	// 차량 핸들 입력 적용
	ATeam24VehiclePawn* Pawn = OwnerPawn.Get();
	if (!Pawn) return;

	Pawn->DoSteering(Steering);
}

void USplineFollowerComponent::HandlePathCompleted()
{
	// 도로 끝 도달 - 차량 완전 정지
	ATeam24VehiclePawn* Pawn = OwnerPawn.Get();
	if (!Pawn) return;

	//물리 시뮬레이션 완전 중지하는 방법은 주석처리 했습니다.

	//Pawn->DoThrottle(0.f);    // 가속 페달 떼기
	//Pawn->DoSteering(0.f);    // 핸들 중앙으로
	//Pawn->DoHandbrakeStart();
	//Pawn->DoBrakeStart();     // 후미등 켜기 (시각 효과)


	// 물리 시뮬레이션 완전 중지
	//if (USkeletalMeshComponent* Mesh = Pawn->GetMesh())
	//{
	//	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	//	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	//	Mesh->SetSimulatePhysics(false);  // 핵심: 물리 자체 OFF
	//}

	//UE_LOG(LogTemp, Log, TEXT("Path completed - vehicle stopping."));
	//bPathCompleted = true;
	//SetComponentTickEnabled(false);


	//백록담님 블로그 "[트러블슈팅] 차량을 도로 끝에서 멈추게 하기" 부분에서 2차변경 파트 부분 사용했습니다.
	Pawn->DoThrottle(0.f);
	Pawn->DoBrake(1.f);       // 실제 브레이크 100%
	Pawn->DoSteering(0.f);
	Pawn->DoBrakeStart();     // 후미등
	bPathCompleted = true;
	SetComponentTickEnabled(false);

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

	if (USplineComponent* Spline = GetSpline())
	{
		const float Len = Spline->GetSplineLength();
		const float Target = CurrentDistance + AheadOffset;
	}

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

void USplineFollowerComponent::OnTunnelToggled(bool bInTunnel)
{
	// 터널 진입 전 LookAhead 베이스라인은 1회만 저장 (LookAhead는 날씨와 무관)
	if (!bBaselineCached)
	{
		BaselineLookAheadBase = LookAheadBase;
		bBaselineCached = true;
	}

	// 터널 상태 갱신 (날씨 합성이 이 값을 참조)
	bIsInTunnelNow = bInTunnel;

	// 터널 상태가 바뀌면 유효 날씨도 바뀜(터널↔노출).
	// 실제 날씨로 ApplyWeatherProfile 재호출 → 마찰/감속/Min/Preview 전부 재계산.
	ApplyWeatherProfile(CurrentWeather);

	// MaxSpeed는 항상 '날씨 기준값 × 터널 배율'로 합성.
	// WeatherBaseMaxSpeed가 아직 0이면(날씨 콜백 전) 현재 MaxSpeed를 기준으로.
	const float WeatherBase =
		(WeatherBaseMaxSpeed > KINDA_SMALL_NUMBER) ? WeatherBaseMaxSpeed : MaxSpeed;

	if (bInTunnel)
	{
		MaxSpeed = WeatherBase * TunnelSpeedScale;
		LookAheadBase = BaselineLookAheadBase * TunnelLookAheadScale;

		// 즉시 감속 (현재 속도가 새 MaxSpeed보다 빠르면 클램프)
		SmoothedTargetSpeed = FMath::Min(SmoothedTargetSpeed, MaxSpeed);
	}
	else
	{
		MaxSpeed = WeatherBase;                 // 터널 배율 해제 = 날씨 기준값 그대로
		LookAheadBase = BaselineLookAheadBase;
	}

	UE_LOG(LogTeam24, Log,
		TEXT("Autopilot tunnel mode: %s, MaxSpeed=%.0f, LookAhead=%.0f, WeatherBase=%.0f"),
		bInTunnel ? TEXT("ON") : TEXT("OFF"), MaxSpeed, LookAheadBase, WeatherBase);
}

void USplineFollowerComponent::ApplyWeatherProfile(EWeather Weather)
{
	// 실제 날씨 기억 (터널을 나갈 때 이 값으로 복귀)
	CurrentWeather = Weather;

	// 터널 안이면 날씨를 Clear로 간주.
	// (터널은 비/눈이 안 들이쳐 노면이 안 젖음 → 날씨 영향 없음)
	const EWeather EffectiveWeather = bIsInTunnelNow ? EWeather::Clear : Weather;

	// 처음 호출 시 원본 LateralFriction을 한 번만 저장
	// (이후 어떤 날씨로 바뀌든 항상 이 원본 기준으로 배율 적용)
	if (!bWeatherBaselineCached)
	{
		BaselineLateralFriction = LateralFriction;
		// 추가: 원본 DecelRate도 같이 백업
		BaselineDecelRate = DecelRate;
		// 속도 제한
		BaselineMaxSpeedWeather = MaxSpeed;
		// 날씨별 전방주시 변경
		BaselineBrakePreviewDist = BrakePreviewDist;
		// 날씨별 정지거리 변경
		BaselineEndApproachDistance = EndApproachDistance;
		// 날씨별 최소속도 설정
		BaselineMinSpeed = MinSpeed;
		bWeatherBaselineCached = true;
	}

	// 월드의 WeatherSubsystem에서 현재 날씨의 DataAsset을 가져온다
	float FrictionScale = 1.f;  // 기본값: DataAsset 없으면 원본 그대로 (안전)

	if (UWorld* World = GetWorld())
	{
		if (UWeatherSubsystem* WeatherSub = World->GetSubsystem<UWeatherSubsystem>())
		{
			if (UWeatherPresetDataAsset* Preset = WeatherSub->GetCurrentWeatherPreset())
			{
				FrictionScale = Preset->LateralFrictionScale;
			}
			else
			{
				UE_LOG(LogTeam24, Warning,
					TEXT("ApplyWeatherProfile: WeatherPreset is null. Using baseline friction."));
			}
		}
	}

	// 원본 × 배율 (현재값에 곱하지 않음 - 누적 오염 방지)
	LateralFriction = BaselineLateralFriction * FrictionScale;

	// 마찰이 낮은 날씨일수록 더 공격적으로 감속
	// FrictionScale이 작을수록(눈길) DecelRate를 크게
	const float DecelBoost = FMath::Clamp(1.f / (FrictionScale * FrictionScale), 1.f, 12.f);
	DecelRate = BaselineDecelRate * DecelBoost;

	// 날씨별 최고속도 상한 (곡선 있는 일반도로 기준)
	// Clear ~80km/h / Rain ~60km/h / Snow ~50km/h
	float WeatherMaxSpeed;
	switch (EffectiveWeather)
	{
	case EWeather::Rain:
		WeatherMaxSpeed = 1700.f;
		break;
	case EWeather::Snow:
		WeatherMaxSpeed = 1400.f;
		break;
	default:  // Clear
		WeatherMaxSpeed = 2200.f;
		break;
	}

	// 순수 날씨 기준값 저장 (터널 배율 미적용 — 합성의 기준점)
	WeatherBaseMaxSpeed = WeatherMaxSpeed;

	// 최종 MaxSpeed = 날씨 기준값 × (터널 안이면 터널 배율)
	// 터널이 바뀌든 날씨가 바뀌든 항상 이 공식으로 수렴 → 덮어쓰기 충돌 제거
	MaxSpeed = WeatherBaseMaxSpeed * (bIsInTunnelNow ? TunnelSpeedScale : 1.f);

	// 날씨가 나쁠수록 곡선을 더 멀리서 미리 보고 감속 시작
	float PreviewScale = 1.f;
	switch (Weather)
	{
	case EWeather::Rain:
		PreviewScale = 1.4f;
		break;
	case EWeather::Snow:
		PreviewScale = 1.6f;
		break;
	default:  // Clear
		PreviewScale = 1.f;
		break;
	}
	BrakePreviewDist = BaselineBrakePreviewDist * PreviewScale;

	// 날씨가 미끄러울수록 더 일찍 멈출 준비 (눈길 제동거리가 길다)
	float EndApproachScale = 1.f;
	switch (Weather)
	{
	case EWeather::Rain:
		EndApproachScale = 1.5f;
		break;
	case EWeather::Snow:
		EndApproachScale = 1.8f;
		break;
	default:  // Clear
		EndApproachScale = 1.f;
		break;
	}
	EndApproachDistance = BaselineEndApproachDistance * EndApproachScale;

	// 미끄러운 날씨일수록 커브 최저속도를 올려, 곡선에서 기어가지 않게.
	// 단, 너무 높이면 급커브 안전속도를 초과해 이탈 위험 → 보수적으로.
	float WeatherMinSpeed;
	switch (Weather)
	{
	case EWeather::Rain:
		WeatherMinSpeed = 550.f;   // 시속 ~20
		break;
	case EWeather::Snow:
		WeatherMinSpeed = 750.f;   // 시속 ~27
		break;
	default:  // Clear
		WeatherMinSpeed = BaselineMinSpeed;   // 원본 400 유지
		break;
	}
	MinSpeed = WeatherMinSpeed;

	UE_LOG(LogTeam24, Log,
		TEXT("Autopilot weather: %d, LateralFriction=%.3f (base=%.3f x %.2f), DecelRate=%.2f, MaxSpeed=%.0f, MinSpeed=%.0f, Preview=%.0f, EndApproach=%.0f"),
		static_cast<int32>(EffectiveWeather), LateralFriction,
		BaselineLateralFriction, FrictionScale, DecelRate, MaxSpeed, MinSpeed, BrakePreviewDist, EndApproachDistance);
}
