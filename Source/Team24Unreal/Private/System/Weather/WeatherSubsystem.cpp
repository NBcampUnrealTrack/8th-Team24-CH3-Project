// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Weather/WeatherSubsystem.h"
#include "System/Weather/WeatherTypes.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Actor/RoadActor.h"

void UWeatherSubsystem::SetWeather(EWeather NewWeather)
{
	if (CurrentWeather == NewWeather)
	{
		return;
	}

	CurrentWeather = NewWeather;

	// 1. 등록된 모든 차량 순회 및 알림
	for (TArray<TWeakObjectPtr<ATeam24VehiclePawn>>::TIterator It = RegisteredVehicles.CreateIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			// 차량 내부의 델리게이트를 통해 컴포넌트들에 신호 전파
			It->Get()->OnWeatherChangedDelegate.Broadcast(CurrentWeather);
		}
		else { It.RemoveCurrent(); } // 유효하지 않은 포인터 정리
	}

	// 2. 등록된 모든 도로 순회 및 물리 재질 교체
	for (TArray<TWeakObjectPtr<ARoadActor>>::TIterator It = RegisteredRoads.CreateIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			// 도로 물리 재질 교체 로직 호출
			// It->Get()->UpdateRoadPhysicsByWeather(CurrentWeather);
		}
		else { It.RemoveCurrent(); }
	}
}

void UWeatherSubsystem::RegisterVehicle(class ATeam24VehiclePawn* Vehicle)
{
	if (Vehicle)
	{
		RegisteredVehicles.AddUnique(Vehicle);
		// 등록 시점에 현재 날씨 즉시 적용(차량)
		Vehicle->OnWeatherChangedDelegate.Broadcast(CurrentWeather);
	}
}

void UWeatherSubsystem::UnregisterVehicle(class ATeam24VehiclePawn* Vehicle)
{
	RegisteredVehicles.Remove(Vehicle);
}

void UWeatherSubsystem::RegisterRoad(class ARoadActor* Road)
{
	if (Road)
	{
		RegisteredRoads.AddUnique(Road);
		// 등록 시점에서 현재 날씨 즉시 적용(도로)
	}
}

void UWeatherSubsystem::UnregisterRoad(class ARoadActor* Road)
{
	RegisteredRoads.Remove(Road);
}
