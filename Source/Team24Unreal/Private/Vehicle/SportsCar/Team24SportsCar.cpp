// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/SportsCar/Team24SportsCar.h"
#include "Vehicle/SportsCar/Team24SportsWheelFront.h"
#include "Vehicle/SportsCar/Team24SportsWheelRear.h"
#include "ChaosWheeledVehicleMovementComponent.h"

ATeam24SportsCar::ATeam24SportsCar()
{

	GetChaosVehicleMovement()->ChassisHeight = 144.0f;
	// 무게 중심 높이(144cm)를 설정합니다.
	// 자동차 엔진과 뼈대가 바닥에 얼마나 가깝게 붙어있는지를 의미합니다.
	// 이 수치가 낮을수록 코너를 돌 때 차가 넘어지지 않고 바닥에 깔려서 안정적으로 회전할 수 있습니다.

	GetChaosVehicleMovement()->DragCoefficient = 0.31f;
	// 공기 저항 계수입니다.
	// 차가 앞으로 달릴 때 바람을 얼마나 매끄럽게 나가는지를 뜻합니다.
	// (보통 스포츠카는 0.28 ~ 0.33 사이) 수치가 낮을수록 고속 주행 시 바람의 방해를 덜 받아 속도가 더 잘 오릅니다.

	GetChaosVehicleMovement()->bLegacyWheelFrictionPosition = true;
	// 바퀴의 마찰력을 계산할 때 과거의 계산 방식을 사용할지 여부입니다.

	// ===========================================================================
	// [Wheel Setups] - 앞서 만든 타이어를 3D 모델 뼈대에 부착
	// ============================================================

	GetChaosVehicleMovement()->WheelSetups.SetNum(4); //4개의 바퀴를 장착하겠다고 선언

	// 앞바퀴 2개 조립
	GetChaosVehicleMovement()->WheelSetups[0].WheelClass = UTeam24SportsWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[0].BoneName = FName("Phys_Wheel_FL");
	GetChaosVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[1].WheelClass = UTeam24SportsWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[1].BoneName = FName("Phys_Wheel_FR");
	GetChaosVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);
	//3D 모델의 왼쪽 앞뼈대(FL)와 오른쪽 앞뼈대(FR)에 할당하라는 명령어입니다.


	// 뒷바퀴 2개 조립
	GetChaosVehicleMovement()->WheelSetups[2].WheelClass = UTeam24SportsWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[2].BoneName = FName("Phys_Wheel_BL");
	GetChaosVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[3].WheelClass = UTeam24SportsWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[3].BoneName = FName("Phys_Wheel_BR");
	GetChaosVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);
	//뒷뼈대(BL, BR)에 할당하라

	// ===========================================================================
	// [자동차 엔진 세팅]
	// ===========================================================================

	GetChaosVehicleMovement()->EngineSetup.MaxTorque = 750.0f;
	// 엔진이 바퀴를 돌려버리는 최대 회전력(토크)입니다.
	// 토크는 순간적으로 치고 나가는 펀치력입니다.
	// 750이면 높은 가속력을 보여줍니다.

	GetChaosVehicleMovement()->EngineSetup.MaxRPM = 7000.0f;
	// 자동차 엔진이 1분에 최대 몇 번 뛸 수 있는지(RPM) 설정합니다. 높을수록 고속까지 힘을 잃지 않고 달립니다.

	GetChaosVehicleMovement()->EngineSetup.EngineIdleRPM = 900.0f;
	// 가만히 서 있을 때(공회전) 엔진의 기본 박동수입니다.

	GetChaosVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.2f;
	// 엑셀에서 발을 뗐을 때, 엔진 저항 때문에 속도가 자연스럽게 줄어드는(엔진 브레이크) 현상의 강도입니다.

	GetChaosVehicleMovement()->EngineSetup.EngineRevUpMOI = 5.0f;
	// 엑셀을 밟았을 때 RPM 치솟는 반응 속도입니다. 낮을수록 빠르게 반응합니다.

	GetChaosVehicleMovement()->EngineSetup.EngineRevDownRate = 600.0f;
	// 엑셀에서 발을 뗐을 때 RPM 바늘이 떨어지는 속도입니다.

    // ===========================================================================
    // [변속기 세팅]
    // ===========================================================================

    GetChaosVehicleMovement()->TransmissionSetup.bUseAutomaticGears = true;
    GetChaosVehicleMovement()->TransmissionSetup.bUseAutoReverse = true;
    // 기어 변속(1단~5단)과 후진 기어를 엔진이 자동으로 알아서 바꿔주도록(오토매틱) 설정합니다.

    GetChaosVehicleMovement()->TransmissionSetup.FinalRatio = 2.81f;
    // 엔진의 힘을 바퀴로 전달하기 전, 전체 기어비에 곱해주는 최종 증폭 수치입니다. (가속력과 최고속도의 밸런스를 맞춥니다.)

    GetChaosVehicleMovement()->TransmissionSetup.ChangeUpRPM = 6000.0f;
    GetChaosVehicleMovement()->TransmissionSetup.ChangeDownRPM = 2000.0f;
    // 언제 기어를 변속할지 결정합니다.
    // "RPM이 6000까지 치솟으면 다음 단으로 기어를 올리고(ChangeUp), 속도가 줄어들어 2000까지 떨어지면 기어를 내려라(ChangeDown)!"

    GetChaosVehicleMovement()->TransmissionSetup.GearChangeTime = 0.2f;
    // 기어를 바꾸는 데 걸리는 시간입니다. 0.2초면 스포츠카 듀얼 클러치처럼 빠르기 바뀝니다.

    GetChaosVehicleMovement()->TransmissionSetup.TransmissionEfficiency = 0.9f;
    // 기어가 맞물려 돌아갈 때 열이나 마찰로 잃어버리는 힘을 빼고, 실제 전달되는 효율(90%)입니다.

    // 전진 1단 ~ 5단 기어 비율
    GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios.SetNum(5);
    GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[0] = 4.25f; // 1단
    GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[1] = 2.52f; // 2단
    GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[2] = 1.66f; // 3단
    GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[3] = 1.22f; // 4단
    GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[4] = 1.0f;  // 5단
    // 1단(4.25)은 언덕을 오를 때처럼 페달은 가볍게 돌지만 속도는 안 나옵니다.
    // 5단(1.0)은 평지를 달릴 때처럼 페달은 무겁지만(가속은 느림) 최고 속도는 엄청나게 빠릅니다. 단이 올라갈수록 숫자가 줄어드는 이유입니다.

    GetChaosVehicleMovement()->TransmissionSetup.ReverseGearRatios.SetNum(1);
    GetChaosVehicleMovement()->TransmissionSetup.ReverseGearRatios[0] = 4.04f;
    // 후진 기어의 비율입니다. 1단 기어와 비슷하게 강한 힘으로 천천히 움직이게 설정했습니다.

	// ===========================================================================
	// [조향 세팅]
	// ===========================================================================

	GetChaosVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::Ackermann;
	// Ackermann 조향 구조를 사용합니다.
	// 자동차가 커브를 돌 때, 안쪽 바퀴와 바깥쪽 바퀴가 그리는 원의 크기가 다릅니다.
	// 안쪽 바퀴가 더 작은 원을 그리므로 많이 꺾여야 타이어가 끌리지 않습니다.
	// 이 각도 차이를 자동으로 계산해주는 조향 시스템입니다.

	GetChaosVehicleMovement()->SteeringSetup.AngleRatio = 0.7f;
	// 기능: 위에서 말한 안쪽 바퀴와 바깥쪽 바퀴의 꺾이는 각도 차이 비율입니다.
}
