// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Team24PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Sensor/SensorViewWidget.h"
#include "DataLogger/DataViewWidget.h"

// [추가] Chaos 차량 무브먼트 컴포넌트를 사용하기 위해 헤더를 포함합니다.
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Component/HazardDetectorComponent.h" // 팀원 컴포넌트 헤더

void ATeam24PlayerController::BeginPlay()
{
	Super::BeginPlay();
	bAttachToPawn = true;

	//팀원 코드 받아야 주석해제
	//센서 뷰 위젯을 생성하고 뷰포트(화면)에 추가합니다
	if (SensorViewWidgetClass)
	{
		SensorViewWidget = CreateWidget<USensorViewWidget>(this, SensorViewWidgetClass);
		if (SensorViewWidget)
		{
	      // Z-Order를 설정해 기본 UI보다 위에 덮이도록 우선순위를 줍니다.
			SensorViewWidget->AddToViewport(10);
	      // 화면에는 보이지만 마우스 클릭은 통과하도록(SelfHitTestInvisible) 설정합니다.
			SensorViewWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	// 데이터 뷰 위젯 미리 생성 (꺼진 상태)
	if (DataViewWidgetClass)
	{
		DataViewWidget = CreateWidget<UDataViewWidget>(this, DataViewWidgetClass);
		if (DataViewWidget)
		{
			DataViewWidget->AddToViewport(); // 레이어 우선순위 5
		}
	}
	GetWorldTimerManager().SetTimer(DataUpdateTimerHandle, this, &ATeam24PlayerController::UpdateDataUI, 0.1f, true);

	if (VehiclePawn)
	{
		if (UHazardDetectorComponent* HazardDetector = VehiclePawn->FindComponentByClass<UHazardDetectorComponent>())
		{
			HazardDetector->OnHazardDetected.AddDynamic(this, &ATeam24PlayerController::OnHazardEventReceived);
		}
	}
}

void ATeam24PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 해당 컨트롤러가 로컬 플레이어(현재 컴퓨터에서 플레이하는 유저)일 때만 입력을 등록합니다.
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				// MappingContext을 엔진의 Subsystem에 전부 할당합니다.
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

void ATeam24PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 탑승한 폰이 ATeam24VehiclePawn 타입인지 확인(CastChecked) 후 저장합니다.
	VehiclePawn = CastChecked<ATeam24VehiclePawn>(InPawn);
	// 차량이 파괴될 때(OnDestroyed) 컨트롤러의 OnPawnDestroyed 함수가 자동 실행되도록 구독(AddDynamic)합니다.
	VehiclePawn->OnDestroyed.AddDynamic(this, &ATeam24PlayerController::OnPawnDestroyed);
}

void ATeam24PlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	// 월드에 배치된 시작 지점(PlayerStart)을 전부 찾습니다.
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// 첫 번째 시작 지점의 위치와 회전값(Transform)을 가져옵니다.
		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		// 해당 위치에 새 차량(VehiclePawnClass)을 소환하고, 성공적으로 소환되면 다시 Possess합니다.
		if (ATeam24VehiclePawn* RespawnedVehicle = GetWorld()->SpawnActor<ATeam24VehiclePawn>(VehiclePawnClass, SpawnTransform))
		{
			Possess(RespawnedVehicle);
		}
	}
}

void ATeam24PlayerController::ToggleSensorView(UTextureRenderTarget2D* InCameraRT)
{

	if (!SensorViewWidget)
	{
		return;
	}

	// UI 위젯에 카메라 렌더 타겟 데이터를 넘겨준 뒤, 위젯 내부의 토글 함수를 실행시킵니다.
	if (InCameraRT)
	{
		SensorViewWidget->SetRenderTarget(InCameraRT);
	}
	SensorViewWidget->ToggleCameraView();
}

void ATeam24PlayerController::ToggleLidarView(UTexture2D* InLidarRT)
{
	if (!SensorViewWidget)
	{
		return;
	}

	// UI 위젯에 라이다 데이터를 넘겨준 뒤, 위젯 내부의 토글 함수를 실행시킵니다.
	if (InLidarRT)
	{
		SensorViewWidget->SetLidarRenderTarget(InLidarRT);
	}
	SensorViewWidget->ToggleLidarView();
}

bool ATeam24PlayerController::IsLidarViewVisible() const
{
    // 위젯이 존재하고, 위젯 내의 라이다 화면이 켜져 있다면 true를 반환합니다.
	return SensorViewWidget && SensorViewWidget->IsLidarViewVisible();
}


void ATeam24PlayerController::UpdateDataUI() const
{
	// OnPossess에서 VehiclePawn이 이미 할당되어 있다고 가정합니다.
	if (VehiclePawn && DataViewWidget)
	{
		// 차량의 속도를 계산 (cm/s -> km/h)
		float CurrentSpeed = VehiclePawn->GetVelocity().Size() * 0.036f;

		// 위젯에 속도 전달
		DataViewWidget->UpdateSpeedDisplay(CurrentSpeed);

		// 2. [추가] 차량에서 Chaos 무브먼트 컴포넌트를 찾아 기어 값을 가져옵니다.
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = VehiclePawn->FindComponentByClass
			<UChaosWheeledVehicleMovementComponent>())
		{
			int32 CurrentGear = VehicleMovement->GetCurrentGear();

			// 위젯에 기어 값 전달
			DataViewWidget->UpdateGearDisplay(CurrentGear);
		}
	}
}

void ATeam24PlayerController::OnHazardEventReceived(const FHazardEvent& HazardEvent)
{
	// 1. 이벤트의 상태(Phase)에 따라 현재 활성화된 위험(CurrentHazardFlags)을 업데이트합니다.
	if (HazardEvent.Phase == EHazardPhase::Enter)
	{
		// 비트 OR 연산: 새 위험을 추가합니다.
		CurrentHazardFlags |= HazardEvent.ActiveFlags;
	}
	else if (HazardEvent.Phase == EHazardPhase::Exit)
	{
		// 비트 AND NOT 연산: 해당 위험만 뺍니다. (다른 위험은 유지됨)
		CurrentHazardFlags &= ~HazardEvent.ActiveFlags;
	}

	// 2. 그룹화 로직 (UI 갱신)
	if (DataViewWidget)
	{
		// '경로 이탈' 비트가 하나라도 켜져 있는지 확인
		bool bLaneDeparture = (CurrentHazardFlags & (int32)EHazardFlags::LaneDeparture) != 0;

		// '경로 이탈'을 제외한(~) 나머지 비트(미끄러짐, 횡G, 추락 등) 중 하나라도 켜져 있는지 확인
		bool bGeneralHazard = (CurrentHazardFlags & ~(int32)EHazardFlags::LaneDeparture) != 0;

		// 위젯 함수 호출
		DataViewWidget->SetLaneWarningActive(bLaneDeparture);
		DataViewWidget->SetGeneralWarningActive(bGeneralHazard);
	}
}
