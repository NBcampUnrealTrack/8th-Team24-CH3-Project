// Copyright NBC, Inc. All Rights Reserved.

#include "Sensor/CameraSensorComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/PlatformFileManager.h"  //Hardware Abstraction Layer 플랫폼 상관 없이 동일한 시스템 접근 제공
#include "IImageWrapper.h" // SaveCameraImage()에서 JPEG 압축에 사용
#include "IImageWrapperModule.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h" // JPEG를 파일로 저장
#include "Misc/Paths.h" // JPEG파일 저장 경로 생성
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCameraSensor, Log, All);

UCameraSensorComponent::UCameraSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SensorSceneCapture"));
	SceneCapture->SetupAttachment(this);
}

void UCameraSensorComponent::OnRegister() // 위의 생성자에서 설정한 내용이지만 방어적 프로그래밍을 위해 한번 더 설정한 것
{
	Super::OnRegister();

	if (SceneCapture && SceneCapture->IsRegistered() == false)
	{
		SceneCapture->SetupAttachment(this);
		SceneCapture->RegisterComponent();
	}
}

void UCameraSensorComponent::BeginPlay() // 게임 시작 시 카메라 센서를 초기화하고 프리셋을 적용
{
	Super::BeginPlay();
	InitializeCapture(); // 캡처 시스템 초기화(텍스처 메모리 설정, 씬캡처 적용, 후처리 효과 적용, 렌즈 왜곡 적용, 타이머 시작)
	ApplyPreset(Preset); // 프리셋에 맞는 카메라 스펙 적용(TeslaHW3_Wide). 해상도, 시야각, 렌즈 계수 등...
}                        // 위에서 캡처시스템을 0으로 초기화한걸 여기서 특정한 값으로 덮어씌우는거임

void UCameraSensorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCaptureTimer();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR // 에디터 안에서만 사용할 것이기 때문에 이렇게 주석처리함
void UCameraSensorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
// 위에서 속성을 변경했으면 즉시 ApplyPreset()을 호출하는 함수
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCameraSensorComponent, Preset))
	{
		ApplyPreset(Preset);
	}
}
#endif

void UCameraSensorComponent::InitializeCapture() // 카메라 시스템을 전부 다 0같은 값으로 초기화하는 함수
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("CameraSensorComponent has no owning actor."));
		return;
	}

	CreateRenderTarget(); // 텍스처 메모리 생성
	ConfigureSceneCapture(); // SceneCapture에 RenderTarget 연결
	ApplyPostProcessSettings(); // 후처리 효과 적용
	ApplyLensDistortion(); // 렌즈 왜곡 적용

	if (bSensorEnabled) // 조건부로 타이머 시작
	{
		StartCaptureTimer();
	}

	UE_LOG(LogCameraSensor, Log, TEXT("CameraSensor initialized: %dx%d @ %.0f Hz, FOV %.0f°"), // 초기화 완료 로그
		Intrinsics.ImageWidth, Intrinsics.ImageHeight, Intrinsics.FrameRate, Intrinsics.FOVDegrees);
}

void UCameraSensorComponent::CreateRenderTarget()
{
	RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("SensorRenderTarget"));
	RenderTarget->InitAutoFormat(Intrinsics.ImageWidth, Intrinsics.ImageHeight); // 카메라 해상도 설정
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8; // 32bit 일반 카메라 포맷으로 설정
	RenderTarget->bAutoGenerateMips = false; // Mips 자동완성 꺼놓음
	RenderTarget->UpdateResourceImmediate(true); // GPU에 즉시 반영함
}

void UCameraSensorComponent::ConfigureSceneCapture()
{
	if (SceneCapture == nullptr || RenderTarget == nullptr)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("SceneCaptureComponent is not ready."));
		return;
	}

	SceneCapture->TextureTarget = RenderTarget; // 씬 캡처가 렌더타겟에 렌더링
	SceneCapture->FOVAngle = Intrinsics.FOVDegrees; // 실제 카메라 스펙의 시야각 적용

	SceneCapture->bCaptureEveryFrame = false;// 매 프레임 자동 캡처 off
	SceneCapture->bCaptureOnMovement = false;// 움직임 감지 캡처 off
	SceneCapture->bAlwaysPersistRenderingState = true; // 랜더링 상태 유지

	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR; // 최종 랜더링 결과 캡처

	SceneCapture->PostProcessBlendWeight = 1.0f; // 후처리 100% 적용
}

