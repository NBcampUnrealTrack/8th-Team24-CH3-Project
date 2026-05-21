// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/Base/Team24VehiclePawn.h"
#include "Team24Unreal/Team24Unreal.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "TimerManager.h"
#include "System/Team24PlayerController.h"
#include "Components/SpotLightComponent.h"
#include "System/Weather/WeatherSubsystem.h"
#include "System/Weather/WeatherTypes.h"
#include "System/Weather/WeatherPresetDataAsset.h"
#include "NiagaraComponent.h"
#include "ChaosVehicleWheel.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/AudioComponent.h"
//팀원 코드 헤더 추가
#include"Component/SplineFollowerComponent.h"
#include"Sensor/CameraSensorComponent.h"
#include"Sensor/LidarSensorComponent.h"
#include"DataLogger/AgentDataLogger.h"


ATeam24VehiclePawn::ATeam24VehiclePawn()
{
	//카메라 설정
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false; //벽에 부딪혔을 때 카메라가 차체 안으로 파고들어 오지 않도록 충돌 테스트를 끕니다.
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;//기능: 회전 지연을 켜는데 속도를 15로 높게 주어, 차가 회전할 때 카메라가 늦게 따라오게 하여 역동성을 제공
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false; // 게임 시작 시 기본 화면은 3인칭이므로, 1인칭 카메라는 일단 꺼둔 상태로 대기합니다.

	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f; // 회전 지연 속도를 2로 낮게 설정하여, 차가 급격히 꺾여도 카메라는 묵직하고 부드럽게 뒤따라오게 합니다.
	BackSpringArm->CameraLagMaxDistance = 50.0f; //지연이 아무리 심해도 차체와 카메라의 거리가 50cm 이상 벌어지지 않도록 한계치

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	//물리 시스템 초기화
	GetMesh()->SetSimulatePhysics(true); //스켈레탈 메시가 언리얼 카오스물리 엔진의 통제를 받도록 물리 시뮬레이션을 킴
	GetMesh()->SetCollisionProfileName(FName("Vehicle")); //Vehicle에서 제공하는 기본 콜리전 프리셋으로 설정

	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
	//부모 클래스가 가지고 있는 기본 무브먼트를 카오스 전용 차량 무브먼트로 강제 형변환(Cast)하여 가져옵니다.

	ChaosVehicleMovement->bReverseAsBrake = false;// 자동 후진(Reverse as Brake) 끄기

	//팀원 코드오면 주석해제
	CameraSensor = CreateDefaultSubobject<UCameraSensorComponent>(TEXT("CameraSensor"));
	CameraSensor->SetupAttachment(GetMesh());

	LidarSensor = CreateDefaultSubobject<ULidarSensorComponent>(TEXT("LidarSensor"));
	LidarSensor->SetupAttachment(GetMesh());
	LidarSensor->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));

	SplineFollower = CreateDefaultSubobject<USplineFollowerComponent>(TEXT("SplineFollower"));
	DataLogger = CreateDefaultSubobject<UAgentDataLogger>(TEXT("DataLogger"));

	FlipCheckTime = 3.0f;
	FlipCheckMinDot = -0.2f;
	//bFrontCameraActive = false;
	bPreviousFlipCheck = false;

	//헤드라이트 설정
	LeftHeadLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("LeftHeadLight"));
	LeftHeadLight->SetupAttachment(GetMesh(), FName("HeadLight_FL"));
	LeftHeadLight->SetVisibility(false); // 기본상태는 안켜둠
	LeftHeadLight->OuterConeAngle = 45.0f; // 빛이 퍼지는 각도
	LeftHeadLight->Intensity = 50000.0f;

	RightHeadLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("RightHeadLight"));
	RightHeadLight->SetupAttachment(GetMesh(), FName("HeadLight_FR"));
	RightHeadLight->SetVisibility(false);
	RightHeadLight->OuterConeAngle = 45.0f;
	RightHeadLight->Intensity = 50000.0f;


	// 날씨 파티클 컴포넌트 부착 (차량을 따라다니도록)
	WeatherParticleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeatherParticleComponent"));
	WeatherParticleComponent->SetupAttachment(GetMesh());
	WeatherParticleComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f)); // 차 지붕 위 3m 쯤에 배치
	WeatherParticleComponent->bAutoActivate = false; // 기본적으로 꺼둠

	// [Audio] 엔진 사운드 스피커 부착
	EngineSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineSoundComponent"));
	EngineSoundComponent->SetupAttachment(GetMesh());
	EngineSoundComponent->SetRelativeLocation(FVector(150.0f,0.0f,70.0f));


	bIsInTunnel=true;
}

