// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Team24PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include"Sensor/SensorViewWidget.h"
#include "DataLogger/DataViewWidget.h"

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
	}
}
