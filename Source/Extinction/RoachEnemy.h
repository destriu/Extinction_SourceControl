// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnemyBaseClass.h"
#include "RoachEnemy.generated.h"

/**
 * 
 */
UCLASS()
class EXTINCTION_API ARoachEnemy : public AEnemyBaseClass
{
	GENERATED_BODY()

public:
#pragma region Constructor
	ARoachEnemy();
#pragma endregion

#pragma region Blueprint Callable Functions
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void ScatterPointReached(FVector point);
#pragma endregion

#pragma region Properties
	UPROPERTY(BlueprintReadWrite, Category = "Roach Attributes")
		TArray<FVector> ScatterPoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		float ScatterRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		float ScatterMinimuimDistanceFromStartPosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		float ChargeMinimuimDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		float ChargeMaximuimDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		FVector ChargePosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		bool IsCharging;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		float SearchTimeLimit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		float MoveToTargetsLastKnownPositionTimeLimit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roach Attributes")
		int32 NumScatterPoints;
#pragma endregion
};