void ATeam24VehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)// Pawn (정확히는 Actor) 클래스 안에는 이미 뼈대로 만들어진 InputComponent가 존재해서 매개변수를 변경해줘야 한다.
{

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 차량 복구 바인딩
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ATeam24VehiclePawn::ResetVehicle);
		// 카메라 뷰 토글 바인딩
		EnhancedInputComponent->BindAction(ToggleCameraViewAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::ToggleSensorView);
		// 라이다 뷰 토글 바인딩
		EnhancedInputComponent->BindAction(ToggleLidarViewAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::ToggleLidarView);
		// 날씨 맑음 바인딩
		EnhancedInputComponent->BindAction(ToggleClearWeatherAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::ToggleClearWeather);
		// 날씨 비 바인딩
		EnhancedInputComponent->BindAction(ToggleRainAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::ToggleRain);

		EnhancedInputComponent->BindAction(ToggleSnowAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::ToggleSnow);

		EnhancedInputComponent->BindAction(CyclePresetAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::CyclePreset);
		EnhancedInputComponent->BindAction(CycleLidarPresetAction, ETriggerEvent::Started, this, &ATeam24VehiclePawn::CycleLidarPreset);

// ---------------------------------------------------------------------------
		// 작동 원리
		// ---------------------------------------------------------------------------
		// 1. 규칙 제출 (Controller): 게임 시작 시, 어떤 키가 어떤 신호(InputAction)를 발생시킬지 적힌 사전(MappingContext)을 엔진에 등록이 됩니다.
		// 2. 구독 신청 (Pawn)      : BindAction을 통해 "특정 신호(InputAction)가 발생하면 내 함수를 실행해 줘"라고 엔진에 예약해 둡니다.
		// 3. 실제 실행 (Subsystem) : 유저가 키를 누르면 중앙 엔진이 사전을 확인하고 신호를 뿌려, 예약해 둔 함수를 작동시킵니다.
		// ---------------------------------------------------------------------------
	}
	else
	{
		//예외처리 에러 문구
		UE_LOG(LogTeam24, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATeam24VehiclePawn::BeginPlay()
{
	// 부모 클래스의 시작 로직을 먼저 실행합니다.
	Super::BeginPlay();

	// 전복 감지 타이머 가동
	// TimerManager를 통해 'FlipCheckTimer'를  FlipCheckTime마다 1번씩 FlippedCheck() 함수를 무한 반복(true) 실행시킵니다.
	GetWorld()->GetTimerManager().SetTimer(FlipCheckTimer, this, &ATeam24VehiclePawn::FlippedCheck, FlipCheckTime, true);

	// ==========================================================
	// 각 컴포넌트 터널 델리게이트 연결하는 곳
	// ==========================================================
	if (CameraSensor)
	{
		OnTunnelToggleDelegate.AddUObject(CameraSensor, &UCameraSensorComponent::ApplyTunnelProfile);
	}
	if (LidarSensor)
	{
		OnTunnelToggleDelegate.AddUObject(LidarSensor, &ULidarSensorComponent::ApplyTunnelProfile);
	}

	// ==========================================================
	// 각 컴포넌트 날씨 델리게이트 연결하는 곳
	// ==========================================================

	if (SplineFollower)
	{
		OnWeatherChangedDelegate.AddUObject(SplineFollower, &USplineFollowerComponent::ApplyWeatherProfile);
	}

	// ==========================================================
	// 날씨 시스템
	// ==========================================================

	// 차량 본체(물리/파티클) 날씨 변경 될 때 변경되는 값을 관리하는 함수
	OnWeatherChangedDelegate.AddUObject(this, &ATeam24VehiclePawn::ApplyWeather);

	if (UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>())
	{
		WeatherSub->RegisterVehicle(this);
	}

	//시작하자마자 View가 보이게 하는 부분
	DoToggleSensorView();
	DoToggleLidarView();
}

void ATeam24VehiclePawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// 전복 타이머해제
	GetWorld()->GetTimerManager().ClearTimer(FlipCheckTimer);

	//부모 클래스의 소멸 로직 실행
	Super::EndPlay(EndPlayReason);
}

void ATeam24VehiclePawn::Tick(float Delta)
{
	Super::Tick(Delta);

	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround(); // 차량이 현재 지면에 닿아 있는지 bool값으로 확인합니다.
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);
    //공기 마찰력 설정: GetMesh()로 스켈레탈 메쉬를 가져와 SetAngularDamping(공기 마찰력) 값을 조절합니다.
    //공중에 있을 때: 값을 높게 설정하여 바퀴가 공중에서 비정상적으로 빠르게 헛도는 것을 방지합니다.
    //땅에 있을 때: 마찰력을 낮게 돌려주어 정상적인 주행과 회전이 가능하게 합니다.
    //즉, 공기마찰력이 크면 가기 힘든거 처럼 바퀴를 못움직이게 한다

	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw; //BackSpringArm의 상대적인 Yaw 값을 체크
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);
	//현재 Yaw 값을 프레임당 일정 수치만큼 0으로 보간하여, 사용자가 조작하지 않을 때 카메라가 서서히 차량 정면을 바라보게 구현되었습니다.

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f)); //보간된 값을 SetRelativeRotation으로 갱신


	//디버그용(차량)
	if (GEngine && ChaosVehicleMovement)
	{
		// 현재 속도 (km/h)
		float CurrentSpeedKmh = ChaosVehicleMovement->GetForwardSpeed() * 0.036f;

		// 현재 엔진 회전수(RPM)
		float CurrentRPM = ChaosVehicleMovement->GetEngineRotationSpeed();

		// 현재 기어 단수 구하기
		int32 CurrentGear = ChaosVehicleMovement->GetCurrentGear();

		// 현재 엑셀을 얼마나 밟고 있는지 (0.0 ~ 1.0) -> 사실상 가속 Cmd 값
		float CurrentThrottle = ChaosVehicleMovement->GetThrottleInput();

		// 현재 브레이크를 얼마나 밟고 있는지 (0.0 ~ 1.0) -> 사실상 감속 Cmd 값
		float CurrentBrake = ChaosVehicleMovement->GetBrakeInput();

		// 화면에 출력
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Cyan,
				  FString::Printf(TEXT("[Vehicle Status] Speed: %.1f km/h | RPM: %.0f | Gear: %d | Cmd: %.2f | Brake: %.2f"),
					 CurrentSpeedKmh, CurrentRPM, CurrentGear, CurrentThrottle,CurrentBrake));
	}

	//디버그용(타이어)
	if (ChaosVehicleMovement->Wheels.Num() >= 4)
	{
		// 각 바퀴 인스턴스에 적용된 현재 마찰력을 가져옵니다.
		float GripFL = ChaosVehicleMovement->Wheels[0]->FrictionForceMultiplier; // 앞바퀴 좌측 (Front Left)
		float GripFR = ChaosVehicleMovement->Wheels[1]->FrictionForceMultiplier; // 앞바퀴 우측 (Front Right)
		float GripRL = ChaosVehicleMovement->Wheels[2]->FrictionForceMultiplier; // 뒷바퀴 좌측 (Rear Left)
		float GripRR = ChaosVehicleMovement->Wheels[3]->FrictionForceMultiplier; // 뒷바퀴 우측 (Rear Right)

		// 기존 메시지와 겹치지 않도록 첫 번째 인자(Key)를 2번으로 설정하고 노란색으로 출력합니다.
		GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("[Tire Grip] Front(L/R): %.2f / %.2f | Rear(L/R): %.2f / %.2f"),
				GripFL, GripFR, GripRL, GripRR));
	}

	if (ChaosVehicleMovement->Wheels.Num() > 0)
	{
		// 대표로 0번 바퀴(앞바퀴 좌측)가 현재 닿아있는 바닥의 피직스 머티리얼을 가져옵니다.
		UPhysicalMaterial* ContactMat = ChaosVehicleMovement->Wheels[0]->GetContactSurfaceMaterial();

		// 바닥 재질(PM)이 인식되면 그 이름과 마찰력을, 허공이거나 없으면 None과 1.0f를 반환합니다.
		FString MatName = ContactMat ? ContactMat->GetName() : TEXT("None");
		float RoadFriction = ContactMat ? ContactMat->Friction : 1.0f;

		// 기존 1번(Cyan), 2번(Yellow) 메시지와 겹치지 않게 Key를 3번으로 주고 초록색으로 출력합니다.
		GEngine->AddOnScreenDebugMessage(3, 0.0f, FColor::Green,
			FString::Printf(TEXT("[Road Surface] Material: %s | Friction: %.2f"), *MatName, RoadFriction));
	}

	//테스트용
	if (EngineSoundComponent && ChaosVehicleMovement)
	{
		// 1. 피치(음높이) 조절: RPM이 올라가면 소리도 날카로워짐
		float CurrentRPM = ChaosVehicleMovement->GetEngineRotationSpeed();
		float MaxRPM = ChaosVehicleMovement->EngineSetup.MaxRPM;
		float RPM_Ratio = FMath::Clamp(CurrentRPM / MaxRPM, 0.0f, 1.0f);

		EngineSoundComponent->SetPitchMultiplier(FMath::Lerp(0.8f, 2.0f, RPM_Ratio));

		// 가속(엑셀) 깊이에 따른 볼륨 조절: 발을 떼면 감속되며 소리가 작아짐
		float CurrentThrottle = ChaosVehicleMovement->GetThrottleInput();
		float TargetVolume = FMath::Lerp(0.4f, 1.0f, CurrentThrottle);
		EngineSoundComponent->SetVolumeMultiplier(TargetVolume);

	}
}

