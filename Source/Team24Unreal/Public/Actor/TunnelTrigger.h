// Copyright Team24. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunnelTrigger.generated.h"

class UBoxComponent;

UCLASS()
class TEAM24UNREAL_API ATunnelTrigger : public AActor
{
	GENERATED_BODY()

public:
	ATunnelTrigger();

protected:
	virtual void BeginPlay() override;

	// Actor의 Overlap 이벤트 (블루프린트의 OnActorBeginOverlap에 해당)
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
	// 터널 영역 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunnel",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TunnelBox;
};
