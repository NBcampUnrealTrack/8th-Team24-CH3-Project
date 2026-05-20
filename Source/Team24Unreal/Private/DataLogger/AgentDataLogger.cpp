// Copyright Team24. All Rights Reserved.

#include "DataLogger/AgentDataLogger.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
// 디버그 드로잉을 위한 헤더 추가
// #include "DrawDebugHelpers.h"
// 팀원이 만든 컴포넌트 헤더 포함
#include "Component/HazardDetectorComponent.h"

UAgentDataLogger::UAgentDataLogger()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}



void UAgentDataLogger::BeginPlay()
{
	Super::BeginPlay();

	OriginUtmZone = GetUtmZone(OriginLongitude);
	LatLonToUtm(OriginLatitude, OriginLongitude, OriginUtmZone, OriginUtmEasting, OriginUtmNorthing);

	if (bEnableLogging)
	{
		StartRecording();
	}

	// 1. Hazard용 CSV 파일 생성 및 헤더 작성
	CreateHazardLogFile();

	// 2. 차량에서 컴포넌트 찾아 구독 (로거가 차량에 부착된 Component라면 GetOwner() 사용, Actor라면 타겟 차량 포인터 사용)
	AActor* TargetVehicle = GetOwner(); // 차량 포인터로 적절히 수정하세요.

	if (TargetVehicle)
	{
		// 차량에서 UHazardDetectorComponent 찾기
		UHazardDetectorComponent* HazardDetector = TargetVehicle->FindComponentByClass<UHazardDetectorComponent>();

		if (HazardDetector)
		{
			// 팀원이 열어둔 OnHazardDetected 델리게이트에 내 콜백 함수 연결
			HazardDetector->OnHazardDetected.AddDynamic(this, &UAgentDataLogger::OnHazardEventReceived);
			UE_LOG(LogTemp, Log, TEXT("DataLogger: Hazard Detector에 성공적으로 바인딩되었습니다."));
		}
	}
}

void UAgentDataLogger::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopRecording();
	Super::EndPlay(EndPlayReason);
}


void UAgentDataLogger::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRecording)
	{
		return;
	}

	ElapsedRecordingTime += DeltaTime;
	TimeSinceLastSave += DeltaTime;

	const float SaveInterval = 1.0f / FMath::Max(SaveFrequencyHz, 0.1f);
	if (TimeSinceLastSave >= SaveInterval)
	{
		AppendRow();
		TimeSinceLastSave -= SaveInterval;
/*
		// ---------------------------------------------------------
		// 2. [VFX 추가] 이동 궤적 시각화 (디버그 스피어)
		// ---------------------------------------------------------
		if (const AActor* Owner = GetOwner())
		{
			// 차량의 현재 위치를 가져옵니다.
			const FVector CurrentLocation = Owner->GetActorLocation();

			// 차량의 위치에 구(Sphere)를 그립니다.
			// 인자: World, 위치, 반지름(Radius), 구의 분할 수(Segments), 색상, 영구유지 여부, 유지시간(초), 두께
			DrawDebugSphere(
				GetWorld(),           // 현재 게임 월드
				CurrentLocation,      // 구를 그릴 위치 (차량 위치)
				25.0f,                // 반지름 크기 (25cm)
				12,                   // 구를 구성하는 선의 개수 (퀄리티)
				FColor::Red,          // 색상 (빨간색)
				false,                // 영구적으로 남길지 여부 (false = 시간 지나면 사라짐)
				5.0f                  // 화면에 유지되는 시간 (5초 뒤 사라짐)
			);
		}
		// ---------------------------------------------------------
*/
	}

}

void UAgentDataLogger::StartRecording()
{
	if (bIsRecording)
	{
		return;
	}

	CreateCsvFile();
	bIsRecording = true;
	TimeSinceLastSave = 0.0f;
	ElapsedRecordingTime = 0.0f;
}

void UAgentDataLogger::StopRecording()
{
	bIsRecording = false;
}

void UAgentDataLogger::CreateCsvFile()
{
	const FString OutputDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Output"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*OutputDir);

	const FDateTime Now = FDateTime::Now();
	const FString FileName = FString::Printf(
		TEXT("AgentData-%04d_%02d-%02d-%02d-%02d-%02d.csv"),
		Now.GetYear(), Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond()
	);

	CsvFilePath = FPaths::Combine(OutputDir, FileName);

	const FString Header =
		TEXT("Timestamp,World_X,World_Y,World_Z,UTM_Easting,UTM_Northing,UTM_Zone,Velocity_kmh,Yaw\n"
	);
	FFileHelper::SaveStringToFile(Header, *CsvFilePath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM
	);

	UE_LOG(LogTemp, Log, TEXT("[AgentDataLogger] Recording to: %s  (%.1f Hz)"), *CsvFilePath, SaveFrequencyHz);
}

