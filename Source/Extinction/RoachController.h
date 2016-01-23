// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BaseController.h"
#include "RoachEnemy.h"
#include "RoachController.generated.h"

/**
*
*/
#pragma region Defining Constants
#define ATTACKING  0
#define CHASING  1
#define IDLE  2
#define SEARCHING  3
#define WANDERING  4
#define SCATTERING  5
#define SEARCHING	6
#define ROARING 7
#define ALERTED 8
#define CHARGING 9

#define NORMAL 0
#define ENRAGED 1
#pragma endregion

UCLASS()
class EXTINCTION_API ARoachController : public ABaseController
{
	GENERATED_BODY()

public:
#pragma region Constructor
	ARoachController(const FObjectInitializer&);
#pragma endregion

#pragma region Code only functions
	virtual void SetUseableStates();

	void ChaseTimer(float deltaTime);

	void MoveToLastKnownLocationTimer(float deltaTime);

	void SearchTimer(float deltaTime);

	void CheckChargeRange();
#pragma endregion

#pragma region Blueprint Callable functions
	UFUNCTION(BlueprintCallable, Category = Behavior)
		FVector GetScatterLocation(FVector previousLocation);// Gets a new location within the navmesh for the roach(this) to scatter to

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void ScatterStarted();// Starts the rachs scatter process

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void ScatterCompleted();// Ends the roachs scatter process

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void ForceAttack();// Makes the roach attack or at least puts it in the attack state

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void ForceRoar(FString stateBeforeRoar, ACharacter* target);// Makes the roach go into the roaring state

	virtual void Tick(float deltaTime);// It's like a updatefuction

	virtual void TargetAttacked();// Starts the events that happen after a target is successfully attacked

	virtual void SearchForTarget();// Searchs for any possible targets in the roachs sight

	virtual void SearchForTargetAtLastKnownPosition();// 

	void CheckRoachsInRangeForTarget();

	void ResetVariablesForStateChange(int state);
#pragma endregion

#pragma region Properties
	UPROPERTY(BlueprintReadOnly, Category = "Roach Controller Properties")
		FString StateBeforeRoar;

	UPROPERTY(BlueprintReadWrite, Category = "Roach Controller Properties")
		int32 AttackAttempts;// Holds how many attempted attacks the roach(this) has made

	UPROPERTY(BlueprintReadOnly, Category = "Roach Controller Properties")
		bool LostTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Roach Controller Properties")
		bool CanCharge;

	UPROPERTY(BlueprintReadWrite, Category = "Roach Controller Properties")
		bool SearchingCompleted;// Holds whether searching was completed or not

	UPROPERTY(BlueprintReadWrite, Category = "Roach Controller Properties")
		bool ReachedTargetsLastKnownPosition;
#pragma endregion

#pragma region Roach Controller code only variables
	class ARoachEnemy*	Roach;// Holds a reference to the actor of the roach

	float				TimeElaspedSinceTargetWasLost;// Holds the time that has gone by since the roach(this) lost sight of it's target
	float				TimeElaspedSinceSearchingStarted;// Holds the time that has gone by since the roach(this) started searching
	float				TimeElaspedSinceMovingToTargetLastKnownPositionStarted;// Holds the time that has gone by since the roach started trying to move to the targets last known position

	bool				StartChaseTimer;// Holds whether the chase timer should be started or not
	bool				StartSearchTimer;// Holds whether the searching timer should be started or not
#pragma endregion
};

