// Copyright Team24. All Rights Reserved.

#include "DataLogger/AgentDataLogger.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
// 디버그 드로잉을 위한 헤더 추가
#include "DrawDebugHelpers.h"

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
