// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "Team24WheelRear.generated.h"

/**
 * @class UTeam24WheelRear
 * @brief 물리 엔진 기반 차량의 후륜(뒷바퀴) 물리 설정을 정의하는 클래스입니다.
 *
 * [주요 역할]
 * - UChaosVehicleWheel을 상속받아 차량 후면 차축의 물리적 특성을 결정합니다.
 * - 주로 엔진의 구동력 전달 및 핸드브레이크 제어 로직을 담당합니다.
 *
 * [활용 방법]
 * - C++에서 공통적인 후륜 물리 뼈대를 잡아줍니다.
 */
UCLASS()
class TEAM24UNREAL_API UTeam24WheelRear : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UTeam24WheelRear();
};
