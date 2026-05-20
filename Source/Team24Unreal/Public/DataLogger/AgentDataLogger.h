// Copyright Team24, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Hazard/HazardTypes.h" // 팀원이 작성한 구조체/열거형 헤더 포함 필수!
#include "AgentDataLogger.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEAM24UNREAL_API UAgentDataLogger : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentDataLogger();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	static int32 GetUtmZone(double Longitude);
	static void LatLonToUtm(double Lat, double Lon, int32 Zone, double& OutEasting, double& OutNorthing);
	void WorldToUtm(const FVector& WorldLocation, double& OutEasting, double& OutNorthing) const;
	void CreateCsvFile();
	void AppendRow();

	UFUNCTION(BlueprintCallable, Category="Data Logger")
	void StartRecording();

	UFUNCTION(BlueprintCallable, Category="Data Logger")
	void StopRecording();

	UFUNCTION(BlueprintPure, Category="Data Logger")
	bool IsRecording() const { return bIsRecording; }

	// [추가] 델리게이트에 바인딩할 콜백 함수 (반드시 UFUNCTION 매크로 필요)
	UFUNCTION()
	void OnHazardEventReceived(const FHazardEvent& HazardEvent);

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data Logger",
		meta=(AllowPrivateAccess="true"))
	bool bEnableLogging = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data Logger",
		meta=(ClampMin="0.1", ClampMax="100.0", Units="Hz", AllowPrivateAccess="true"))
	float SaveFrequencyHz = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data Logger|UTM Reference",
		meta=(ClampMin="-90.0", ClampMax="90.0", Units="deg", AllowPrivateAccess="true"))
	double OriginLatitude = 36.4800;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data Logger|UTM Reference",
		meta=(ClampMin="-180.0", ClampMax="180.0", Units="deg", AllowPrivateAccess="true"))
	double OriginLongitude = 127.0000;

private:
	double OriginUtmEasting = 0.0;
	double OriginUtmNorthing = 0.0;
	int32 OriginUtmZone = 0;

	FString CsvFilePath;
	bool bIsRecording = false;
	float TimeSinceLastSave = 0.0f;
	float ElapsedRecordingTime = 0.0f;

	// [추가] 위험 이벤트 기록용 CSV 파일 경로
	FString HazardLogFilePath;

	// [추가] 위험 이벤트 CSV 초기화 및 데이터 추가 함수
	void CreateHazardLogFile();
	void AppendHazardLog(const FHazardEvent& HazardEvent);

	// [추가] Enum과 Bitmask를 텍스트로 변환하는 도우미 함수
	FString GetPhaseString(EHazardPhase Phase) const;
	FString GetHazardFlagsString(int32 Flags) const;
};
