// Copyright Epic Games, Inc. All Rights Reserved.

#include "Team24Unreal.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Team24Unreal, "Team24Unreal" );

// 로그 채널 생성
DEFINE_LOG_CATEGORY(LogTeam24)
// 예시로그 사용법
// UE_LOG(Team24, Log, TEXT("캐릭터가 점프했습니다!"));
// UE_LOG(Team24, Error, TEXT("데이터 로거를 찾을 수 없습니다!"));
