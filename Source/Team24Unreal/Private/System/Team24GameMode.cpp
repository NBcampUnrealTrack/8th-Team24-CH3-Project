// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Team24GameMode.h"
#include "System/Team24PlayerController.h"

ATeam24GameMode::ATeam24GameMode()
{
	PlayerControllerClass = ATeam24PlayerController::StaticClass();
}
