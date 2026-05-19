// Copyright Team24. All Rights Reserved.

#include "Component/HazardDetectorComponent.h"
#include "Component/SplineFollowerComponent.h"

//  생성자 / 라이프사이클
UHazardDetectorComponent::UHazardDetectorComponent()
{
	// 매 프레임 Tick 활성화 (위험을 계속 측정해야 하므로)
	PrimaryComponentTick.bCanEverTick = true;
}

void UHazardDetectorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHazardDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 매 프레임 7종 위험 검사
	CheckSlip();
	CheckLatG();
	CheckYaw();
	CheckLaneDeparture();
	CheckRollover();
	CheckJerk();
	CheckFallOff();
}

//  1. 미끄러짐 (Slip)
//     차체 방향과 실제 진행 방향의 각도 차이

void UHazardDetectorComponent::CheckSlip()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	// 1. 실제 진행 방향 = 속도 벡터 (Z는 버림: 경사가 각도 오염 방지)
	FVector Velocity = Car->GetVelocity();
	FVector VelFlat  = FVector(Velocity.X, Velocity.Y, 0.f);
	float   Speed    = VelFlat.Size();

	// 2. 저속이면 계산 안 함 (속도 벡터가 0에 가까워 각도가 튐)
	if (Speed < MinSpeedForSlip)
	{
		// 느려졌는데 아직 위험 상태면 해제 한 번 방송
		if (bSlipActive)
		{
			bSlipActive = false;
			BroadcastSlip(EHazardPhase::Exit, 0.f, Car, Speed);
		}
		return;
	}

	// 3. 차체 방향 (코 방향, Z는 버림)
	FVector Forward = Car->GetActorForwardVector();
	FVector FwdFlat = FVector(Forward.X, Forward.Y, 0.f);

	// 4. 두 방향 사이 각도 (내적 → acos)
	VelFlat.Normalize();
	FwdFlat.Normalize();
	float Dot = FMath::Clamp(FVector::DotProduct(FwdFlat, VelFlat), -1.f, 1.f);
	float SlipAngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// 5. 진입/해제 판정 (히스테리시스)
	if (!bSlipActive)
	{
		if (SlipAngleDeg >= SlipEnterDeg)
		{
			bSlipActive = true;
			BroadcastSlip(EHazardPhase::Enter, SlipAngleDeg, Car, Speed);
		}
	}
	else
	{
		if (SlipAngleDeg < SlipExitDeg)
		{
			bSlipActive = false;
			BroadcastSlip(EHazardPhase::Exit, SlipAngleDeg, Car, Speed);
		}
	}
}

void UHazardDetectorComponent::BroadcastSlip(EHazardPhase Phase, float SlipAngleDeg,
	AActor* Car, float Speed)
{
	FHazardEvent Ev;
	Ev.TimeStamp     = GetWorld()->GetTimeSeconds();
	Ev.ActiveFlags   = (int32)EHazardFlags::Skid;
	Ev.Phase         = Phase;
	Ev.WorldLocation = Car->GetActorLocation();
	Ev.Speed         = Speed;
	Ev.SlipAngleDeg  = SlipAngleDeg;

	// 임시 작동 확인용 (로거 연결되면 삭제 부탁드려요)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
			FString::Printf(TEXT("[위험] 미끄러짐 %s | 슬립각 %.1f도"),
				Phase == EHazardPhase::Enter ? TEXT("시작") : TEXT("해제"),
				SlipAngleDeg));
	}

	OnHazardDetected.Broadcast(Ev);
}

//  2. 횡G (Lateral G)
//     옆 방향 가속도 (타이어 접지 한계 지표)

void UHazardDetectorComponent::CheckLatG()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (DeltaTime <= 0.f) return;

	// 1. 가속도 = (이번 속도 - 직전 속도) / 시간
	FVector Velocity = Car->GetVelocity();
	FVector Accel    = (Velocity - PrevVelocity) / DeltaTime;
	PrevVelocity     = Velocity;

	// 2. 가속도의 옆 방향 성분만 추출 → g 단위 변환 (980 ≈ 1g)
	FVector RightDir = Car->GetActorRightVector();
	float   LatAccel = FVector::DotProduct(Accel, RightDir);
	float   RawLatG  = FMath::Abs(LatAccel) / 980.f;

	// 3. 스무딩: 새 값을 일부만 반영해 노이즈 제거
	SmoothedLatG = SmoothedLatG * (1.f - LatGSmoothingFactor)
	             + RawLatG * LatGSmoothingFactor;
	float LatG = SmoothedLatG;

	float Speed = FVector(Velocity.X, Velocity.Y, 0.f).Size();

	// 4. 진입/해제 판정 (히스테리시스)
	if (!bLatGActive)
	{
		if (LatG >= LatGEnter)
		{
			bLatGActive = true;
			BroadcastLatG(EHazardPhase::Enter, LatG, Car, Speed);
		}
	}
	else
	{
		if (LatG < LatGExit)
		{
			bLatGActive = false;
			BroadcastLatG(EHazardPhase::Exit, LatG, Car, Speed);
		}
	}
}

