// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Weather/WeatherTypes.h"
#include "WeatherSubsystem.generated.h"

/**
 *
 */
//WeatherSubsystem 역할
//

class ATeam24VehiclePawn;
class ARoadActor;

UCLASS()
class TEAM24UNREAL_API UWeatherSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	void SetWeather(EWeather NewWeather); //날씨 설정
	EWeather GetCurrentWeather() const { return CurrentWeather; } //날씨 Getter

	void RegisterVehicle(class ATeam24VehiclePawn* Vehicle); //날씨에 영향받는 차량 등록
	void UnregisterVehicle(class ATeam24VehiclePawn* Vehicle); //등록된 차량 해제
	void RegisterRoad(class ARoadActor* Road); //날씨에 영향받는 도로 등록
	void UnregisterRoad(class ARoadActor* Road); //등록된 도로 해제

private:
	EWeather CurrentWeather = EWeather::Clear;

	//등록할 차량 및 도로 vector
	//vector로 저장하는 이유

	//단일포인터로 저장하면 WeatherVehicle->OnWeatherChangedDelegate.Broadcast()를 호출하면,
	//가장 마지막에 스폰되어 포인터를 덮어씌운 단 한 대의 차량(3번 차량)에게만 날씨 변경 신호가 전달된다.
	//여러 도로 및 차량에도 다 적용되게 vector에 저장하고 접근해서 사용

	TArray<TWeakObjectPtr<class ATeam24VehiclePawn>> RegisteredVehicles;
	TArray<TWeakObjectPtr<class ARoadActor>> RegisteredRoads;
};
