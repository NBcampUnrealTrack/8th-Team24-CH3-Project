// Copyright Team24. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineMeshComponent.h"
#include "RoadActor.generated.h"

class USplineComponent;
class UStaticMesh;
class UMaterialInterface;

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

	// ============================================================
	// [Road Mesh] - 스플라인을 따라 자동 배치되는 도로 메시
	// ============================================================

	// 스플라인을 따라 반복 배치할 도로 단면 메시
	UPROPERTY(EditAnywhere, Category = "Road|Mesh")
	TObjectPtr<UStaticMesh> RoadMesh;

	// 도로 머티리얼 오버라이드 (없으면 메시 기본 머티리얼 사용)
	UPROPERTY(EditAnywhere, Category = "Road|Mesh")
	TObjectPtr<UMaterialInterface> RoadMaterialOverride;

	// 메시의 어느 축이 "앞" 방향인지 (대부분의 도로 메시는 X)
	UPROPERTY(EditAnywhere, Category = "Road|Mesh")
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::X;

	// 한 segment의 길이(cm). 짧을수록 곡선이 부드럽지만 무거워짐
	UPROPERTY(EditAnywhere, Category = "Road|Mesh", meta=(ClampMin="50.0"))
	float SegmentLength = 500.f;

	// 폭(Y) / 두께(Z) 스케일
	UPROPERTY(EditAnywhere, Category = "Road|Mesh")
	FVector2D MeshScale = FVector2D(1.f, 1.f);

	// 차량이 위에서 달릴 수 있도록 하는 콜리전 프리셋
	UPROPERTY(EditAnywhere, Category = "Road|Mesh")
	FName CollisionProfile = FName("BlockAllDynamic");

	// 디테일 패널에서 클릭으로 강제 재빌드
	UFUNCTION(CallInEditor, Category = "Road|Mesh")
	void RebuildRoadMesh();

protected:
	// 액터 생성/이동/스플라인 편집 시 자동 호출 → 메시 재빌드
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// 도로 경로를 정의하는 스플라인 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USplineComponent> SplineComponent;

	// 동적으로 생성된 도로 segment 메시들
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshes;

	void ClearSplineMeshes();
};
