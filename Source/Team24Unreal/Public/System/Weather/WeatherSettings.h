// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WeatherSettings.generated.h"

enum class EWeather : uint8;
class UWeatherPresetDataAsset;
/** WeatherDataAsset을 저장하고 미리 할당하는 세팅 역할입니다. */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Weather System Settings"))
class TEAM24UNREAL_API UWeatherSettings : public UDeveloperSettings
{
    //UWeatherSettings
    // [편집] -> [프로젝트 세팅] 창에 "Weather System Settings"라는 메뉴를 생성하여
    //게임 전역에서 사용되는 날씨 프리셋 데이터들을 관리하는 글로벌 세팅 클래스입니다.
	//접근 방법: GetDefault<UWeatherSettings>()를 통해 어디서든 즉시 읽어올 수 있습니다.

	//UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Weather System Settings"))
	// Config=Game : 이 클래스에서 변경된 수치나 에셋 경로를 프로젝트 폴더의 Config/DefaultGame.ini 파일에 텍스트로 저장하겠다는 뜻입니다.
	// defaultconfig : 에디터에서 값이 변경되면 즉시 ini 파일에 덮어써서(저장) 반영하겠다는 뜻입니다.
	// meta=(DisplayName=...) : 프로젝트 세팅 창 좌측 목록에 표시될 메뉴 이름입니다.

	GENERATED_BODY()
public:
	UWeatherSettings();

	//날씨 타입(EWeather)에 따른 데이터 애셋(UWeatherPresetDataAsset) 매핑 테이블입니다.
	UPROPERTY(Config, EditAnywhere, Category = "Weather Setup")
	TMap<EWeather, TSoftObjectPtr<UWeatherPresetDataAsset>> WeatherPresets;

};
