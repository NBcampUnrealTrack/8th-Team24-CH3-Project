// Copyright NBC, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CameraSensorTypes.h"
#include "LidarSensorComponent.generated.h"

class ULidarBevRenderer;
class UTextureRenderTarget2D;

UCLASS(ClassGroup = (Sensor), meta = (BlueprintSpawnableComponent), BlueprintType)
class TEAM24UNREAL_API ULidarSensorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULidarSensorComponent();
    UFUNCTION()
	void CyclePreset();

	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void StartScan();

	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void StopScan();

	UFUNCTION(BlueprintPure, Category = "LidarSensor")
	UTexture2D* GetBevRenderTarget() const; // BEV 텍스처 반환, 블루프린트 퓨어 =  노드에 실행 핀 없음??
	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void ApplyTunnelProfile(bool bInTunnel); // 터널에서 쓸 함수

	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void ApplyWeatherProfile(bool bIsWeatherChanged); // 날씨 바뀔 때 쓸 함수
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void ApplyPreset(ELidarSensorPreset NewPreset);

	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void SetScanRate(float Hz);

	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void RefreshSettings();

	void InitializeSensor();
	void StartScanTimer();
	void StopScanTimer();

	void OnScanTimer();
	void FireAsyncTraces(); // 레이 일괄 발사
	void CollectAsyncResults();
	void SavePointCloudData();

	void RebuildDirectionCache();// 방향 벡터 캐시 재계산

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarSensor|Config",
		meta=(AllowPrivateAccess="true"))
	ELidarSensorPreset Preset = ELidarSensorPreset::VelodyneVLP16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarSensor|Config",
		meta=(AllowPrivateAccess="true"))
	bool bSensorEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarSensor|Config",
		meta=(AllowPrivateAccess="true"))
	FLidarSensorConfig Config;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarSensor|BEV",
		meta=(AllowPrivateAccess="true"))
	FBevRenderConfig BevConfig; // 이건 Bev

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarSensor|DataSave",
		meta=(AllowPrivateAccess="true"))
	bool bIsDataSaving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LidarSensor|DataSave",
		meta=(AllowPrivateAccess="true"))
	FSensorDataSaveConfig DataSaveConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LidarSensor|Output",
		meta=(AllowPrivateAccess="true"))
	FLidarPointCloudData LastPointCloud;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LidarSensor|Output",
		meta=(AllowPrivateAccess="true"))
	int64 FrameCount = 0;

	UFUNCTION(BlueprintPure, Category = "LidarSensor")
	const FLidarPointCloudData& GetPointCloud() const { return LastPointCloud; }

	UPROPERTY()
	TObjectPtr<ULidarBevRenderer> BevRenderer;

private:

	FTimerHandle ScanTimerHandle;
	TArray<bool> ScanIsBuilding; // 빌딩 스캔
	TArray<FTraceHandle> PendingHandles; // 발사된 비동기 레이 핸들 목록
	TArray<FVector> PendingWorldDirs; // 발사된 레이 방향 벡터 목록
	FTransform PendingTransform; // 레이 발사 시점의 센서 트랜스폼??

	bool bHasPendingTraces = false; // 비동기 결과 대기 중 여부
	uint64 FireFrameNumber = 0; // 레이 발사한 프레임 번호

	TArray<FVector> CachedLocalDirections; // 미리 계산된 로컬 방향 벡터 캐시

	bool bDirectionsDirty = true; // 캐시 무효화 플래그??

	TArray<FVector> ScanPoints; // 임시 충돌 위치 버퍼?
	TArray<float>   ScanIntensities; // 임시 강도값 버퍼?

	float CachedNoise = 0.f;
	float CachedMaxRange = 0.f;

	float CachedWeatherNoise = 0.f;
	float CachedWeatherMaxRange = 0.f;



};
