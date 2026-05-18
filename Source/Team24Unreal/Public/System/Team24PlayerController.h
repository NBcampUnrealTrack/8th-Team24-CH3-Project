// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Team24PlayerController.generated.h"

class UInputMappingContext;
class ATeam24VehiclePawn;
class UTextureRenderTarget2D;
class UTexture2D;
class USensorViewWidget;
class UDataViewWidget;

UCLASS()
class TEAM24UNREAL_API ATeam24PlayerController : public APlayerController
{
	GENERATED_BODY()
private:
	// ---------------------------------------------------------------------------
	// [입력 매핑 컨텍스트 설정]
	// ---------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts; //사용하는 입력 매핑 컨텍스트를 여기에 할당

	// ---------------------------------------------------------------------------
	// [스폰 및 UI 클래스 정보 보관]
	// TSubclassOf는 정의된 클래스 혹은 정의된 클래스를 상속받은 블루프린트만 넣을 수 있게 에디터에서 제한을 걸어주는 안전장치입니다.
	// ---------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category="Vehicle|Respawn")
	TSubclassOf<ATeam24VehiclePawn> VehiclePawnClass;

	//팀원 코드를 합친 후 주석해제
	UPROPERTY(EditAnywhere, Category="Vehicle|UI")
	TSubclassOf<USensorViewWidget> SensorViewWidgetClass;

	// ---------------------------------------------------------------------------
	// 멤버 변수 선언 (언리얼 5 국룰: TObjectPtr 사용)
	// ---------------------------------------------------------------------------

	UPROPERTY()
	TObjectPtr<ATeam24VehiclePawn> VehiclePawn;

	//팀원 코드를 합친 후 주석해제
	UPROPERTY()
	TObjectPtr<USensorViewWidget> SensorViewWidget;


	// ---------------------------------------------------------------------------
	// 사용하지않는 변수들
	// ---------------------------------------------------------------------------
	/*UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls")
	bool bUseSteeringWheelControls = false;

	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls", meta = (EditCondition = "bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;*/

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override; //컨트롤러의 입력 시스템을 설정합니다.
	virtual void OnPossess(APawn* InPawn) override; // 플레이어가 차량에 빙의할 때 호출됩니다.

	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn); // 차량이 파괴되었을 때 호출되는 Respawn 로직입니다.

public:
	// ---------------------------------------------------------------------------
	// 센서(카메라/라이다) 뷰포트 토글 및 상태 확인 함수
	// ---------------------------------------------------------------------------
	void ToggleSensorView(UTextureRenderTarget2D* InCameraRT); //차량 전방 카메라 화면을 켜거나 끕니다.
	void ToggleLidarView(UTexture2D* InLidarRT); //라이다(LiDAR) 센서 화면을 켜거나 끕니다.
	bool IsLidarViewVisible() const;
	//현재 라이다 위젯 화면이 켜져 있는지 확인합니다.
	//라이다 화면 활성화 여부를 반환하여, 차량 센서가 불필요하게 작동(스캔)하지 않도록 상태를 확인하는 데 쓰입니다.

	// [추가] 데이터 뷰용 입력 액션 (에디터에서 3번 키로 할당된 IA를 넣으세요)
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* IA_ToggleDataView;

	// 데이터 뷰 위젯 설정
	UPROPERTY(EditAnywhere, Category = "UI|Data")
	TSubclassOf<UDataViewWidget> DataViewWidgetClass;

	UPROPERTY()
	UDataViewWidget* DataViewWidget;

	// 타이머 및 상태 변수
	FTimerHandle DataUpdateTimerHandle;
	bool bIsDataViewVisible = true;

	// 데이터를 업데이트할 함수
	void UpdateDataUI() const;
};