void ATeam24VehiclePawn::FlippedCheck()
{
	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());
	//FVector::UpVector: 월드(지구)의 절대적인 위쪽 방향(하늘)을 가리키는 고정된 화살표 (0, 0, 1)입니다.
	//GetMesh()->GetUpVector(): 자동차 지붕에서 뻗어 나가는 자동차 기준의 위쪽 방향 화살표입니다.

	//내적(Dot Product)의 성질
	//두 화살표(방향 벡터)를 내적하면, 두 화살표가 이루는 각도에 따라 1.0에서 -1.0 사이의 수치가 나옵니다.
	//1.0: 차가 평지에 똑바로 서 있음 (두 화살표가 완벽히 같은 곳을 봄)
	//0.0: 차가 옆으로 완전히 누워 있음 (두 화살표가 수직임)
	//-1.0: 차가 배를 까고 완전히 뒤집혀 있음 (두 화살표가 정반대를 봄)

	if (UpDot < FlipCheckMinDot) //현재 계산된 내적 값이 설정해 둔 최소 허용치와 비교
	{
		if (bPreviousFlipCheck) //bPreviousFlipCheck true면 차량리셋
		{
			DoResetVehicle();
		}

		//낮아지면(차가 기울어지면), 상태를 기억하는 bool 값을 true
		bPreviousFlipCheck = true;
	}
	else
	{
		// 최소 허용치보다 높으면 현상유지
		bPreviousFlipCheck = false;
	}
}

