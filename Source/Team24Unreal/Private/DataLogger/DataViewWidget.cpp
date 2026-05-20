// Copyright Team24, All Rights Reserved.

#include "DataLogger/DataViewWidget.h"
#include "Components/TextBlock.h"

void UDataViewWidget::UpdateSpeedDisplay(float NewSpeed) const
{
	if (SpeedText)
	{
		// km/h 단위를 문자열로 변환 (예: 60.5 km/h)
		FText SpeedValueText = FText::AsNumber(FMath::RoundToFloat(NewSpeed * 10.0f) / 10.0f);
		FText FullText = FText::Format(NSLOCTEXT("UI", "SpeedFormat", "{0}"), SpeedValueText);

		SpeedText->SetText(FullText);
	}
}
