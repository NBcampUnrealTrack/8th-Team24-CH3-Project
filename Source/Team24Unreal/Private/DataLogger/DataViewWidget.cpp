// Copyright Team24, All Rights Reserved.

#include "DataLogger/DataViewWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h" // [추가] UImage 관련 기능을 사용하기 위해 포함합니다.

void UDataViewWidget::UpdateSpeedDisplay(float NewSpeed)
{
	// 두 텍스트 블록이 모두 에디터에 잘 연결되어 있는지 확인
	if (LeadingZeroText && SpeedText)
	{
		// 1. 실수 속도를 정수로 변환 (음수 방지를 위해 0~999로 제한)
		int32 RoundedSpeed = FMath::Clamp(FMath::RoundToInt(NewSpeed), 0, 999);

		FString ZeroString = TEXT("");
		FString SpeedString = FString::FromInt(RoundedSpeed);

		// 2. 숫자의 크기에 따라 앞에 붙을 '0'의 개수를 결정합니다.
		if (RoundedSpeed < 10)         // 속도가 0~9 일 때
		{
			ZeroString = TEXT("00");   // 예: 00 / 7
		}
		else if (RoundedSpeed < 100)   // 속도가 10~99 일 때
		{
			ZeroString = TEXT("0");    // 예: 0 / 73
		}
		// 속도가 100 이상이면 ZeroString은 비어있게 됩니다 ("")

		// 3. 쪼개진 문자열을 각각의 UI 텍스트에 적용합니다.
		LeadingZeroText->SetText(FText::FromString(ZeroString));
		SpeedText->SetText(FText::FromString(SpeedString));
	}
}

void UDataViewWidget::UpdateGearDisplay(int32 NewGear)
{
	if (GearText)
	{
		FString GearString;

		// Chaos Vehicle의 기어 규칙에 따라 문자로 치환합니다.
		if (NewGear < 0)
		{
			GearString = TEXT("R"); // 후진 (Reverse)
		}
		else if (NewGear == 0)
		{
			GearString = TEXT("N"); // 중립 (Neutral)
		}
		else
		{
			// 1단 이상 전진 기어는 숫자 그대로 표시 (예: 1, 2, 3...)
			GearString = FString::FromInt(NewGear);
		}

		GearText->SetText(FText::FromString(GearString));
	}
}

void UDataViewWidget::SetLaneWarningActive(bool bIsActive)
{
	if (LaneWarningImage)
	{
		// bIsActive가 true면 화면에 보이고, false면 화면에서 숨깁니다.
		LaneWarningImage->SetVisibility(bIsActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UDataViewWidget::SetGeneralWarningActive(bool bIsActive)
{
	if (GeneralWarningImage)
	{
		GeneralWarningImage->SetVisibility(bIsActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
