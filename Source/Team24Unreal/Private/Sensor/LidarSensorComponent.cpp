// Copyright NBC, Inc. All Rights Reserved.

#include "Sensor/LidarSensorComponent.h"
#include "Sensor/LidarBevRenderer.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLidarSensor, Log, All);

ULidarSensorComponent::ULidarSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULidarSensorComponent::CyclePreset()
{
	const TArray<ELidarSensorPreset> List =
		{
		ELidarSensorPreset::Custom,
		ELidarSensorPreset::VelodyneVLP16,
		ELidarSensorPreset::VelodyneVLP32,
		ELidarSensorPreset::OusterOS1_64,
		ELidarSensorPreset::Livox_Mid360,
	};
	int32 Idx = List.IndexOfByKey(Preset);
	Preset = List[(Idx + 1) % List.Num()];
	ApplyPreset(Preset);
	RefreshSettings();

}

void ULidarSensorComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyPreset(Preset);
	InitializeSensor();
}

void ULidarSensorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopScanTimer();
	Super::EndPlay(EndPlayReason);
}

void ULidarSensorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHasPendingTraces && GFrameCounter > FireFrameNumber) // bHasPendingTraces = 비동기트레이스가 아직 처리중인지 여부, FireFrameNumber = 레이를 쏜 프레임 번호
	{
		CollectAsyncResults();
		SetComponentTickEnabled(false);
	}
}

#if WITH_EDITOR
void ULidarSensorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(ULidarSensorComponent, Preset))
		ApplyPreset(Preset);
	bDirectionsDirty = true; // 방향 캐시 무효화
}
#endif

void ULidarSensorComponent::InitializeSensor()
{
	BevConfig.ViewRange = Config.MaxRange; // BEV 이미지의 표시 범위를 라이다 최대 거리로 맞춤

	BevRenderer = NewObject<ULidarBevRenderer>(this, TEXT("BevRenderer"));
	BevRenderer->Initialize(BevConfig);

	const int32 TotalPts = Config.GetTotalPoints();
	PendingHandles.Reserve(TotalPts); // 비동기 트레이스 핸들
	PendingWorldDirs.Reserve(TotalPts); // 레이 방향 벡터
	ScanPoints.Reserve(TotalPts); // 충돌 위치
	ScanIntensities.Reserve(TotalPts); // 강도값
	ScanIsBuilding.Reserve(TotalPts); // 메모리 할당 용
	LastPointCloud.bIsBuilding.Reserve(TotalPts);
	LastPointCloud.Points.Reserve(TotalPts); // 최종 포인트클라우드
	LastPointCloud.Intensities.Reserve(TotalPts); // 최종 강도

	bDirectionsDirty = true;
	bSensorEnabled = false;

	UE_LOG(LogLidarSensor, Log,
		TEXT("LidarSensor initialized: %d ch x %d pts @ %.0f Hz, range %.0f m  [AsyncTrace]"),
		Config.NumChannels, Config.PointsPerChannel,
		Config.RotationRate, Config.MaxRange / 100.0f
	);
}

void ULidarSensorComponent::RebuildDirectionCache() // 라이다가 쏠 모든 레이의 방향 벡터를 미리 계산해서 캐싱하는 함수
{
	const int32 NumCh    = Config.NumChannels;
	const int32 NumPts   = Config.PointsPerChannel;
	const float VertLow  = Config.VerticalFOVLower;
	const float VertRng  = Config.VerticalFOVUpper - VertLow;
	const float HorizFOV = Config.HorizontalFOV;

	CachedLocalDirections.SetNum(NumCh * NumPts, EAllowShrinking::No);

	for (int32 Ch = 0; Ch < NumCh; ++Ch)
	{
		const float VertDeg  = (NumCh > 1) ? VertLow + VertRng * (float(Ch) / (NumCh - 1)) : 0.f;
		const float CosVert  = FMath::Cos(FMath::DegreesToRadians(VertDeg));
		const float SinVert  = FMath::Sin(FMath::DegreesToRadians(VertDeg));

		for (int32 Pt = 0; Pt < NumPts; ++Pt)
		{
			const float HorizRad = FMath::DegreesToRadians((float(Pt) / NumPts) * HorizFOV);
			CachedLocalDirections[Ch * NumPts + Pt] = FVector(
				CosVert * FMath::Cos(HorizRad),
				CosVert * FMath::Sin(HorizRad),
				SinVert
			);
		}
	}

	bDirectionsDirty = false;
}

void ULidarSensorComponent::StartScanTimer() // 일정 주기마다 OnScanTimer()를 자동 호출하는 타이머를 등록하는 함수
{
	if (GetWorld() == nullptr) return;

	const float Interval = 1.0f / FMath::Max(Config.RotationRate, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(
		ScanTimerHandle,
		this,
		&ULidarSensorComponent::OnScanTimer,
		Interval,
		true);
}

void ULidarSensorComponent::StopScanTimer()
{
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);

	bHasPendingTraces = false;
	SetComponentTickEnabled(false);
}