void ATeam24VehiclePawn::ResetVehicle(const FInputActionValue& Value)
{
	DoResetVehicle();
}

void ATeam24VehiclePawn::ToggleSensorView(const FInputActionValue& Value)
{
	DoToggleSensorView();
}

void ATeam24VehiclePawn::ToggleLidarView(const FInputActionValue& Value)
{
	DoToggleLidarView();
}

void ATeam24VehiclePawn::ToggleClearWeather(const FInputActionValue& Value)
{
	DoToggleClearWeather();
}

void ATeam24VehiclePawn::ToggleRain(const FInputActionValue& Value)
{
	DoToggleRain();
}

void ATeam24VehiclePawn::ToggleSnow(const FInputActionValue& Value)
{
	DoToggleSnow();
}


void ATeam24VehiclePawn::DoResetVehicle()
{
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f); //바퀴가 땅에 파묻히는 버그를 막기 위해, 그 자리에서 z축으로 50 더해서 공중으로 띄워줍니다.
	FRotator ResetRotation = GetActorRotation(); //현재 엑터의 Rotation값을 받음
	ResetRotation.Pitch = 0.0f; //차가 앞뒤로 쏠린 각도 초기화
	ResetRotation.Roll = 0.0f; // 뒤집힌 각도 초기화
	// 차량이 바라보는 방향(Yaw)은 그대로 유지

	//차량 위치 복구를 하기위한 함수 실행
	LocationRecoveryVehicle(ResetLocation, ResetRotation);

	/*
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);
	//계산한 새로운 위치와 회전값을 차량에 적용
	//ETeleportType::TeleportPhysics를 사용하여 일반적인 이동이 아닌 ‘물리적 텔레포트’임을 엔진에 알립니다.
	//차가 순간이동하는 궤적 사이에 있는 물체들과 충돌하지 않도록 물리 엔진을 잠깐 끄고 안전하게 옮겨줍니다.

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	//SetPhysicsLinearVelocity : 물체가 ‘직선 방향으로 날아가거나 이동하는 속도’를 설정
	// 차가 절벽을 향해 돌진하고 있었다면, 리셋 후에도 허공에서 계속 앞으로 날아갈 것입니다. 이를 0으로 만들어 이동 속도와 관성을 0으로 만들어서 멈추게 합니다.
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
	// SetPhysicsAngularVelocityInDegrees: 물체가 도는 회전 속도를 설정
	// 차가 절벽에서 떨어지면서 팽이처럼 돌고 있었다면, 리셋 후에도 계속 돌려고 할 것입니다.
	// 이를 0으로 만들어 회전 관성을 0으로해서 멈추게 합니다.
	*/
}

