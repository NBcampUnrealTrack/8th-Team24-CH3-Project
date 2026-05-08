// Copyright Team24. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoadActor.generated.h"

class USplineComponent;

/*
 * ARoadActor
 *
 * 자율주행 차량이 따라갈 도로 경로를 정의하는 액터.
 * AActor를 상속받고 안에 USplineComponent를 직접 추가하는 방식.
 */

UCLASS(DisplayName="Road Actor", ClassGroup=(Team24))
class TEAM24UNREAL_API ARoadActor : public AActor
{
	GENERATED_BODY()
 
public:
	ARoadActor();
 
	// 자율주행 컴포넌트가 호출할 게터
	USplineComponent* GetSplineComponent() const { return SplineComponent; }
	
	// 도로 전용 속성들
	
	// 도로 이름 (디버그/구분용) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road")
	FString RoadName = TEXT("Unnamed Road");
	
	// 이 도로의 제한 속도 (cm/s)
	// 0이면 무제한 (컴포넌트의 MaxSpeed 그대로 사용) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road",
		meta=(ClampMin="0"))
	float SpeedLimit = 0.f;
	
	// 자율주행에 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road")
	bool bUseForAutopilot = true;
	
private:
	// 도로 경로를 정의하는 스플라인 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineComponent> SplineComponent;
};