void UAgentDataLogger::AppendRow()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
		return;

	const FVector WorldLoc = Owner->GetActorLocation();        // cm
	const FRotator WorldRot = Owner->GetActorRotation();
	const FVector Velocity = Owner->GetVelocity();             // cm/s

	const double SpeedKmh = Velocity.Size() * 0.01 * 3.6;
	const double Yaw = WorldRot.Yaw;

	double UtmEasting = 0.0;
	double UtmNorthing = 0.0;
	WorldToUtm(WorldLoc, UtmEasting, UtmNorthing);

	const FString Row = FString::Printf(
		TEXT("%.3f,%.2f,%.2f,%.2f,%.4f,%.4f,%d,%.2f,%.4f\n"),
		ElapsedRecordingTime,
		WorldLoc.X, WorldLoc.Y, WorldLoc.Z,
		UtmEasting, UtmNorthing, OriginUtmZone,
		SpeedKmh,
		Yaw
	);

	FFileHelper::SaveStringToFile(Row, *CsvFilePath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(),
		EFileWrite::FILEWRITE_Append
	);
}

void UAgentDataLogger::WorldToUtm(const FVector& WorldLocation,
	double& OutEasting, double& OutNorthing) const
{
	const double OffsetEastM  =  WorldLocation.X * 0.01;
	const double OffsetNorthM = -WorldLocation.Y * 0.01;

	OutEasting  = OriginUtmEasting  + OffsetEastM;
	OutNorthing = OriginUtmNorthing + OffsetNorthM;
}

int32 UAgentDataLogger::GetUtmZone(double Longitude)
{
	return FMath::FloorToInt((Longitude + 180.0) / 6.0) + 1;
}

void UAgentDataLogger::LatLonToUtm(double Lat, double Lon, int32 Zone, double& OutEasting, double& OutNorthing)
{
	// WGS-84 ellipsoid constants
	constexpr double a  = 6378137.0;            // semi-major axis (m)
	constexpr double f  = 1.0 / 298.257223563;  // flattening
	constexpr double k0 = 0.9996;               // UTM scale factor

	const double e2 = 2.0 * f - f * f;          // first eccentricity squared
	const double ep2 = e2 / (1.0 - e2);         // second eccentricity squared

	const double LatRad = FMath::DegreesToRadians(Lat);
	const double CentralMeridian = (Zone - 1) * 6.0 - 180.0 + 3.0;
	const double DeltaLon = FMath::DegreesToRadians(Lon - CentralMeridian);

	const double SinLat = FMath::Sin(LatRad);
	const double CosLat = FMath::Cos(LatRad);
	const double TanLat = FMath::Tan(LatRad);

	const double N = a / FMath::Sqrt(1.0 - e2 * SinLat * SinLat);
	const double T = TanLat * TanLat;
	const double C = ep2 * CosLat * CosLat;
	const double A = CosLat * DeltaLon;

	// Meridional arc (M) — series expansion
	const double e4 = e2 * e2;
	const double e6 = e4 * e2;
	const double M = a * (
		(1.0 - e2 / 4.0 - 3.0 * e4 / 64.0  - 5.0 * e6 / 256.0) * LatRad
		- (3.0 * e2 / 8.0 + 3.0 * e4 / 32.0 + 45.0 * e6 / 1024.0) * FMath::Sin(2.0 * LatRad)
		+ (15.0 * e4 / 256.0 + 45.0 * e6 / 1024.0) * FMath::Sin(4.0 * LatRad)
		- (35.0 * e6 / 3072.0) * FMath::Sin(6.0 * LatRad));

	const double A2 = A * A;
	const double A4 = A2 * A2;
	const double A6 = A4 * A2;

	OutEasting = k0 * N * (
		A
		+ (1.0 - T + C) * A2 * A / 6.0
		+ (5.0 - 18.0 * T + T * T + 72.0 * C - 58.0 * ep2) * A4 * A / 120.0
	) + 500000.0;   // false easting

	OutNorthing = k0 * (M + N * TanLat * (
		A2 / 2.0
		+ (5.0 - T + 9.0 * C + 4.0 * C * C) * A4 / 24.0
		+ (61.0 - 58.0 * T + T * T + 600.0 * C - 330.0 * ep2) * A6 / 720.0
	));

	// Southern hemisphere offset
	if (Lat < 0.0)
		OutNorthing += 10000000.0;
}

// 델리게이트 콜백: 위험 이벤트가 방송(Broadcast)될 때마다 자동으로 실행됨
void UAgentDataLogger::OnHazardEventReceived(const FHazardEvent& HazardEvent)
{
	// 이벤트가 들어오면 바로 CSV에 기록
	AppendHazardLog(HazardEvent);
}


