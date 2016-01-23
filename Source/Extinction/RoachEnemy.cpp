// Fill out your copyright notice in the Description page of Project Settings.

#include "Extinction.h"
#include "RoachController.h"
#include "RoachEnemy.h"


#pragma region Initializing
ARoachEnemy::ARoachEnemy()
{
	AIControllerClass = ARoachController::StaticClass();
}
#pragma endregion

#pragma region Scatter Stuff
void ARoachEnemy::ScatterPointReached(FVector point)
{
	ScatterPoints.Remove(point);
}
#pragma endregion