void UHazardDetectorComponent::BroadcastLatG(EHazardPhase Phase, float LatG,
	AActor* Car, float Speed)
{
	FHazardEvent Ev;
	Ev.TimeStamp     = GetWorld()->GetTimeSeconds();
	Ev.ActiveFlags   = (int32)EHazardFlags::HighLateralG;
	Ev.Phase         = Phase;
	Ev.WorldLocation = Car->GetActorLocation();
	Ev.Speed         = Speed;
	Ev.LateralG      = LatG;

	// 임시 작동 확인용 (로거 연결되면 삭제)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
			FString::Printf(TEXT("[위험] 횡G %s | %.2fg"),
				Phase == EHazardPhase::Enter ? TEXT("시작") : TEXT("해제"),
				LatG));
	}

	OnHazardDetected.Broadcast(Ev);
}

//  3. 휘청거림 (Yaw Instability)
//     차체 회전 속도 (스핀 / 오버스티어 징후)

void UHazardDetectorComponent::CheckYaw()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (DeltaTime <= 0.f) return;

	// 1. 현재 차체 Yaw 각도 (도)
	float CurrentYaw = Car->GetActorRotation().Yaw;

	// 2. 첫 프레임이면 기준만 잡고 종료 (이전값 없음)
	if (!bYawInitialized)
	{
		PrevYaw = CurrentYaw;
		bYawInitialized = true;
		return;
	}

	// 3. 이전 프레임과의 각도 차이 (359→0 경계 안전 처리)
	float DeltaYaw = FMath::FindDeltaAngleDegrees(PrevYaw, CurrentYaw);
	PrevYaw = CurrentYaw;

	// 4. 회전 속도 = 각도 변화 / 시간 (도/초, 부호 무시)
	float YawRate = FMath::Abs(DeltaYaw / DeltaTime);

	float Speed = FVector(Car->GetVelocity().X, Car->GetVelocity().Y, 0.f).Size();

	// 5. 진입/해제 판정 (히스테리시스)
	if (!bYawActive)
	{
		if (YawRate >= YawRateEnter)
		{
			bYawActive = true;
			BroadcastYaw(EHazardPhase::Enter, YawRate, Car, Speed);
		}
	}
	else
	{
		if (YawRate < YawRateExit)
		{
			bYawActive = false;
			BroadcastYaw(EHazardPhase::Exit, YawRate, Car, Speed);
		}
	}
}

void UHazardDetectorComponent::BroadcastYaw(EHazardPhase Phase, float YawRate,
	AActor* Car, float Speed)
{
	FHazardEvent Ev;
	Ev.TimeStamp     = GetWorld()->GetTimeSeconds();
	Ev.ActiveFlags   = (int32)EHazardFlags::YawInstability;
	Ev.Phase         = Phase;
	Ev.WorldLocation = Car->GetActorLocation();
	Ev.Speed         = Speed;
	Ev.YawRate       = YawRate;

	// 임시 작동 확인용 (로거 연결되면 삭제)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("[위험] 휘청거림 %s | %.0f도/초"),
				Phase == EHazardPhase::Enter ? TEXT("시작") : TEXT("해제"),
				YawRate));
	}

	OnHazardDetected.Broadcast(Ev);
}

//  4. 경로이탈 (Lane Departure)
//     도로 중심선에서 벗어난 거리 (SplineFollower의 CTE 사용)

void UHazardDetectorComponent::CheckLaneDeparture()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	// 1. SplineFollower에서 횡오차(CTE) 가져오기 (없으면 0)
	float CTE = 0.f;
	if (USplineFollowerComponent* Spline =
		Car->FindComponentByClass<USplineFollowerComponent>())
	{
		CTE = Spline->GetCrossTrackError();
	}

	float AbsCTE = FMath::Abs(CTE);
	float Speed  = FVector(Car->GetVelocity().X, Car->GetVelocity().Y, 0.f).Size();

	// 2. 진입/해제 판정 (히스테리시스)
	if (!bLaneActive)
	{
		if (AbsCTE >= LaneDepartEnter)
		{
			bLaneActive = true;
			BroadcastSimple(EHazardFlags::LaneDeparture, EHazardPhase::Enter,
				Car, Speed, FColor::Yellow, TEXT("경로이탈"), AbsCTE);
		}
	}
	else
	{
		if (AbsCTE < LaneDepartExit)
		{
			bLaneActive = false;
			BroadcastSimple(EHazardFlags::LaneDeparture, EHazardPhase::Exit,
				Car, Speed, FColor::Yellow, TEXT("경로이탈"), AbsCTE);
		}
	}
}

//  5. 전복 (Rollover)
//     차체 좌우 기울기 (Roll 각도)

