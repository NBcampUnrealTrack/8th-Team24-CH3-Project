// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Team24OffroadCar.generated.h"

/**
 *
 */
UCLASS()
class TEAM24UNREAL_API ATeam24OffroadCar : public ATeam24VehiclePawn
{
	GENERATED_BODY()
private:
	// ===========================================================================
	// 스태틱 메시(Static Mesh)를 따로 추가한 이유
	// ===========================================================================
	// 스포츠카(SportsCar): 스켈레톤(뼈대) 에셋 자체가 이미 '완성된 자동차 외형'을 그대로 가지고 있습니다. 그래서 뼈대만 부르면 외형도 같이 따라옵니다.
	// 오프로드(Offroad): 스켈레톤 에셋이 자동차 껍데기 없이 '모터와 차축(Axle)' 모양으로만 생겼습니다.
	// 따라서, 카오스 물리 엔진이 굴려줄 '뼈대' 위에, 우리 눈에 보일 Static Mesh를  씌워주기 위해 선언했습니다.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Meshes, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Chassis;// 오프로드 차량의 몸체입니다

	// 4개의 타이어 외형입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Meshes, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireFrontLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Meshes, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireFrontRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Meshes, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireRearLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Meshes, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireRearRight;

public:
	ATeam24OffroadCar();
};