void ATeam24VehiclePawn::DoToggleSensorView()
{
	//1인칭 카메라가 보이게하는 로직 (팀원 코드 합칠시 변경)
	ATeam24PlayerController* PC = Cast<ATeam24PlayerController>(GetController());
	if (PC == nullptr)
		return;

	UTextureRenderTarget2D* CamRT = CameraSensor ? CameraSensor->GetRenderTarget() : nullptr;
	PC->ToggleSensorView(CamRT);
}

void ATeam24VehiclePawn::DoToggleLidarView()
{
	//Lidar센서가 보이게 하는 로직(팀원 코드 합칠시 변경)
	ATeam24PlayerController* PC = Cast<ATeam24PlayerController>(GetController());
	if (PC == nullptr)
		return;

	UTexture2D* LidarRT = LidarSensor ? LidarSensor->GetBevRenderTarget() : nullptr;
	PC->ToggleLidarView(LidarRT);
	if (LidarSensor)
	{
		if (PC->IsLidarViewVisible())
			LidarSensor->StartScan();
		else
			LidarSensor->StopScan();
	}
}

void ATeam24VehiclePawn::DoSteering(float SteeringValue)
{
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
    //핸들 입력값을 설정합니다.
	//장착한 Wheel의 AxleType에 따라 무슨 바퀴가 돌아갈지 결정함
    //입력값의 범위: 일반적으로 -1.0(왼쪽 최대) ~ 1.0(오른쪽 최대) 사이의 값을 받습니다. 0.0이 들어오면 핸들을 꺾지 않은 정면 상태를 유지합니다.
}

