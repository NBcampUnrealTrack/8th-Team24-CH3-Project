// Copyright Team24. All Rights Reserved.

#include "Actor/RoadActor.h"

// USplineComponent 함수들을 사용하기 위한 헤더
#include "Components/SplineComponent.h"

//  생성자
//  AActor를 상속받았으므로 USplineComponent를 직접 만들어 붙여야 함

ARoadActor::ARoadActor()
{
	// 도로는 매 프레임 업데이트할 게 없으므로 Tick 끔 
	PrimaryActorTick.bCanEverTick = false;
	
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("RoadSpline"));
	
	// 스플라인을 루트로 두면 액터를 옮기면 도로 전체가 같이 움직임
	RootComponent = SplineComponent;
 
	// 에디터에서 다른 SplineActor와 시각적으로 구분되도록 색상 변경 (보라색)
	SplineComponent->EditorUnselectedSplineSegmentColor = FLinearColor(0.5f, 0.2f, 0.8f);
	SplineComponent->EditorSelectedSplineSegmentColor = FLinearColor(1.0f, 0.4f, 1.0f);
 
	// 닫힌 루프 여부 기본값 (에디터에서 언제든 변경 가능)
	// false = 시작-끝이 분리된 일반 도로
	// true  = 순환 도로 (트랙처럼 무한 반복)
	SplineComponent->SetClosedLoop(false);
}