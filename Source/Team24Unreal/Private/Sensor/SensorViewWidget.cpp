// Copyright NBC, Inc. All Rights Reserved.

#include "Sensor/SensorViewWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"

void USensorViewWidget::NativeConstruct() // 위젯이 생성될 때 초기 상태를 설정하는 함수
{
	Super::NativeConstruct();

	if (SensorBorder)
		SensorBorder->SetVisibility(ESlateVisibility::Collapsed);
	if (LidarBorder)
		LidarBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void USensorViewWidget::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget) // 카메라 렌더 타겟을 UI 이미지에 연결하는 함수
{
	if (!InRenderTarget || !SensorImage)
		return;
	
	SensorImage->SetBrushResourceObject(InRenderTarget);
}

void USensorViewWidget::SetLidarRenderTarget(UTexture2D* InRenderTarget) // 라이다 BEV 텍스처를 UI 이미지에 연결하는 함수
{
	if (!InRenderTarget || !LidarImage)
		return;
	
	LidarImage->SetBrushResourceObject(InRenderTarget);
}

void USensorViewWidget::ToggleCameraView() // 카메라 뷰를 켜고 끄는 토글 함수
{
	if (!SensorBorder)
		return;

	const ESlateVisibility NewVis = IsCameraViewVisible()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::SelfHitTestInvisible;

	SensorBorder->SetVisibility(NewVis);
}

void USensorViewWidget::ToggleLidarView() // 라이다 뷰를 켜고 끄는 토글 함수
{
	if (!LidarBorder)
		return;

	const ESlateVisibility NewVis = IsLidarViewVisible()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::SelfHitTestInvisible;

	LidarBorder->SetVisibility(NewVis);
}

bool USensorViewWidget::IsCameraViewVisible() const
{
	return SensorBorder && SensorBorder->GetVisibility() != ESlateVisibility::Collapsed;
}

bool USensorViewWidget::IsLidarViewVisible() const
{
	return LidarBorder && LidarBorder->GetVisibility() != ESlateVisibility::Collapsed;
}