void UHazardDetectorComponent::CheckRollover()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	// 1. 차체 Roll 각도 (절댓값)
	float RollDeg = FMath::Abs(Car->GetActorRotation().Roll);

	float Speed = FVector(Car->GetVelocity().X, Car->GetVelocity().Y, 0.f).Size();

	// 2. 진입/해제 판정 (히스테리시스)
	if (!bRollActive)
	{
		if (RollDeg >= RollEnter)
		{
			bRollActive = true;
			BroadcastSimple(EHazardFlags::RolloverRisk, EHazardPhase::Enter,
				Car, Speed, FColor::Magenta, TEXT("전복위험"), RollDeg);
		}
	}
	else
	{
		if (RollDeg < RollExit)
		{
			bRollActive = false;
			BroadcastSimple(EHazardFlags::RolloverRisk, EHazardPhase::Exit,
				Car, Speed, FColor::Magenta, TEXT("전복위험"), RollDeg);
		}
	}
}

//  6. 급조작 (Harsh Maneuver)
//     횡가속도의 변화율(Jerk) — 급격한 조작 감지

void UHazardDetectorComponent::CheckJerk()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (DeltaTime <= 0.f) return;

	// 1. 현재 횡가속도 (g 단위) — CheckLatG와 동일 방식
	FVector Velocity   = Car->GetVelocity();
	FVector Accel      = (Velocity - PrevVelocity) / DeltaTime;
	FVector RightDir   = Car->GetActorRightVector();
	float   LatAccelG  = FVector::DotProduct(Accel, RightDir) / 980.f;

	// 2. 첫 프레임이면 기준만 잡고 종료 (이전값 없음)
	if (!bJerkInitialized)
	{
		PrevLatAccelForJerk = LatAccelG;
		bJerkInitialized = true;
		return;
	}

	// 3. Jerk = 횡가속도의 변화율 (얼마나 급격히 바뀌나)
	float Jerk = FMath::Abs((LatAccelG - PrevLatAccelForJerk) / DeltaTime);
	PrevLatAccelForJerk = LatAccelG;

	float Speed = FVector(Velocity.X, Velocity.Y, 0.f).Size();

	// 4. 진입/해제 판정 (히스테리시스)
	if (!bJerkActive)
	{
		if (Jerk >= JerkEnter)
		{
			bJerkActive = true;
			BroadcastSimple(EHazardFlags::HarshManeuver, EHazardPhase::Enter,
				Car, Speed, FColor::White, TEXT("급조작"), Jerk);
		}
	}
	else
	{
		if (Jerk < JerkExit)
		{
			bJerkActive = false;
			BroadcastSimple(EHazardFlags::HarshManeuver, EHazardPhase::Exit,
				Car, Speed, FColor::White, TEXT("급조작"), Jerk);
		}
	}
}

//  7. 추락 (Fall Off)
//     도로 밖으로 떨어지는 속도 (속도 벡터의 Z 성분)

void UHazardDetectorComponent::CheckFallOff()
{
	AActor* Car = GetOwner();
	if (!Car) return;

	// 1. 속도의 Z 성분 → 아래로 떨어지는 속도만 양수로 (위로 가는 건 추락 아님)
	float VerticalSpeed = Car->GetVelocity().Z;
	float FallSpeed = (VerticalSpeed < 0.f) ? -VerticalSpeed : 0.f;

	float Speed = FVector(Car->GetVelocity().X, Car->GetVelocity().Y, 0.f).Size();

	// 2. 진입/해제 판정 (히스테리시스)
	if (!bFallActive)
	{
		if (FallSpeed >= FallSpeedEnter)
		{
			bFallActive = true;
			BroadcastSimple(EHazardFlags::FallOff, EHazardPhase::Enter,
				Car, Speed, FColor::Red, TEXT("추락"), FallSpeed);
		}
	}
	else
	{
		if (FallSpeed < FallSpeedExit)
		{
			bFallActive = false;
			BroadcastSimple(EHazardFlags::FallOff, EHazardPhase::Exit,
				Car, Speed, FColor::Red, TEXT("추락"), FallSpeed);
		}
	}
}

//  공용 방송 함수
//  전용 칸이 필요 없는 단순 지표(경로이탈/전복/급조작/추락)가 공유

void UHazardDetectorComponent::BroadcastSimple(EHazardFlags Flag, EHazardPhase Phase,
	AActor* Car, float Speed, const FColor& Color, const FString& Label, float Value)
{
	FHazardEvent Ev;
	Ev.TimeStamp     = GetWorld()->GetTimeSeconds();
	Ev.ActiveFlags   = (int32)Flag;
	Ev.Phase         = Phase;
	Ev.WorldLocation = Car->GetActorLocation();
	Ev.Speed         = Speed;

	// 지표 종류에 맞는 전용 칸 채우기
	if (Flag == EHazardFlags::LaneDeparture)      Ev.CrossTrackError = Value;
	else if (Flag == EHazardFlags::RolloverRisk)  Ev.RollDeg = Value;

	// 임시 작동 확인용 (로거 연결되면 삭제)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, Color,
			FString::Printf(TEXT("[위험] %s %s | %.1f"),
				*Label,
				Phase == EHazardPhase::Enter ? TEXT("시작") : TEXT("해제"),
				Value));
	}

	OnHazardDetected.Broadcast(Ev);
}
