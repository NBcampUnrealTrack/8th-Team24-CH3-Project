// Copyright NBC, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CameraSensorTypes.h"
#include "CameraSensorComponent.generated.h"

class USceneCaptureComponent2D; // 씬을 @D 텍스처로 렌더링하는 카메라 컴포넌트
class UTextureRenderTarget2D; // 렌더 결과를 저장하는 텍스처 버퍼...??
class UMaterialInterface; // 머티리얼의 기본 인터페이스??
class UMaterialInstanceDynamic; // 런타임에 파라미터 변경 가능한 머티리얼 인스턴스

UCLASS(ClassGroup = (Sensor), meta = (BlueprintSpawnableComponent), BlueprintType)
class TEAM24UNREAL_API UCameraSensorComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UCameraSensorComponent();

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UFUNCTION(BlueprintCallable, Category = "CameraSensor")
	void ApplyPreset(ECameraSensorPreset NewPreset);

	UFUNCTION(BlueprintCallable, Category = "CameraSensor")
	void SetCaptureRate(float Hz);

	UFUNCTION(BlueprintCallable, Category = "CameraSensor")
	void CaptureOnce();

	UFUNCTION(BlueprintCallable, Category = "CameraSensor")
	void RefreshSettings();

	void InitializeCapture();
	void CreateRenderTarget();
	void ConfigureSceneCapture();
	void ApplyPostProcessSettings();
	void ApplyLensDistortion();
	void StartCaptureTimer();
	void StopCaptureTimer();
	void OnCaptureTimer();

	void SaveCameraImage();

	UFUNCTION(BlueprintCallable, Category = "LidarSensor")
	void ApplyTunnelProfile(bool bInTunnel); // 터널에서 쓸 함수

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Config",
		meta = (AllowPrivateAccess = "true"))
	ECameraSensorPreset Preset = ECameraSensorPreset::TeslaHW3_Wide; // 카메라 프리셋 선택

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Config",
		meta=(AllowPrivateAccess="true"))
	bool bSensorEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Intrinsics",
		meta=(AllowPrivateAccess="true"))
	FCameraSensorIntrinsics Intrinsics; // 카메라의 초점거리, 해상도 같은 광학파라미터들을 담는 구조체

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Distortion",
		meta=(AllowPrivateAccess="true"))
	FLensDistortionParams Distortion; // 왜곡계수들 넣어놓은 구조체

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Noise",
		meta=(AllowPrivateAccess="true"))
	FSensorNoiseParams Noise; //  가우시안 노이즈, 샷 노이즈 구조체...??

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|PostProcess",
		meta=(AllowPrivateAccess="true"))
	FCameraPostProcessEffects PostProcess; // 포스트 프로세스 구조체

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Exposure",
		meta=(AllowPrivateAccess="true"))
	FAutoExposureParams Exposure; // 자동 노출 설정 구조체

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|DataSave",
		meta=(AllowPrivateAccess="true"))
	bool bIsDataSaving = false; // 저장 활성화 여부

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|DataSave",
		meta=(AllowPrivateAccess="true"))
	FSensorDataSaveConfig DataSaveConfig; // 저장경로, 파일명 등 설정

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraSensor|Distortion",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMaterialInterface> LensDistortionMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CameraSensor|Output",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTextureRenderTarget2D> RenderTarget; // 카메라 렌더 결과물을 적용할 텍스쳐

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CameraSensor|Output",
		meta=(AllowPrivateAccess="true"))
	int64 FrameCount = 0; // '현재까지 캡처한' 프레임 수

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> SceneCapture; // 실제 렌더링 담당 컴포넌트

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DistortionMID; // 왜곡 머티리얼 인스턴스??

	FTimerHandle CaptureTimerHandle;

	float CachedMinEV = 0.f;
	float CachedMaxEV = 0.f;
	float CachedBloom = 0.f;
	float CachedNoise = 0.f;
};
