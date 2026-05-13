// Copyright Team24. All Rights Reserved.

#include "Actor/TunnelTrigger.h"
#include "Team24Unreal/Team24Unreal.h"
#include "Components/BoxComponent.h"
#include "Vehicle/Base/Team24VehiclePawn.h"

ATunnelTrigger::ATunnelTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	// BoxComponent 생성 + 루트로 설정
	TunnelBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TunnelBox"));
	RootComponent = TunnelBox;

	// 박스 크기 기본값 (디테일 패널에서 조정 가능)
	TunnelBox->SetBoxExtent(FVector(500.f, 300.f, 200.f));

	// Collision 설정: OverlapAllDynamic
	TunnelBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TunnelBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TunnelBox->SetGenerateOverlapEvents(true);
}

void ATunnelTrigger::BeginPlay()
{
	Super::BeginPlay();

	// PIE 시작 시 이미 박스 안에 있는 차량 처리
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ATeam24VehiclePawn::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (ATeam24VehiclePawn* Pawn = Cast<ATeam24VehiclePawn>(Actor))
		{
			Pawn->SetInTunnel(true);
			UE_LOG(LogTeam24, Log, TEXT("Tunnel: vehicle already inside at BeginPlay"));
		}
	}
}

void ATunnelTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// 차량 폰인지 확인 후 SetInTunnel 호출
	if (ATeam24VehiclePawn* Pawn = Cast<ATeam24VehiclePawn>(OtherActor))
	{
		Pawn->SetInTunnel(true);
		UE_LOG(LogTeam24, Log, TEXT("Tunnel: vehicle entered"));
	}
}

void ATunnelTrigger::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (ATeam24VehiclePawn* Pawn = Cast<ATeam24VehiclePawn>(OtherActor))
	{
		Pawn->SetInTunnel(false);
		UE_LOG(LogTeam24, Log, TEXT("Tunnel: vehicle exited"));
	}
}
