// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/OffroadCar/Team24OffroadCar.h"
#include "Vehicle/OffroadCar/Team24OffroadWheelFront.h"
#include "Vehicle/OffroadCar/Team24OffroadWheelRear.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

ATeam24OffroadCar::ATeam24OffroadCar()
{
    // ===========================================================================
	// [Static Mesh 설정]
	// ===========================================================================

	Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chassis"));
	Chassis->SetupAttachment(GetMesh()); // 기본 스켈레톤 자식으로 할당.

	// [왼쪽 앞바퀴]
	TireFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Front Left"));
	TireFrontLeft->SetupAttachment(GetMesh(), FName("VisWheel_FL")); // VisWheel(시각적 바퀴) 뼈대 위치에 붙입니다.
	TireFrontLeft->SetCollisionProfileName(FName("NoCollision"));
	// NoCollision을 설정한 이유는, 벽에 부딪히는 물리 계산은 스켈레톤이 하고 있기 때문입니다.
	// 껍데기까지 충돌을 켜두면 계산이 꼬이고 무거워집니다.

	// [오른쪽 앞바퀴]
	TireFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Front Right"));
	TireFrontRight->SetupAttachment(GetMesh(), FName("VisWheel_FR"));
	TireFrontRight->SetCollisionProfileName(FName("NoCollision"));
	TireFrontRight->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	// 오른쪽 바퀴를 180도 뒤집어줍니다! 안 뒤집으면 휠의 바깥쪽이 차체 안쪽을 보게 됩니다.

	// [뒷바퀴들]
	TireRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Rear Left"));
	TireRearLeft->SetupAttachment(GetMesh(), FName("VisWheel_BL"));
	TireRearLeft->SetCollisionProfileName(FName("NoCollision"));

	TireRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Rear Right"));
	TireRearRight->SetupAttachment(GetMesh(), FName("VisWheel_BR"));
	TireRearRight->SetCollisionProfileName(FName("NoCollision"));
	TireRearRight->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));


	// ===========================================================================
	// [카메라 위치 조정]
	// ===========================================================================

	GetFrontSpringArm()->SetRelativeLocation(FVector(-5.0f, -30.0f, 135.0f));
	GetBackSpringArm()->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));


	// ===========================================================================
	// [오프로드 특화 물리 세팅]
	// ===========================================================================

	GetChaosVehicleMovement()->ChassisHeight = 160.0f;
	// 스포츠카(144)보다 차체를 높게 띄워(160) 바위에 긁히지 않게 합니다.
	// 높게 띄우는 이유: 차체가 낮으면 바퀴가 장애물을 넘기도 전에 차량 하부가 장애물에 걸려 버리기 때문입니다.

	GetChaosVehicleMovement()->DragCoefficient = 0.1f;//차량이 달려나갈 때 공기가 방해하는 힘의 크기입니다.
	GetChaosVehicleMovement()->DownforceCoefficient = 0.1f;//차량이 달릴 때 공기가 차체를 아래로 누르는 힘입니다.

	// [무게 중심 강제 설정]
	GetChaosVehicleMovement()->CenterOfMassOverride = FVector(0.0f, 0.0f, 75.0f);
	GetChaosVehicleMovement()->bEnableCenterOfMassOverride = true;
	//  오프로드 차는 덩치가 커서 넘어지기 쉽습니다.
	//  그래서 차체의 무게 중심을 Z축 기준 75 높이로 명확하게 고정해 주어 안정성을 높입니다.

	//안정성이 높은 이유
	//차체를 높이면 물리적으로는 차가 위아래로 길어지면서 좌우로 뒤집히기 쉬운 상태가 됩니다.
	//이때 무게 중심을 아래쪽(Z=75)으로 낮게 고정하면, 차가 아무리 높고 덩치가 커도 무게가 바닥 쪽에 집중되는 효과가 납니다.
    //마치 아래쪽이 '오뚝이'처럼, 경사면을 주행하거나 급회전을 할 때 차체가 기울어지더라도 다시 바닥으로 내려앉으려는 복원력이 강해져서 전복 사고를 획기적으로 줄여줍니다.

	GetChaosVehicleMovement()->bLegacyWheelFrictionPosition = true;


	// ===========================================================================
	// [물리 바퀴 연결]
	// ===========================================================================
	GetChaosVehicleMovement()->WheelSetups.SetNum(4);

	// 시각적 외형은 'VisWheel' 뼈대에 붙였지만, 실제 물리 계산은 'PhysWheel' 스켈레톤에서 수행합니다.
	GetChaosVehicleMovement()->WheelSetups[0].WheelClass = UTeam24OffroadWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[0].BoneName = FName("PhysWheel_FL");
	GetChaosVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[1].WheelClass = UTeam24OffroadWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[1].BoneName = FName("PhysWheel_FR");
	GetChaosVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[2].WheelClass = UTeam24OffroadWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[2].BoneName = FName("PhysWheel_BL");
	GetChaosVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[3].WheelClass = UTeam24OffroadWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[3].BoneName = FName("PhysWheel_BR");
	GetChaosVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);


	// ===========================================================================
	// [엔진 및 구동계 세팅]
	// ===========================================================================

	GetChaosVehicleMovement()->EngineSetup.MaxTorque = 600.0f;
	GetChaosVehicleMovement()->EngineSetup.MaxRPM = 5000.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineIdleRPM = 1200.0f;
	// 스포츠카(최대 7000 RPM)와 달리 최고 RPM은 5000으로 낮지만, 공회전(Idle) RPM을 1200으로 높여서 초반 험지 돌파를 할 수 있게 세팅했습니다.

	GetChaosVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.05f;//엑셀을 뗐을 때 엔진 저항으로 속도가 줄어드는 정도입니다.
	//0.05는 낮은 수치입니다.
	//발을 떼도 차가 멈추지 않고 미끄러지듯 계속 전진하려는 성질을 줍니다.
	//오프로드에서는 장애물을 넘기 위해 탄력 주행이 중요한데, 차가 갑자기 멈추는 것을 막아줍니다.

	GetChaosVehicleMovement()->EngineSetup.EngineRevUpMOI = 5.0f;//자동차 엔진 내부 부품들이 얼마나 무거운지(관성)를 나타냅니다.
	//값이 높을수록 자통차 엔진이 무겁게 반응합니다.
	//스포츠카는 값음 낮춰서 엑셀을 밟자마자 RPM이 솟구치게 하지만, 오프로드 차량은 5.0 정도로 설정하여  바퀴가 헛도는 것을 방지하고 밀어붙이는 느낌을 줍니다.

	GetChaosVehicleMovement()->EngineSetup.EngineRevDownRate = 600.0f;//엑셀에서 발을 뗐을 때 RPM이 떨어지는 속도입니다.
	//험지에서 잠시 엑셀을 뗐다가 다시 밟을 때 엔진 힘이 죽지 않고 바로 다시 치고 나갈 수 있게 도와줍니다.


	// [사륜구동 설정]
	GetChaosVehicleMovement()->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
	GetChaosVehicleMovement()->DifferentialSetup.FrontRearSplit = 0.5f;
	// 엔진의 힘을 앞바퀴와 뒷바퀴에 5:5(0.5f)로 공평하게 나눠줍니다.
	// 네 바퀴가 모두 땅을 파고들며 굴러가는 오프로드 주행이 가능해집니다!

	GetChaosVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::AngleRatio;
	GetChaosVehicleMovement()->SteeringSetup.AngleRatio = 0.7f;
	//핸들을 꺾을 때 안쪽 바퀴와 바깥쪽 바퀴가 돌아가는 비율을 고정합니다.
}
