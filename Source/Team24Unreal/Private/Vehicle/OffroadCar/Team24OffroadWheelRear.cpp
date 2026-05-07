// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/OffroadCar/Team24OffroadWheelRear.h"

UTeam24OffroadWheelRear::UTeam24OffroadWheelRear()
{
	// ===========================================================================
	// [타이어 규격 - 앞바퀴와 동일한 밸런스]
	// ===========================================================================

	WheelRadius = 50.f;
	// 뒷바퀴도 앞바퀴와 똑같이 50cm의 거대한 크기를 가집니다.
	// 스포츠카는 뒷바퀴를 더 크게 쓰기도 하지만, 오프로드 차량은 사륜구동(앞뒤 바퀴가 모두 땅을 긁으며 굴러감)이기 때문에 밸런스를 맞추기 위해 크기를 통일합니다.

	CorneringStiffness = 750.0f;
	FrictionForceMultiplier = 4.0f;
	// 뒷바퀴 역시 진흙을 파고들며 차를 밀어내야 하므로, 4.0의 강력한 접지력을 줍니다.


	// ===========================================================================
	// [서스펜션 - OffroadWheelFront에 자세한 설명 있음]
	// ===========================================================================

	SuspensionMaxRaise = 20.0f;
	SuspensionMaxDrop = 20.0f;
	// 뒷바퀴 가동 범위도 앞과 똑같이 20cm로 깁니다.
	// 가파른 언덕을 오를 때, 앞바퀴가 공중에 살짝 떠도 뒷바퀴가 땅을 끝까지 짚고 밀어줄 수 있도록 도와줍니다.

	WheelLoadRatio = 1.0f;
	SpringRate = 100.0f;
	// 뒷바퀴 스프링도 앞바퀴랑 똑같이 설정합니다.

	SpringPreload = 100.0f;
	SweepShape = ESweepShape::Shapecast;


	// ===========================================================================
	// [제동-OffroadWheelFront에 자세한 설명 있음]
	// ===========================================================================

	MaxBrakeTorque = 3000.0f;
	MaxHandBrakeTorque = 6000.0f;
	// 안전하게 속도를 줄이거나 정지할 수 있도록 브레이크 수치를 분배합니다.
}