void ATeam24VehiclePawn::DoThrottle(float ThrottleValue)
{
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
	//가속 페달/엑셀 입력값을 설정합니다.
    //값이 커질수록 엔진 RPM(분당 회전수)이 상승하고, 기어 변속 시스템과 연동되어 바퀴에 더 큰 회전력를 전달합니다.
    //입력값의 범위: 일반적으로 0.0(가속 안 함) ~ 1.0(최대로 밟음) 사이의 값을 받습니다.
	ChaosVehicleMovement->SetBrakeInput(0.0f);
	//브레이크(제동 페달) 입력값을 설정합니다.
	//차량의 바퀴에 물리적인 마찰(제동력)을 가해 속도를 줄이거나 멈추게 합니다.
	//입력값의 범위: 일반적으로 0.0(브레이크 안 밟음) ~ 1.0(풀 브레이크) 사이의 값을 받습니다.
}

void ATeam24VehiclePawn::DoBrake(float BrakeValue)
{
	//DoThrottle에 각 의미 있음
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void ATeam24VehiclePawn::DoBrakeStart()
{
	BrakeLights(true);
	//블루프린트에 구현된 브레이크 등을 키는 함수를 실행시킴
}

void ATeam24VehiclePawn::DoBrakeStop()
{
	BrakeLights(false);
	//블루프린트에 구현된 브레이크 등을 끔
	ChaosVehicleMovement->SetBrakeInput(0.0f);//이거 확인해보기 있어야하는지 굳이?
}

void ATeam24VehiclePawn::DoHandbrakeStart()
{
	// 역할: 차량의 (Rear Axle)를 물리적으로 고정하여 회전을 즉시 멈추게 합니다.
	// 기능: 일반 브레이크보다 강력한 제동력을 발생시켜 급정거를 돕거나, 주행 중 뒷바퀴의 마찰력을 의도적으로 상실시켜 '드리프트(Drift)'를 유발할 수 있습니다.
	ChaosVehicleMovement->SetHandbrakeInput(true);
	BrakeLights(true);

}

void ATeam24VehiclePawn::DoHandbrakeStop()
{
	// 역할: 잠겨 있던 바퀴의 고정 상태를 풀고 다시 엔진 동력이 전달될 수 있는 상태로 복구합니다.
	// 기능: 입력값을 false로 설정하여 바퀴가 자유롭게 회전할 수 있도록 하며, 다시 정상적인 가속 및 자동차 방향이 변경 가능한 상태로 만듭니다.
	ChaosVehicleMovement->SetHandbrakeInput(false);
	BrakeLights(false);
}

void ATeam24VehiclePawn::DoToggleClearWeather()
{
	if (UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>())
	{
		WeatherSub->SetWeather(EWeather::Clear); // 강제 맑음
	}
}

void ATeam24VehiclePawn::DoToggleRain()
{
	if (UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>())
	{
		WeatherSub->SetWeather(EWeather::Rain);
	}
}

void ATeam24VehiclePawn::DoToggleSnow()
{
	if (UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>())
	{
		WeatherSub->SetWeather(EWeather::Snow);
	}
}

void ATeam24VehiclePawn::SetInTunnel(bool bNewInTunnel)
{
	UE_LOG(LogTeam24, Log, TEXT("SetInTunnel: %s"),
		bNewInTunnel ? TEXT("true") : TEXT("false"));

	bIsInTunnel = bNewInTunnel;
	OnTunnelToggleDelegate.Broadcast(bNewInTunnel);

	//터널 헤드라이트 키는 부분(빛이 너무 약함 수정예정)
	//Pawn에 SpotLight 장착예정
	if (LeftHeadLight && RightHeadLight)
	{
		LeftHeadLight->SetVisibility(bNewInTunnel);
		RightHeadLight->SetVisibility(bNewInTunnel);
	}

	// 머티리얼이 빛나는 효과(Emission)
	HeadLights(bNewInTunnel);

	if (bIsInTunnel)
	{
		BrakeLights(true);
	}
	else
	{
		BrakeLights(false);
	}
	//파티클 제어(터널)
	if (WeatherParticleComponent)
	{
		if (bIsInTunnel)
		{
			// 터널에 들어가면 파티클 끔
			WeatherParticleComponent->Deactivate();
		}
		else
		{
			// 터널에서 나왔을 때, 현재 날씨 에셋을 확인해서 비/눈 파티클이 있다면 다시 켬
			if (UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>())
			{
				UWeatherPresetDataAsset* Preset = WeatherSub->GetCurrentWeatherPreset();

				// Preset 데이터가 있고, 그 안에 WeatherParticle 에셋이 할당되어
				if (Preset && Preset->WeatherParticle)
				{
					WeatherParticleComponent->SetAsset(Preset->WeatherParticle);
					WeatherParticleComponent->Activate(true);
				}
			}
		}
	}
}

void ATeam24VehiclePawn::ApplyWeather(EWeather Weather)
{
	UWeatherSubsystem* WeatherSub = GetWorld()->GetSubsystem<UWeatherSubsystem>();
	if (!WeatherSub)
	{
		return;
	}

	UWeatherPresetDataAsset* Preset = WeatherSub->FindWeatherPreset(Weather);
	if (!Preset)
	{
		return;
	}

	// 타이어 그립(마찰력) 변경
	if (ChaosVehicleMovement)
	{
		for (UChaosVehicleWheel* Wheel : ChaosVehicleMovement->Wheels)
		{
			if (Wheel)
			{
				// 값을 읽어와서 데이터 에셋의 배율(TireFrictionScale)을 곱해 덮어씌움
				float DefaultFriction = Wheel->GetClass()->GetDefaultObject<UChaosVehicleWheel>()->FrictionForceMultiplier;
				Wheel->FrictionForceMultiplier = DefaultFriction * Preset->TireFrictionScale;
			}
		}
	}

	// 파티클 시스템 제어
	if (WeatherParticleComponent)
	{
		// 맑음(Clear)처럼 에디터에서 파티클을 비워뒀다면 (None / nullptr), else문으로 빠져서 꺼집니다.
		if (Preset->WeatherParticle)
		{
			// 비나 눈 파티클 에셋으로 갈아 끼움
			WeatherParticleComponent->SetAsset(Preset->WeatherParticle);

			// 현재 터널 밖일 때만 파티클 재생
			if (!bIsInTunnel)
			{
				WeatherParticleComponent->Activate(true);
			}
		}
		else
		{
			// 데이터 에셋에 파티클이 없다면 바로 꺼버림
			WeatherParticleComponent->Deactivate();
		}
	}
	bool bIsWeather = (Weather != EWeather::Clear);
	if (CameraSensor)
		CameraSensor->ApplyWeatherProfile(bIsWeather);
	if (LidarSensor)
		LidarSensor->ApplyWeatherProfile(bIsWeather);
}

void ATeam24VehiclePawn::LocationRecoveryVehicle(const FVector& TargetLocation, const FRotator& TargetRotation)
{
	SetActorTransform(FTransform(TargetRotation, TargetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);
	//계산한 새로운 위치와 회전값을 차량에 적용
	//ETeleportType::TeleportPhysics를 사용하여 일반적인 이동이 아닌 ‘물리적 텔레포트’임을 엔진에 알립니다.
	//차가 순간이동하는 궤적 사이에 있는 물체들과 충돌하지 않도록 물리 엔진을 잠깐 끄고 안전하게 옮겨줍니다.

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	//SetPhysicsLinearVelocity : 물체가 ‘직선 방향으로 날아가거나 이동하는 속도’를 설정
	// 차가 절벽을 향해 돌진하고 있었다면, 리셋 후에도 허공에서 계속 앞으로 날아갈 것입니다. 이를 0으로 만들어 이동 속도와 관성을 0으로 만들어서 멈추게 합니다.
 	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
 	// SetPhysicsAngularVelocityInDegrees: 물체가 도는 회전 속도를 설정
 	// 차가 절벽에서 떨어지면서 팽이처럼 돌고 있었다면, 리셋 후에도 계속 돌려고 할 것입니다.
 	// 이를 0으로 만들어 회전 관성을 0으로해서 멈추게 합니다.
 }

void ATeam24VehiclePawn::CyclePreset()
{
	if (CameraSensor)
	{
		CameraSensor->CyclePreset();
	}
}

void ATeam24VehiclePawn::CycleLidarPreset()
{
	if (LidarSensor)
	{
		LidarSensor->CyclePreset();
	}
}

