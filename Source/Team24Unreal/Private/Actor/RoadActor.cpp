// Copyright Team24. All Rights Reserved.

#include "Actor/RoadActor.h"

// USplineComponent 함수들을 사용하기 위한 헤더
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

//  생성자
//  AActor를 상속받았으므로 USplineComponent를 직접 만들어 붙여야 함

ARoadActor::ARoadActor()
{
	// 도로는 매 프레임 업데이트할 게 없으므로 Tick 끔
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("RoadSpline"));

	// 스플라인을 루트로 두면 액터를 옮기면 도로 전체가 같이 움직임
	RootComponent = SplineComponent;

	// 닫힌 루프 여부 기본값 (에디터에서 언제든 변경 가능)
	// false = 시작-끝이 분리된 일반 도로
	// true  = 순환 도로 (트랙처럼 무한 반복)
	SplineComponent->SetClosedLoop(false);
}

//  스플라인 편집/액터 이동 시 자동 호출
void ARoadActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildRoadMesh();
}

//  스플라인을 따라 도로 메시를 segment 단위로 깔아줌
void ARoadActor::RebuildRoadMesh()
{
	ClearSplineMeshes();

	if (!SplineComponent || !RoadMesh) return;

	const float TotalLength = SplineComponent->GetSplineLength();
	if (TotalLength <= KINDA_SMALL_NUMBER) return;

	// 전체 길이를 SegmentLength로 균등 분할 (마지막 자투리 방지)
	const int32 NumSegments = FMath::Max(1, FMath::CeilToInt(TotalLength / SegmentLength));
	const float ActualSegLen = TotalLength / NumSegments;

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float StartDist = i * ActualSegLen;
		const float EndDist   = (i + 1) * ActualSegLen;

		USplineMeshComponent* SMC = NewObject<USplineMeshComponent>(
			this, USplineMeshComponent::StaticClass(), NAME_None, RF_Transactional);

		SMC->SetMobility(EComponentMobility::Movable);
		SMC->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);
		SMC->SetForwardAxis(ForwardAxis, false);
		SMC->SetStaticMesh(RoadMesh);
		if (RoadMaterialOverride)
		{
			SMC->SetMaterial(0, RoadMaterialOverride);
		}
		SMC->SetStartScale(MeshScale, false);
		SMC->SetEndScale(MeshScale, false);

		// 차량이 위를 달릴 수 있도록 콜리전 활성화
		SMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SMC->SetCollisionProfileName(CollisionProfile);

		// Local 좌표로 가져와야 액터를 회전/이동해도 정상 작동
		const FVector StartPos = SplineComponent->GetLocationAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
		const FVector EndPos   = SplineComponent->GetLocationAtDistanceAlongSpline(EndDist,   ESplineCoordinateSpace::Local);
		FVector StartTan = SplineComponent->GetTangentAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
		FVector EndTan   = SplineComponent->GetTangentAtDistanceAlongSpline(EndDist,   ESplineCoordinateSpace::Local);

		// 탄젠트 길이를 segment 실제 길이에 맞춰야 곡선이 출렁이지 않음
		StartTan = StartTan.GetSafeNormal() * ActualSegLen;
		EndTan   = EndTan.GetSafeNormal()   * ActualSegLen;

		SMC->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan, true);
		SMC->RegisterComponent();

		SplineMeshes.Add(SMC);
	}
}

//  기존 segment 메시들 제거
//  배열에 추적된 것뿐 아니라 액터에 붙어있는 모든 SplineMeshComponent를 찾아 제거
//  → 컴파일/재로드 후 추적 안 된 메시들도 안전하게 정리됨
void ARoadActor::ClearSplineMeshes()
{
	TArray<USplineMeshComponent*> ExistingMeshes;
	GetComponents<USplineMeshComponent>(ExistingMeshes);

	for (USplineMeshComponent* SMC : ExistingMeshes)
	{
		if (SMC)
		{
			SMC->DestroyComponent();
		}
	}

	SplineMeshes.Reset();
}
