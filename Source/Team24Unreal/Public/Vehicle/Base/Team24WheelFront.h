// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "Team24WheelFront.generated.h"


/**
 * @class UTeam24WheelFront
 * @brief 카오스 물리 엔진(Chaos Physics) 기반 차량의 전륜(앞바퀴) 물리 설정을 정의하는 클래스입니다.
 *
 * [주요 역할]
 * - UChaosVehicleWheel을 상속받아 차량 전면 차축의 물리적 특성을 결정합니다.
 *
 * [활용 방법]
 * - C++에서 기본적인 앞바퀴의 뼈대를 잡아줍니다.
 */
UCLASS()
class TEAM24UNREAL_API UTeam24WheelFront : public UChaosVehicleWheel
{
	GENERATED_BODY()

public:
	UTeam24WheelFront();//생성자

};
