// Copyright Team24, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DataViewWidget.generated.h"

class UTextBlock;
class UImage;

// 텍스트 블록을 제어하기 위해 필요합니다.
UCLASS()
class TEAM24UNREAL_API UDataViewWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// [추가] 앞자리의 0을 표시할 텍스트 (예: 어두운 회색)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* LeadingZeroText;

	// 위젯 블루프린트의 TextBlock 이름이 'SpeedText'여야 합니다.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SpeedText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GearText;

	// [추가] 경로 이탈 경고 이미지
	UPROPERTY(meta = (BindWidget))
	UImage* LaneWarningImage;

	// [추가] 일반 위험 경고 이미지 (나머지 6종)
	UPROPERTY(meta = (BindWidget))
	UImage* GeneralWarningImage;

public:
	// 속도 데이터를 받아 UI를 업데이트하는 함수
	void UpdateSpeedDisplay(float NewSpeed);
	void UpdateGearDisplay(int32 NewGear);

	// [추가] 경고 텍스트의 가시성(보이기/숨기기)을 설정하는 함수
	void SetLaneWarningActive(bool bIsActive);
	void SetGeneralWarningActive(bool bIsActive);
};
