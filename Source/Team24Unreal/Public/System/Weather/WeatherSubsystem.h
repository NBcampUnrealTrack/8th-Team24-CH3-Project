// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Weather/WeatherTypes.h"
#include "WeatherSubsystem.generated.h"

/**
 * WeatherSubsystem 사용법
 * UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>(); 이렇게 subsystem을 받아오고
 * UWeatherPresetDataAsset* Preset = WeatherSub-> GetCurrentWeatherPreset();으로 현재 날씨의 데이터 에셋을 들고오고
 * Preset에서 값을 빼와서 그에 맞는 값으로 변경되게 한다.
 * 날씨 바뀌는거 broadcast는 pawn자체에서 키 입력으로 SetWeather을 호출해 바로 적용되게 합니다.
 * 델리게이트 연결은 각 컴포넌트에서 터널에 했던거 처럼 OnWeatherChangedDelegate로 연결하면 됩니다.
 */
//WeatherSubsystem 역할
//등록된 차량 및 도로에게 날씨가 변경됬다고 방송하는 시스템

class UWeatherPresetDataAsset;
class ATeam24VehiclePawn;
class ARoadActor;

UCLASS()
class TEAM24UNREAL_API UWeatherSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	// =================================================================================
	// UWorldSubsystem 생명주기(Lifecycle) 오버라이딩
	// 서브시스템은 맵에 배치하는 액터가 아니기 때문에 BeginPlay()나 Destroyed()가 없습니다.
	// 대신 엔진이 서브시스템을 만들고 부술 때 아래 두 함수를 자동으로 실행해 줍니다.
	// =================================================================================
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;//beginplay같은 역할
	virtual void Deinitialize() override;//Destroyed같은 역할


	void SetWeather(EWeather NewWeather); //날씨 설정
	EWeather GetCurrentWeather() const { return CurrentWeather; } //날씨 Getter
	UWeatherPresetDataAsset* FindWeatherPreset(EWeather WeatherType) const;//특정 날씨의 데이터를 검색해서 가지고 오고 싶을 때
	UWeatherPresetDataAsset* GetCurrentWeatherPreset() const;//현재 날씨의 데이터 에셋을 가져오고 싶을 때

	void RegisterVehicle(class ATeam24VehiclePawn* Vehicle); //날씨에 영향받는 차량 등록
	void UnregisterVehicle(class ATeam24VehiclePawn* Vehicle); //등록된 차량 해제
	void RegisterRoad(class ARoadActor* Road); //날씨에 영향받는 도로 등록
	void UnregisterRoad(class ARoadActor* Road); //등록된 도로 해제
private:
	EWeather CurrentWeather;

	//등록할 차량 및 도로 vector
	//vector로 저장하는 이유

	//단일포인터로 저장하면 WeatherVehicle->OnWeatherChangedDelegate.Broadcast()를 호출하면,
	//가장 마지막에 스폰되어 포인터를 덮어씌운 단 한 대의 차량(3번 차량)에게만 날씨 변경 신호가 전달된다.
	//여러 도로 및 차량에도 다 적용되게 vector에 저장하고 접근해서 사용

	TArray<TWeakObjectPtr<class ATeam24VehiclePawn>> RegisteredVehicles;
	TArray<TWeakObjectPtr<class ARoadActor>> RegisteredRoads;
};