void ULidarSensorComponent::OnScanTimer() // 타이머가 호출할 때마다 한 프레임의 라이다 스캔을 실행하는 함수
{
	if (!bSensorEnabled)
		return;

	if (bDirectionsDirty)
		RebuildDirectionCache();

	FireAsyncTraces();

	++FrameCount;
}

void ULidarSensorComponent::FireAsyncTraces() // 모든 레이를 비동기로 한꺼번에 발사하는 함수
{
	UWorld* World = GetWorld();
	if (!World || CachedLocalDirections.IsEmpty())
		return;

	PendingHandles.Reset();
	PendingWorldDirs.Reset();

	const FTransform SensorTransform = GetComponentTransform();
	const FVector    SensorLoc       = SensorTransform.GetLocation();
	const FQuat      SensorQuat      = SensorTransform.GetRotation();
	PendingTransform = SensorTransform;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LidarAsyncTrace), false);
	Params.AddIgnoredActor(GetOwner());
	Params.bReturnPhysicalMaterial = false;

	const float MaxRange = Config.MaxRange;

	for (const FVector& LocalDir : CachedLocalDirections)
	{
		const FVector WorldDir = SensorQuat.RotateVector(LocalDir);
		const FVector End      = SensorLoc + WorldDir * MaxRange;

		FTraceHandle Handle = World->AsyncLineTraceByChannel(
			EAsyncTraceType::Single,
			SensorLoc, End,
			ECC_Visibility,
			Params
		);

		PendingHandles.Add(Handle);
		PendingWorldDirs.Add(WorldDir);
	}

	FireFrameNumber   = GFrameCounter;
	bHasPendingTraces = true;

	SetComponentTickEnabled(true);
}

void ULidarSensorComponent::CollectAsyncResults() // 비동기로 발사했던 레이들의 결과를 수집해서 포인트클라우드로 만드는 함수
{
	UWorld* World = GetWorld();
	if (!World) return;

	ScanPoints.Reset();
	ScanIntensities.Reset();
	ScanIsBuilding.Reset(); //프레임 수집전 프레임 데이터 비우기

	const float MaxRange = Config.MaxRange;
	const float MinRange = Config.MinRange;
	const float NoiseStd = Config.NoiseStdDev;

	for (int32 i = 0; i < PendingHandles.Num(); ++i)
	{
		FTraceDatum Data;
		if (!World->QueryTraceData(PendingHandles[i], Data)) continue; // 비동기 트레이서의 결과가 준비됐는지 확인하고 가져오는 함수
		if (Data.OutHits.IsEmpty()) continue;

		const FHitResult& Hit = Data.OutHits[0];
		if (!Hit.bBlockingHit || Hit.Distance < MinRange) continue;

		FVector HitPoint = Hit.ImpactPoint;
		if (NoiseStd > 0.f && PendingWorldDirs.IsValidIndex(i))
		{
			HitPoint += PendingWorldDirs[i] * FMath::RandRange(-NoiseStd, NoiseStd);
		}

		ScanPoints.Add(HitPoint);
		ScanIntensities.Add(FMath::Clamp(1.f - (Hit.Distance / MaxRange), 0.f, 1.f));
		AActor* HitActor = Hit.GetActor();
		ScanIsBuilding.Add(HitActor && HitActor->ActorHasTag(TEXT("Building"))); // 빌딩 태그 체크
	}

	LastPointCloud.Points      = MoveTemp(ScanPoints); // 포인트 위치 배열 이동
	LastPointCloud.Intensities = MoveTemp(ScanIntensities); // 강도값 배열 이동
	LastPointCloud.bIsBuilding = MoveTemp(ScanIsBuilding); // LastPointCloud로 데이터 이동
	LastPointCloud.PointCount  = LastPointCloud.Points.Num(); // 포인트 개수 기록
	LastPointCloud.FrameNumber = FrameCount; // 프레임 번호 기록

	ScanPoints.Reserve(Config.GetTotalPoints());
	ScanIntensities.Reserve(Config.GetTotalPoints());
	ScanIsBuilding.Reserve(Config.GetTotalPoints()); // 메모리 확보

	if (BevRenderer)
		BevRenderer->RenderPointCloud(LastPointCloud, PendingTransform);

	if (bIsDataSaving && LastPointCloud.PointCount > 0)
		SavePointCloudData();

	bHasPendingTraces = false;

	UE_LOG(LogLidarSensor, Verbose,
		TEXT("LidarSensor frame %lld: %d points"), FrameCount, LastPointCloud.PointCount);
}

void ULidarSensorComponent::StartScan()
{
	bSensorEnabled = true;
	StartScanTimer();
}

void ULidarSensorComponent::StopScan()
{
	bSensorEnabled = false;
	StopScanTimer();
}

void ULidarSensorComponent::SetScanRate(float Hz) // 스캔 주파수를 런타임 중에 변경하는 함수, 0~30Hz까지로 제한함
{
	Config.RotationRate = FMath::Clamp(Hz, 1.0f, 30.0f);
	StopScanTimer();
	if (bSensorEnabled)
		StartScanTimer();
}

