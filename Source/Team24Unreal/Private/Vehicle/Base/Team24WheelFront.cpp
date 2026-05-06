// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/Base/Team24WheelFront.h"
#include "UObject/ConstructorHelpers.h"

UTeam24WheelFront::UTeam24WheelFront()
{
	AxleType = EAxleType::Front;
	// 역할: 물리 엔진에게 이 바퀴가 '앞 차축(앞바퀴)'에 장착되어 있음을 알려주는 선언입니다.(식별 태그)
	// 기능: 이 기준표를 바탕으로 물리 엔진은 급가속/급제동 시 앞으로 쏠리는 하중 이동을 계산하거나, 엔진 동력을 배분하는 등 앞/뒤 물리량을 다르게 적용합니다.

	bAffectedBySteering = true;
	// 역할: 핸들(Steering)을 꺾었을 때 이 바퀴가 좌우로 회전할지 결정하는 스위치입니다.
	// 기능: true로 설정하면 핸들 조작에 맞춰 바퀴가 꺾이며 방향 전환이 가능해지고(주로 앞바퀴), false로 설정하면 조향의 영향을 받지 않고 정면으로 고정됩니다(주로 뒷바퀴).

	MaxSteerAngle = 40.f;
	// 역할: 핸들을 돌렸을 때 바퀴가 물리적으로 꺾일 수 있는 최대 각도를 설정합니다.
	// 기능: 이 수치가 높을수록 핸들을 끝까지 돌렸을 때 급격하게 꺽이고, 낮을수록 회전 부드럽게 꺽입니다.
	// 40.f는 좌우로 각각 최대 40도까지 회전할 수 있다는 의미입니다.
}
