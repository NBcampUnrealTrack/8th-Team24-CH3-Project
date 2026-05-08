// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "Team24VehiclePawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
class UCameraSensorComponent;
class ULidarSensorComponent;
class USplineFollowerComponent;
class UAgentDataLogger;
class UInputComponent;
struct FInputActionValue;

/*
 * 헤더에서 USphereComponent* 선언안하는 이유 : UE4 방식 -> TObjectPtr< >: UE5 방식 하지만 cpp에서는 가벼운 *를 사용
 */
UCLASS()
class TEAM24UNREAL_API ATeam24VehiclePawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

private:
	// ---------------------------------------------------------------------------
	// [Camera System] - 차량 내/외부 시점 제어를 위한 카메라 및 지지대
	// ---------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> FrontSpringArm; // 전방 카메라의 장착 위치를 결정해주는 지지대

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FrontCamera; // 차량 1인칭 시점에서 전방 상황을 렌더링하는 카메라

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> BackSpringArm; // 차량 뒤에서 따라오는 3인칭 시점의 거리와 위치를 결정해주는 지지대

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> BackCamera; // 차량 3인칭 시점을 보여주는 메인 카메라

	// ---------------------------------------------------------------------------
	// [Simulation Systems] - 자율주행 연구 및 데이터 수집을 위한 센서와 모듈
	// ---------------------------------------------------------------------------

	//팀원 코드 합칠시 주석해제
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<UCameraSensorComponent> CameraSensor; //카메라 센서

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<ULidarSensorComponent> LidarSensor; //라이다 센서

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<UAgentDataLogger> DataLogger; //데이터 로거

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineFollowerComponent> SplineFollower; //자동 주행


	// ---------------------------------------------------------------------------
	// [Physics System] - 차량의 물리 연산 및 주행 제어를 담당하는 시스템
	// ---------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;
	//엔진 동력(RPM, 토크), 기어 변속, 서스펜션 압축, 타이어 마찰력 등 차량 이동에 관련된 모든 물리 연산을 총괄하며, 가속, 제동, 조향을 자동차의 실제 움직임으로 변환


	// ---------------------------------------------------------------------------
	// [Input Actions] - 컨트롤러의 입력 신호를 수신하기 위한 input 슬롯
	// ※ 블루프린트에서 UInputAction 에셋을 할당해야 작동합니다.
	// ---------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ResetVehicleAction;//차량을 안전한 위치/상태로 복구하라는 입력 신호

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ToggleCameraViewAction;//1인칭 카메라 시점을 UI를 키는 입력 신호

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ToggleLidarViewAction; //라이더센서 시점을 UI를 키는 입력 신호

	// ---------------------------------------------------------------------------
	// [Flip Check System] - 차량 전복 감지 변수
	// ---------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category="Flip Check", meta = (Units = "s"))
	float FlipCheckTime; // 차량이 뒤집혔는지 검사하는 주기(시간)를 설정합니다.

	UPROPERTY(EditAnywhere, Category="Flip Check")
	float FlipCheckMinDot; // 차량이 '전복되었다'고 판단하는 기울기 기준값, 내적입니다.


    // ---------------------------------------------------------------------------
    // [Internal State Variables] - 내부 상태 추적용 변수
    // ---------------------------------------------------------------------------

	// bool bFrontCameraActive; //전방 카메라와 후방 카메라 시점 변경 변수

	bool bPreviousFlipCheck = false; // 타이머 검사 때 차량이 전복 상태였는지를 기억하는 변수입니다.

	FTimerHandle FlipCheckTimer; // 전복 검사를 반복 실행하는 타이머

public:
	ATeam24VehiclePawn(); //생성자

	// ---------------------------------------------------------------------------
	// [Vehicle Control] - 차량 제어 인터페이스 (블루프린트 및 AI/외부 호출용)
	// ---------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSteering(float SteeringValue); // 차량의 핸들링 수치를 물리 엔진에 전달하는 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoThrottle(float ThrottleValue); // 차량의 가속(엑셀) 수치를 제어 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrake(float BrakeValue); // 차량의 감속(풋 브레이크) 수치를 제어 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStart(); // 브레이크 페달을 밟는 순간의 처리를 하는 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStop(); // 브레이크 페달에서 발을 떼는 순간의 처리를 하는 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStart(); // 핸드브레이크(사이드 브레이크) 체결을 처리하는 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStop(); // 역할: 핸드브레이크 체결 해제를 처리합니다.

	//UFUNCTION(BlueprintCallable, Category="Input")
	//void DoLookAround(float YawDelta); // 주행 중 주변을 둘러보는 카메라 시점(좌우 회전) 변경을 처리합니다.


	// ---------------------------------------------------------------------------
	// [Component Getters] - 내부 부품 접근자
	// ---------------------------------------------------------------------------

	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }
	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }
	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }
	FORCEINLINE UChaosWheeledVehicleMovementComponent* GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
	//팀원 코드 합칠시 주석해제
	//FORCEINLINE UCameraSensorComponent* GetCameraSensor() const { return CameraSensor; }
	//FORCEINLINE ULidarSensorComponent* GetLidarSensor() const { return LidarSensor; }
	//FORCEINLINE UAgentDataLogger* GetDataLogger() const { return DataLogger; }

protected:

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override; //input 슬롯에 할당되어있는 입력신호를 함수와 바인딩 하는 부분
	//매개변수 변경함 UInputComponent를 전방선언도 함
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float Delta) override;

	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking); // 브레이크를 밟았을 때 차량 후미등에 불이 들어오는 '시각적 효과'를 켜고 끄는 이벤트 스위치입니다.
	//BlueprintImplementableEvent로 선언해 구현 부분은 블루프린트에서 설정함

	UFUNCTION()
	void FlippedCheck(); // 차량이 전복되었는지 계산하고 판단하는 검사 함수

private:

	// ---------------------------------------------------------------------------
	// [Input Handlers] - 향상된 입력(Enhanced Input) 수신용 함수
	// ---------------------------------------------------------------------------

	//void LookAround(const FInputActionValue& Value);
	//void ToggleCamera(const FInputActionValue& Value);

	void ResetVehicle(const FInputActionValue& Value); // '차량 리셋' 입력이 들어왔을 때 신호를 받아주는 수신기
	void ToggleSensorView(const FInputActionValue& Value); //센서 뷰를 키는 입력이 들어왔을 때 신호를 받아주는 수신기
	void ToggleLidarView(const FInputActionValue& Value);  //라이다 뷰를 키는 입력이 들어왔을 때 신호를 받아주는 수신기


	// ---------------------------------------------------------------------------
	// [Action Implementations] - 실제 동작 구현부
	// ---------------------------------------------------------------------------

	//UFUNCTION(BlueprintCallable, Category="Input")
	//void DoToggleCamera();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoResetVehicle(); //차량의 똑바로 세우는 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoToggleSensorView(); // 센서 위젯의 카메라 화면을 켜거나 끄는 함수

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoToggleLidarView(); // 라이다 위젯 화면을 켜거나 끄고, 라이다 센서의 물리적 스캔 작동을 제어하는 함수

};