void UCameraSensorComponent::ApplyPostProcessSettings() // 씬 캡처의 여러 설정들을 한번에 바꾸는 함수
{
	if (SceneCapture == nullptr)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("There is no SceneCaptureComponent."));
		return;
	}

	FPostProcessSettings& PP = SceneCapture->PostProcessSettings;

	PP.bOverride_VignetteIntensity = true; // 비네팅, 화면 가장자리가 어두워지는 효과 (0.f: 효과 X, 100.f: 가장자리 어두움)
	PP.VignetteIntensity = PostProcess.VignetteIntensity;

	PP.bOverride_BloomIntensity = true; // 블룸, 밝은 부분이 번져보이는 효과 (0.f: 효과 X, 100.f: 강한 번짐)
	PP.BloomIntensity = PostProcess.BloomIntensity;

	PP.bOverride_MotionBlurAmount = true; // 모션 블러, 빠르게 움직이는 물체가 흐리게 보이는 효과
	PP.MotionBlurAmount = PostProcess.MotionBlurAmount;

	PP.bOverride_SceneFringeIntensity = true; // 색수차, RGB 채널이 번져보이는 효과. 하나의 색이 RGB로 뚜렷하게 분리되어보임.
	PP.SceneFringeIntensity = PostProcess.ChromaticAberration;

	PP.bOverride_LensFlareIntensity = true; // 플레어, 강한 빛 앞에서 생기는 빛 번짐 효과
	PP.LensFlareIntensity = PostProcess.LensFlareIntensity;

	if (Exposure.bEnableAutoExposure) // 자동 노출
	{
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;

		PP.bOverride_AutoExposureMinBrightness = true;
		PP.AutoExposureMinBrightness = Exposure.MinEV;

		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMaxBrightness = Exposure.MaxEV;

		PP.bOverride_AutoExposureSpeedUp = true;
		PP.AutoExposureSpeedUp = Exposure.SpeedUp;

		PP.bOverride_AutoExposureSpeedDown = true;
		PP.AutoExposureSpeedDown = Exposure.SpeedDown;
	}
	else // 수동 노출
	{
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	}
}

void UCameraSensorComponent::ApplyLensDistortion() // 실제 카메라의 왜곡 현상을 머티리얼로 재현해서 씬 캡처에 적용
{
	if (SceneCapture == nullptr)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("There is no SceneCaptureComponent"));
		return;
	}
	if (Distortion.HasDistortion() == false)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("There is no Distortion"));
		return;
	}
	if (LensDistortionMaterial == nullptr)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("There is no LensDistortionMaterial"));
		return;
	}

	DistortionMID = UMaterialInstanceDynamic::Create(LensDistortionMaterial, this); // Distortion MID 생성
	if (DistortionMID == nullptr)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("Failed to create DistortionMID."));
		return;
	}

	DistortionMID->SetScalarParameterValue(TEXT("K1"), Distortion.K1); // 왜곡 계수 적용, k: 방사 왜곡(렌즈 중심에서 멀어질수록 휘는 정도)
	// k가 +일수록 볼록 왜곡, -일수록 오목 왜곡
	DistortionMID->SetScalarParameterValue(TEXT("K2"), Distortion.K2);
	DistortionMID->SetScalarParameterValue(TEXT("K3"), Distortion.K3);
	DistortionMID->SetScalarParameterValue(TEXT("P1"), Distortion.P1);// P: 접선 왜곡(렌즈가 완벽히 수평이 아닐 때 생기는 왜곡)
	DistortionMID->SetScalarParameterValue(TEXT("P2"), Distortion.P2);

	FWeightedBlendable Blendable; // PostProcess에 등록
	Blendable.Object = DistortionMID.Get(); // 적용할 머티리얼
	Blendable.Weight = 1.0f; // 전부 적용
	SceneCapture->PostProcessSettings.WeightedBlendables.Array.Add(Blendable);
}

void UCameraSensorComponent::StartCaptureTimer() // 카메라 센서의 Hz에 맞춰 주기적으로 캡처하는 타이머 시작
{
	if (!GetWorld()) return;

	const float Interval = 1.0f / FMath::Max(Intrinsics.FrameRate, 1.0f); // 캡처 간격 계산, 1이 아니라 0이면 크래쉬남
	GetWorld()->GetTimerManager().SetTimer(
		CaptureTimerHandle, this, &UCameraSensorComponent::OnCaptureTimer,
		Interval, true);
}

void UCameraSensorComponent::StopCaptureTimer()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CaptureTimerHandle);
	}
}

void UCameraSensorComponent::OnCaptureTimer() // 타이머마다 호출되어 캡처 시작, 필요 시 이미지 저장
{
	if (bSensorEnabled && SceneCapture)
	{
		SceneCapture->CaptureScene();
		FrameCount++;

		if (bIsDataSaving && RenderTarget)
		{
			SaveCameraImage();
		}
	}
}

