// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "BaseController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "EnemyBaseClass.generated.h"

UCLASS()
class EXTINCTION_API AEnemyBaseClass : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyBaseClass();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	
#pragma region Base Enemy Attributes
	UPROPERTY(BlueprintReadWrite, Category = "Sight")
		FVector EyeLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sight")
		bool JustSpottedTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy State")
		EnemyState EState;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy State")
		EnemyConditionState ECState;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy State")
		TArray<EnemyState> UseableEStates;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy State")
		TArray<EnemyConditionState> UseableECStates;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float EnemyHealth;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float MinimuimDistanceBetweenWanderLocations;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AlertRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float SearchRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float SightRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float TimeUnseenBeforeAttentionSpaneRelapse;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float IdleMinimuimTimeLimit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float IdleMaximuimTimeLimit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float WanderingMovementSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float ChasingMovementSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float ScatteringMovementSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float EnragedMovementSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		float DelayBetweenAttacks;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attack Attributes")
		float AttackRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		float AttackDamage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		float ForceAppliedOnHit;

	UPROPERTY(EditAnywhere, Category = "Attack Attributes")
		int32 NumAttackAttempts;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		int32 NumMultiHitsInAttack;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		bool MultiHitAttack;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		bool AttackStarted;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		bool AttackHitTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attack Attributes")
		bool AttackCompleted;

	UPROPERTY(EditAnywhere, Category = "Behavior")
	class UBehaviorTree*		EnemyBehavior;
#pragma endregion

#pragma region Code only variables
	float			IdleTimeLimit;
#pragma endregion
};
