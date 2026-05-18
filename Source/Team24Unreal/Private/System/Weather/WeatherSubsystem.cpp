// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Weather/WeatherSubsystem.h"
#include "System/Weather/WeatherTypes.h"
#include "System/Weather/WeatherPresetDataAsset.h"
#include "System/Weather/WeatherSettings.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Actor/RoadActor.h"

void UWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentWeather = EWeather::Clear;
}

void UWeatherSubsystem::Deinitialize()
{
	RegisteredVehicles.Empty();
	RegisteredRoads.Empty();
	Super::Deinitialize();
}

void UWeatherSubsystem::SetWeather(EWeather NewWeather)
{
	if (CurrentWeather == NewWeather)
	{
		return;
	}

	CurrentWeather = NewWeather;
	UWeatherPresetDataAsset* Preset = GetCurrentWeatherPreset();

	// 1. 등록된 모든 차량 순회 및 알림
	for (TArray<TWeakObjectPtr<ATeam24VehiclePawn>>::TIterator It = RegisteredVehicles.CreateIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			// 차량 내부의 델리게이트를 통해 컴포넌트들에 신호 전파
			It->Get()->OnWeatherChangedDelegate.Broadcast(CurrentWeather);
		}
		else
		{
			It.RemoveCurrent();// 유효하지 않은 포인터 정리
		}
	}

	// 2. 등록된 모든 도로 순회 및 물리 재질 교체
	for (TArray<TWeakObjectPtr<ARoadActor>>::TIterator It = RegisteredRoads.CreateIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			// RoadActor에 함수추가
			// 도로 물리 재질 교체 로직 호출
			It->Get()->SetRoadPhysicsMaterial(Preset->RoadPhysicsMaterial);
		}
		else
		{
			It.RemoveCurrent();
		}
	}
}


UWeatherPresetDataAsset* UWeatherSubsystem::FindWeatherPreset(EWeather WeatherType) const
{
	//세팅창 데이터에 접근
	const UWeatherSettings* Settings = GetDefault<UWeatherSettings>();
	if (!Settings)
	{
		return nullptr;
	}

	// Settings 안에 있는 TMap에서 요청받은 날씨(WeatherType)를 찾습니다.
	// 주의: 여기서 FoundPtr은 '데이터 에셋 원본'이 아님
	// 이 에셋은 콘텐츠 브라우저의 어느 폴더에 있어요 라고 적힌 주소표(TSoftObjectPtr)를 가리키는 포인터입니다.
	if (const TSoftObjectPtr<UWeatherPresetDataAsset>* FoundPtr = Settings->WeatherPresets.Find(WeatherType))
	{
		return FoundPtr->LoadSynchronous();
	}
	return nullptr;
}

UWeatherPresetDataAsset* UWeatherSubsystem::GetCurrentWeatherPreset() const
{
	return FindWeatherPreset(CurrentWeather);
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
		// 로직 추가예정
	}
}

void UWeatherSubsystem::UnregisterRoad(class ARoadActor* Road)
{
	RegisteredRoads.Remove(Road);
}
