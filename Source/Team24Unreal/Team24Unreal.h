// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTeam24, Log, All);
// 프로젝트 전용 맞춤형 로그 카테고리(채널)를 선언합니다.
// 우리 팀이 작성한 메시지만 에디터의 '출력 로그(Output Log)' 창에서 쉽게 필터링해서 볼 수 있게 해줍니다.
// Team24: 우리가 쓸 로그 채널의 이름입니다.
// - Log: 기본적으로 출력할 메시지의 중요도(Verbosity)입니다.
// - All: 컴파일할 때 허용할 최대 중요도입니다.