void UCameraSensorComponent::SetCaptureRate(float Hz) // 캡처 Hz를 변경하고 타이머를 재시작해서 새 주기로 캡처
{
	Intrinsics.FrameRate = FMath::Clamp(Hz, 1.0f, 120.0f); // Hz 범위 제한
	StopCaptureTimer();
	if (bSensorEnabled)
	{
		StartCaptureTimer();
	}
}

void UCameraSensorComponent::CaptureOnce() // 타이머와 무관하게 캡처 한번 하는 함수
{
	if (SceneCapture)
	{
		SceneCapture->CaptureScene();
		FrameCount++;
	}
}

void UCameraSensorComponent::RefreshSettings() // 런타임 중 설정이 변경됐을 때 모든 설정을 안전하게 갱신하는 함수
{
	if (SceneCapture == nullptr)
	{
		UE_LOG(LogCameraSensor, Error, TEXT("There is no SceneCaptureComponent."));
		return;
	}

	SceneCapture->FOVAngle = Intrinsics.FOVDegrees;

	if (RenderTarget &&
		(RenderTarget->SizeX != Intrinsics.ImageWidth || RenderTarget->SizeY != Intrinsics.ImageHeight))
	{
		RenderTarget->InitAutoFormat(Intrinsics.ImageWidth, Intrinsics.ImageHeight);
		RenderTarget->UpdateResourceImmediate(true);
	}

	SceneCapture->PostProcessSettings.WeightedBlendables.Array.Empty();
	ApplyPostProcessSettings();
	ApplyLensDistortion();

	StopCaptureTimer();
	if (bSensorEnabled)
	{
		StartCaptureTimer();
	}
}

void UCameraSensorComponent::ApplyPreset(ECameraSensorPreset NewPreset) // 실제 카메라 스펙을 미리 정의해두고 한번에 적용하는 함수
{
	Preset = NewPreset;

	switch (NewPreset)
	{
	case ECameraSensorPreset::TeslaHW3_Wide:
		Intrinsics = { 1280, 960, 36.0f, 120.0f, 1.5f };
		Distortion = { -0.3f, 0.1f, 0.0f, 0.0f, 0.0f };
		PostProcess.VignetteIntensity = 0.4f;
		PostProcess.ChromaticAberration = 0.5f;
		PostProcess.MotionBlurAmount = 0.5f;
		PostProcess.BloomIntensity = 0.0f;
		PostProcess.LensFlareIntensity = 0.1f;
		Exposure = { true, 2.0f, 14.0f, 3.0f, 1.0f };
		Noise = { true, 0.0f, 0.02f, 0.01f };
		break;

	case ECameraSensorPreset::TeslaHW3_Main:
		Intrinsics = { 1280, 960, 36.0f, 50.0f, 5.0f };
		Distortion = { -0.1f, 0.02f, 0.0f, 0.0f, 0.0f };
		PostProcess.VignetteIntensity = 0.3f;
		PostProcess.ChromaticAberration = 0.3f;
		PostProcess.MotionBlurAmount = 0.5f;
		PostProcess.BloomIntensity = 0.0f;
		PostProcess.LensFlareIntensity = 0.05f;
		Exposure = { true, 2.0f, 14.0f, 3.0f, 1.0f };
		Noise = { true, 0.0f, 0.02f, 0.01f };
		break;

	case ECameraSensorPreset::TeslaHW3_Narrow:
		Intrinsics = { 1280, 960, 36.0f, 35.0f, 8.0f };
		Distortion = { -0.05f, 0.01f, 0.0f, 0.0f, 0.0f };
		PostProcess.VignetteIntensity = 0.2f;
		PostProcess.ChromaticAberration = 0.2f;
		PostProcess.MotionBlurAmount = 0.5f;
		PostProcess.BloomIntensity = 0.0f;
		PostProcess.LensFlareIntensity = 0.05f;
		Exposure = { true, 2.0f, 14.0f, 3.0f, 1.0f };
		Noise = { true, 0.0f, 0.02f, 0.01f };
		break;

	case ECameraSensorPreset::TeslaHW4:
		Intrinsics = { 2560, 1920, 24.0f, 130.0f, 1.3f };
		Distortion = { -0.35f, 0.12f, 0.0f, 0.0f, 0.0f };
		PostProcess.VignetteIntensity = 0.35f;
		PostProcess.ChromaticAberration = 0.4f;
		PostProcess.MotionBlurAmount = 0.4f;
		PostProcess.BloomIntensity = 0.0f;
		PostProcess.LensFlareIntensity = 0.08f;
		Exposure = { true, 1.0f, 15.0f, 3.5f, 1.2f };
		Noise = { true, 0.0f, 0.015f, 0.008f };
		break;

	case ECameraSensorPreset::DroneFPV:
		Intrinsics = { 1920, 1080, 60.0f, 150.0f, 2.5f };
		Distortion = { -0.5f, 0.2f, -0.05f, 0.0f, 0.0f };
		PostProcess.VignetteIntensity = 0.5f;
		PostProcess.ChromaticAberration = 0.7f;
		PostProcess.MotionBlurAmount = 0.3f;
		PostProcess.BloomIntensity = 0.1f;
		PostProcess.LensFlareIntensity = 0.15f;
		Exposure = { true, 3.0f, 16.0f, 4.0f, 2.0f };
		Noise = { true, 0.0f, 0.025f, 0.015f };
		break;

	case ECameraSensorPreset::Waymo:
		Intrinsics = { 1920, 1280, 30.0f, 50.0f, 6.0f };
		Distortion = { -0.08f, 0.015f, 0.0f, 0.0f, 0.0f };
		PostProcess.VignetteIntensity = 0.2f;
		PostProcess.ChromaticAberration = 0.15f;
		PostProcess.MotionBlurAmount = 0.3f;
		PostProcess.BloomIntensity = 0.0f;
		PostProcess.LensFlareIntensity = 0.03f;
		Exposure = { true, 1.0f, 16.0f, 4.0f, 1.5f };
		Noise = { true, 0.0f, 0.01f, 0.005f };
		break;

	case ECameraSensorPreset::Custom:
	default:
		break;
	}
}