// CSV 파일 초기화 및 첫 줄(헤더) 작성
void UAgentDataLogger::CreateHazardLogFile()
{
	// [수정] Saved/Logs 가 아니라 기본 로거와 동일한 Output 폴더로 지정합니다.
	FString Directory = FPaths::ProjectDir() / TEXT("Output");

	// 폴더가 없으면 생성합니다 (안전 장치)
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectory(*Directory);
	}

	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));

	// 일반 주행 데이터와 구분되도록 파일명에 HazardLog 명시
	HazardLogFilePath = Directory / FString::Printf(TEXT("HazardLog_%s.csv"), *Timestamp);

	// CSV 첫 줄 (컬럼명)
	// [수정됨] 위험 문자열과 상세 물리값(Slip, G, Yaw, CTE, Roll) 컬럼 추가
	FString Header = TEXT("TimeStamp,Phase,ActiveHazards,Speed(km/h),World_X,World_Y,World_Z,UTM_Easting,UTM_Northing,SlipAngleDeg,LateralG,YawRate,CTE,RollDeg\n");
	FFileHelper::SaveStringToFile(Header, *HazardLogFilePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

// 전달받은 구조체 데이터를 CSV 포맷으로 변환하여 이어붙이기
void UAgentDataLogger::AppendHazardLog(const FHazardEvent& HazardEvent)
{
	// 속도 단위 변환
	float SpeedKmh = HazardEvent.Speed * 0.036f;

	// 좌표 변환 (이전에 만드신 함수 호출)
	double UtmEasting = 0.0;
	double UtmNorthing = 0.0;
	WorldToUtm(HazardEvent.WorldLocation, UtmEasting, UtmNorthing);

	// [추가] 상태와 종류를 텍스트로 변환
	FString PhaseStr = GetPhaseString(HazardEvent.Phase);
	FString FlagsStr = GetHazardFlagsString(HazardEvent.ActiveFlags);

	// 구조체의 모든 필드를 CSV 문자열로 포맷팅
	// %s에 FString을 넣을 때는 반드시 앞에 *를 붙여야 합니다.
	FString Row = FString::Printf(TEXT("%f,%s,%s,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n"),
		HazardEvent.TimeStamp,
		*PhaseStr,             // 진입/유지/해제 텍스트
		*FlagsStr,             // 무슨 위험인지 텍스트
		SpeedKmh,
		HazardEvent.WorldLocation.X,
		HazardEvent.WorldLocation.Y,
		HazardEvent.WorldLocation.Z,
		UtmEasting,
		UtmNorthing,
		HazardEvent.SlipAngleDeg,  // 타이어 미끄러짐 각도
		HazardEvent.LateralG,      // 횡가속도
		HazardEvent.YawRate,       // 차량 휘청거림
		HazardEvent.CrossTrackError, // 경로 이탈 오차 (CTE)
		HazardEvent.RollDeg        // 전복 각도
	);

	FFileHelper::SaveStringToFile(Row, *HazardLogFilePath, FFileHelper::EEncodingOptions::ForceUTF8, &IFileManager::Get(), FILEWRITE_Append);
}

// -------------------------------------------------------------
// [추가] Helper Functions : 숫자를 문자열로 예쁘게 변환해주는 함수
// -------------------------------------------------------------

FString UAgentDataLogger::GetPhaseString(EHazardPhase Phase) const
{
	switch(Phase)
	{
	case EHazardPhase::Enter:   return TEXT("Enter");
	case EHazardPhase::Sustain: return TEXT("Sustain");
	case EHazardPhase::Exit:    return TEXT("Exit");
	default:                    return TEXT("Unknown");
	}
}

FString UAgentDataLogger::GetHazardFlagsString(int32 Flags) const
{
	if (Flags == 0) return TEXT("None");

	TArray<FString> ActiveHazards;

	// Bitwise AND 연산(&)을 통해 어떤 위험 플래그가 켜져 있는지 각각 검사합니다.
	if (Flags & (int32)EHazardFlags::Skid)           ActiveHazards.Add(TEXT("Skid"));
	if (Flags & (int32)EHazardFlags::HighLateralG)   ActiveHazards.Add(TEXT("HighLatG"));
	if (Flags & (int32)EHazardFlags::YawInstability) ActiveHazards.Add(TEXT("YawInstability"));
	if (Flags & (int32)EHazardFlags::LaneDeparture)  ActiveHazards.Add(TEXT("LaneDeparture"));
	if (Flags & (int32)EHazardFlags::RolloverRisk)   ActiveHazards.Add(TEXT("RolloverRisk"));
	if (Flags & (int32)EHazardFlags::HarshManeuver)  ActiveHazards.Add(TEXT("HarshManeuver"));
	if (Flags & (int32)EHazardFlags::FallOff)        ActiveHazards.Add(TEXT("FallOff"));

	// 켜져 있는 모든 위험을 "|" 기호로 묶어서 반환합니다. (예: "Skid|LaneDeparture")
	return FString::Join(ActiveHazards, TEXT("|"));
}
