// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "BaseController.generated.h"

/**
*
*/
#pragma region Enemy States
UENUM(BlueprintType)
enum class EnemyState : uint8
{
	ES_Idle				UMETA(DisplayName = "Idle"),
	ES_Alerted			UMETA(DisplayName = "Alerted"),
	ES_Attacking		UMETA(DisplayName = "Attacking"),
	ES_Chasing			UMETA(DisplayName = "Chasing"),
	ES_Searching		UMETA(DisplayName = "Searching"),
	ES_Wandering		UMETA(DisplayName = "Wandering"),
	ES_Scattering		UMETA(DisplayName = "Scattering"),
	ES_Roaring          UMETA(DisplayName = "Roaring"),
	ES_Charging         UMETA(DisplayName = "Charging")
};

UENUM(BlueprintType)
enum class EnemyConditionState : uint8
{
	ECS_Normal			UMETA(DisplayName = "Normal"),
	ECS_Enraged			UMETA(DisplayName = "Enraged")
};
#pragma endregion

// Holds information about the target to figure out aggro if there is more than one target
USTRUCT(BlueprintType)
struct FTargetInfo
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(BlueprintReadWrite, Category = "Target Info")
		ACharacter* Target;// This will hold the actual target
	UPROPERTY(BlueprintReadWrite, Category = "Target Info")
		float Distance;// Holds the distance the target is from the ai
	UPROPERTY(BlueprintReadWrite, Category = "Target Info")
		float Health;// Holds the health of the target
};

UCLASS()
class EXTINCTION_API ABaseController : public AAIController
{
	GENERATED_BODY()

public:
#pragma region Constructor
	ABaseController(const FObjectInitializer&);
#pragma endregion

#pragma region Components
	UPROPERTY(transient)
	class UBlackboardComponent*		BBComp;

	UPROPERTY(transient)
	class UBehaviorTreeComponent*	BTreeComp;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
		UCrowdFollowingComponent*   CFComp;
#pragma endregion

#pragma region Code Only Functions
	virtual void Possess(class APawn *Self);

	virtual FVector GetNewSearchPosition();

	virtual void Tick(float deltaTime);

	virtual void SetFocusActor(AActor* target);

	virtual void ResetFocusActor();

	virtual void AlertEnemiesInRange(ACharacter* target);

	virtual void SetUseableStates();

	virtual void GetIdleTimeLimit();

	virtual void IdleTimer(float deltaTime);
#pragma endregion

#pragma region Functions Callable by Blueprint
	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual FVector GetNewWanderPosition();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void SearchForTarget();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void SearchForTargetAtLastKnownPosition();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void AttackTarget();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void TargetAttacked();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual FVector GetHitPosition();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void Idle();

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetState(EnemyState eState, FString sName);

	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetConditionState(EnemyConditionState eCState, FString sName);

	UFUNCTION(BlueprintCallable, Category = Targeting)
		void SetTarget(class APawn *Target, float distanceFromTarget);
#pragma endregion

protected:
#pragma region Black Board Key IDs
	FBlackboard::FKey SelfID;
	FBlackboard::FKey EStateID;
	FBlackboard::FKey ECStateID;

	FBlackboard::FKey TargetKeyID;
	FBlackboard::FKey TargetLocationID;
	FBlackboard::FKey DistanceFromTargetID;

	FBlackboard::FKey AlertTargetID;
	FBlackboard::FKey AlertLocationID;

	FBlackboard::FKey EnemyLocationID;

	FBlackboard::FKey TargetsLastKnownPositionID;
	FBlackboard::FKey MovedToLastKnownPositionID;
	FBlackboard::FKey SearchPositionID;
#pragma endregion

#pragma region Other Variables
	class UWorld*		    World;
	class AEnemyBaseClass*  Self;

	class ACharacter*		Target;
	FVector					SearchPosition;
	FVector					TargetsLastKnownPosition;

	FVector					AttackLocation;

	float					TimeElaspedSinceEnemyWentIdle;

	bool					StopSearching;
	bool					StartIdleTimer;
#pragma endregion

};