void UCameraSensorComponent::SaveCameraImage() // RenderTarget의 픽셀을 읽어 JPEG로 저장하는 함수
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("SensorData") / DataSaveConfig.SensorLabel;
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString FilePath = Dir / FString::Printf(TEXT("%06lld.jpg"), FrameCount);

	FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogCameraSensor, Warning, TEXT("RenderTarget resource unavailable for saving."));
		return;
	}

	const int32 Width = RenderTarget->SizeX;
	const int32 Height = RenderTarget->SizeY;

	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Width * Height);
	if (!RTResource->ReadPixels(Pixels))
	{
		UE_LOG(LogCameraSensor, Warning, TEXT("ReadPixels failed for frame %lld"), FrameCount);
		return;
	}

	for (FColor& Pixel : Pixels)
	{
		Pixel.A = 255;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);

	if (ImageWrapper.IsValid() == false)
	{
		UE_LOG(LogCameraSensor, Warning, TEXT("Failed to create JPEG ImageWrapper."));
		return;
	}

	ImageWrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8);
	const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed(DataSaveConfig.JPGQuality);

	FFileHelper::SaveArrayToFile(CompressedData, *FilePath);

	UE_LOG(LogCameraSensor, Verbose, TEXT("Saved %dx%d image → %s (%lld bytes)"),
		Width, Height, *FilePath, CompressedData.Num());
}

void UCameraSensorComponent::ApplyTunnelProfile(bool bInTunnel)
{
	if (bInTunnel)
	{
		CachedMinEV = Exposure.MinEV;
		CachedMaxEV = Exposure.MaxEV;
		CachedBloom = PostProcess.BloomIntensity;
		CachedNoise = Noise.GaussianStdDev;

		Exposure.MinEV -= 2.f;
		Exposure.MaxEV -= 2.f;
		PostProcess.BloomIntensity = 1.5f; // 헤드라이트 번짐
		Noise.GaussianStdDev *= 1.5f;
		ApplyPostProcessSettings();
	}
	else
	{
		Exposure.MinEV = CachedMinEV;
		Exposure.MaxEV = CachedMaxEV;
		PostProcess.BloomIntensity = CachedBloom;
		Noise.GaussianStdDev = CachedNoise;
		ApplyPostProcessSettings();
	}
}

void UCameraSensorComponent::ApplyWeatherProfile(bool bIsWeatherChanged)
{
	if (bIsWeatherChanged)
	{
		CachedWeatherMinEV = Exposure.MinEV;
		CachedWeatherMaxEV = Exposure.MaxEV;
		CachedWeatherBloom = PostProcess.BloomIntensity;
		CachedWeatherNoise = Noise.GaussianStdDev;

		Exposure.MinEV -= 0.5f;
		Exposure.MaxEV -= 2.f;
		PostProcess.BloomIntensity *= 1.5f; // 헤드라이트 번짐
		Noise.GaussianStdDev *= 1.5f;
		ApplyPostProcessSettings();
	}
	else
	{
		Exposure.MinEV = CachedWeatherMinEV;
		Exposure.MaxEV = CachedWeatherMaxEV;
		PostProcess.BloomIntensity = CachedWeatherBloom;
		Noise.GaussianStdDev = CachedWeatherNoise;
		ApplyPostProcessSettings();
	}
}
