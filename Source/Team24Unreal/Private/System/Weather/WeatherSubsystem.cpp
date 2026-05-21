// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Weather/WeatherSubsystem.h"
#include "System/Weather/WeatherTypes.h"
#include "System/Weather/WeatherPresetDataAsset.h"
#include "System/Weather/WeatherSettings.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Actor/RoadActor.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"

void UWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentWeather = EWeather::Clear;
	bIsTransitioningLight = false;
}

void UWeatherSubsystem::Deinitialize()
{
	RegisteredVehicles.Empty();
	RegisteredRoads.Empty();
	Super::Deinitialize();
}

void UWeatherSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// 액터들이 맵에 있으므로 태양광(Directional Light)을 찾을 수 있습니다
	for (TActorIterator<ADirectionalLight> It(&InWorld); It; ++It)
	{
		if (ULightComponent* LightComp = It->GetLightComponent())
		{
			DirectionalLightComponent = LightComp;
			break;
		}
	}

	// 시작할 때 맑음(Clear) 데이터 에셋에서 초기 조명값을 가져옵니다.
	if (UWeatherPresetDataAsset* ClearPreset = GetCurrentWeatherPreset())
	{
		TargetLightIntensity = ClearPreset->DirectionalLightIntensity;
		TargetLightColor = ClearPreset->DirectionalLightColor;

		if (DirectionalLightComponent.IsValid())
		{
			DirectionalLightComponent->SetIntensity(TargetLightIntensity);
			DirectionalLightComponent->SetLightColor(TargetLightColor);
		}
	}
	else
	{
		TargetLightIntensity = 10.0f;
		TargetLightColor = FLinearColor::White;
	}

	//레벨에 배치되어있는 actor를 전부 돌아서 찾는다(별로 안좋은 방식같아서 차후 개선예정, 아닐 수도 있음)
	//for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	//{
	//	FString ClassName = It->GetClass()->GetName();
	//
	//	if (ClassName == TEXT("LocalFogVolume") ||
	//		ClassName == TEXT("RuntimeVirtualTextureVolume") ||
	//		ClassName == TEXT("VirtualHeightfieldMesh"))
	//	{
	//		// false(보임)를 true(숨김)
	//		It->SetActorHiddenInGame(true);
	//	}
	//}

}

void UWeatherSubsystem::Tick(float DeltaTime)
{
	// 1. 방어막: 조명 변경 중이 아니거나 태양광이 없으면 즉시 퇴근! (CPU 낭비 0%)
	if (!bIsTransitioningLight || !DirectionalLightComponent.IsValid()) return;

	bool bIntensityDone = false;
	bool bColorDone = false;

	// 2. 밝기 보간 (1.0f가 속도입니다. 더 느리게 바꾸고 싶으면 0.5f 등으로 낮추세요)
	float CurrentIntensity = DirectionalLightComponent->Intensity;
	if (FMath::IsNearlyEqual(CurrentIntensity, TargetLightIntensity, 0.01f))
	{
		DirectionalLightComponent->SetIntensity(TargetLightIntensity);
		bIntensityDone = true;
	}
	else
	{
		DirectionalLightComponent->SetIntensity(FMath::FInterpTo(CurrentIntensity, TargetLightIntensity, DeltaTime, 1.0f));
	}

	// 3. 색상 보간
	FLinearColor CurrentColor = DirectionalLightComponent->LightColor;
	if (CurrentColor.Equals(TargetLightColor, 0.01f))
	{
		DirectionalLightComponent->SetLightColor(TargetLightColor);
		bColorDone = true;
	}
	else
	{
		DirectionalLightComponent->SetLightColor(FMath::CInterpTo(CurrentColor, TargetLightColor, DeltaTime, 1.0f));
	}

	// 4. 목표에 완전히 도달했으면 스스로 스위치를 끔
	if (bIntensityDone && bColorDone)
	{
		bIsTransitioningLight = false;
		UE_LOG(LogTemp, Log, TEXT("날씨 조명 전환 완료 Tick 연산 스위치 OFF."));
	}
}

TStatId UWeatherSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWeatherSubsystem, STATGROUP_Tickables);
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
	// 3. 새로운 날씨의 조명 타겟을 설정하고 Tick 스위치 ON
	if (Preset && DirectionalLightComponent.IsValid())
	{
		TargetLightIntensity = Preset->DirectionalLightIntensity;
		TargetLightColor = Preset->DirectionalLightColor;

		bIsTransitioningLight = true; // 이 순간부터 Tick 함수가 깨어나서 일을 시작합니다.
	}

	// 눈 날씨일 때 눈이 다시 나타나게 합니다.
	//bool bIsSnow = (CurrentWeather == EWeather::Snow);

	//for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	//{
	//	FString ClassName = It->GetClass()->GetName();

	//	if (ClassName == TEXT("LocalFogVolume") ||
	//		ClassName == TEXT("RuntimeVirtualTextureVolume") ||
	//		ClassName == TEXT("VirtualHeightfieldMesh"))
	//	{
	//		// bIsSnow가 true면 화면에 나타나고, false면 숨겨집니다.
	//		It->SetActorHiddenInGame(!bIsSnow);
	//	}
	//}
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
		if (UWeatherPresetDataAsset* Preset = GetCurrentWeatherPreset())
		{
			if (Preset->RoadPhysicsMaterial)
			{
				Road->SetRoadPhysicsMaterial(Preset->RoadPhysicsMaterial);
			}
		}
	}
}

void UWeatherSubsystem::UnregisterRoad(class ARoadActor* Road)
{
	RegisteredRoads.Remove(Road);
}