void ULidarSensorComponent::RefreshSettings() // 라이다 설정 전체를 런타임 중에 갱신하는 함수
{
	bDirectionsDirty = true;
	BevConfig.ViewRange = Config.MaxRange;

	if (BevRenderer)
		BevRenderer->UpdateConfig(BevConfig);

	StopScanTimer();

	if (bSensorEnabled)
		StartScanTimer();
}

UTexture2D* ULidarSensorComponent::GetBevRenderTarget() const // BEV 렌더러에서 텍스처를 가져오는 함수
{
	return BevRenderer ? BevRenderer->GetRenderTarget() : nullptr;
}

void ULidarSensorComponent::ApplyPreset(ELidarSensorPreset NewPreset) // 라이다 센서 종류 선택
{
	Preset = NewPreset;
	bDirectionsDirty = true;

	switch (NewPreset)
	{
	case ELidarSensorPreset::VelodyneVLP16:
		Config = { 16, 1800, 10.0f, 10000.0f, 50.0f, 15.0f, -15.0f, 360.0f, 2.0f };
		break;

	case ELidarSensorPreset::VelodyneVLP32:
		Config = { 32, 60, 10.0f, 20000.0f, 50.0f, 15.0f, -25.0f, 360.0f, 2.0f };
		break;

	case ELidarSensorPreset::OusterOS1_64:
		Config = { 64, 45, 10.0f, 12000.0f, 50.0f, 22.5f, -22.5f, 360.0f, 1.5f };
		break;

	case ELidarSensorPreset::Livox_Mid360:
		Config = { 8, 45, 10.0f, 7000.0f, 100.0f, 52.0f, -7.0f, 360.0f, 3.0f };
		break;

	case ELidarSensorPreset::Custom:
	default:
		break;
	}
}

void ULidarSensorComponent::SavePointCloudData() // 포인트클라우드를 KITTI 포맷 .bin 파일로 저장하는 함수
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("SensorData") / DataSaveConfig.SensorLabel;
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString FilePath = Dir / FString::Printf(TEXT("%06lld.bin"), FrameCount);

	TUniquePtr<IFileHandle> File(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath));
	if (!File)
	{
		UE_LOG(LogLidarSensor, Warning, TEXT("Failed to open file for writing: %s"), *FilePath);
		return;
	}

	const FTransform InvSensor = PendingTransform.Inverse();
	const int32 NumPoints = LastPointCloud.PointCount;

	struct FKittiPoint { float X, Y, Z, Intensity; };
	TArray<FKittiPoint> Buffer;
	Buffer.SetNumUninitialized(NumPoints);

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector Local = InvSensor.TransformPosition(LastPointCloud.Points[i]);
		Buffer[i].X         =  static_cast<float>(Local.X * 0.01);
		Buffer[i].Y         = -static_cast<float>(Local.Y * 0.01);
		Buffer[i].Z         =  static_cast<float>(Local.Z * 0.01);
		Buffer[i].Intensity = LastPointCloud.Intensities[i];
	}

	File->Write(
		reinterpret_cast<const uint8*>(Buffer.GetData()),
		NumPoints * sizeof(FKittiPoint)
	);

	UE_LOG(LogLidarSensor, Verbose, TEXT("Saved %d points → %s"), NumPoints, *FilePath);
}

void ULidarSensorComponent::ApplyTunnelProfile(bool bInTunnel) // 터널에서
{
	if (bInTunnel)
	{
		CachedNoise = Config.NoiseStdDev;
		CachedMaxRange = Config.MaxRange;

		Config.NoiseStdDev *= 1.5f;
		Config.MaxRange *= 0.7f;

		BevConfig.ViewRange = Config.MaxRange;
		if (BevRenderer)
			BevRenderer->UpdateConfig(BevConfig);

		RebuildDirectionCache();
	}
	else
	{
		Config.NoiseStdDev = CachedNoise;
		Config.MaxRange = CachedMaxRange;

		BevConfig.ViewRange = Config.MaxRange;
		if (BevRenderer)
			BevRenderer->UpdateConfig(BevConfig);

		RebuildDirectionCache();
	}
}

void ULidarSensorComponent::ApplyWeatherProfile(bool bIsWeatherChanged) // 날씨 바뀔 때
{
	if (bIsWeatherChanged)
	{
		CachedWeatherNoise = Config.NoiseStdDev;
		CachedWeatherMaxRange = Config.MaxRange;

		Config.NoiseStdDev *= 1.5f;
		Config.MaxRange *= 0.7f;

		BevConfig.ViewRange = Config.MaxRange;
		if (BevRenderer)
			BevRenderer->UpdateConfig(BevConfig);

		RebuildDirectionCache();
	}
	else
	{
		Config.NoiseStdDev = CachedWeatherNoise;
		Config.MaxRange = CachedWeatherMaxRange;

		BevConfig.ViewRange = Config.MaxRange;
		if (BevRenderer)
			BevRenderer->UpdateConfig(BevConfig);

		RebuildDirectionCache();
	}
}
