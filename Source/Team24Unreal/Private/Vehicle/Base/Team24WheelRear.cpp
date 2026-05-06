// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/Base/Team24WheelRear.h"

UTeam24WheelRear::UTeam24WheelRear()
{
	AxleType = EAxleType::Rear;
	// 역할: 물리 엔진에게 이 바퀴가 '뒤 차축(뒷바퀴)'에 장착되어 있음을 알려주는 선언입니다.(식별 태그)
	// 기능: 이 기준표를 바탕으로 물리 엔진은 급가속 시 뒤로 쏠리는 하중 이동(Squat)을 계산하거나, 후륜/사륜 구동 시 엔진 동력을 뒷바퀴에 배분하는 기준으로 사용합니다.

	bAffectedByHandbrake = true;
	// 역할: 핸드브레이크(사이드 브레이크) 조작 시 이 바퀴가 물리적으로 잠길(Lock) 것인지 결정하는 스위치입니다.
	// 기능: true로 설정하면 핸드브레이크 입력 시 뒷바퀴 회전이 멈추며 지면에서 미끄러집니다. 이를 통해 급정거나 드리프트 같은 역동적인 주행을 구현할 수 있습니다.

	bAffectedByEngine = true;
	// 역할: 엔진에서 생성된 구동력(토크)이 이 바퀴에 직접 전달될지 결정하는 스위치입니다.
	// 기능: true로 설정하면 가속 페달 입력 시 엔진의 힘을 받아 바퀴가 회전하며 차를 앞으로 밀어냅니다. 후륜 구동(RWD) 또는 사륜 구동(AWD) 차량을 만들 때 필수적인 설정입니다.
}
