// Copyright Team24, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DataViewWidget.generated.h"

class UTextBlock;

// 텍스트 블록을 제어하기 위해 필요합니다.
UCLASS()
class TEAM24UNREAL_API UDataViewWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯 블루프린트의 TextBlock 이름이 'SpeedText'여야 합니다.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SpeedText;

public:
	// 속도 데이터를 받아 UI를 업데이트하는 함수
	void UpdateSpeedDisplay(float NewSpeed) const;

};
