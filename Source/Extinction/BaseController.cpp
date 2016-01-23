// Fill out your copyright notice in the Description page of Project Settings.

#include "Extinction.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine.h"
#include "DrawDebugHelpers.h"
#include "BaseController.h"
#include "EnemyBaseClass.h"

#pragma region Initializing
ABaseController::ABaseController(const FObjectInitializer& OI) : Super(OI.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	BBComp = OI.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoardComp"));
	BTreeComp = OI.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorTreeComp"));

	CFComp = Cast<UCrowdFollowingComponent>(this->GetPathFollowingComponent());
}

void ABaseController::Possess(APawn *Pawn)
{
	Super::Possess(Pawn);
	Self = Cast<AEnemyBaseClass>(Pawn);
	SetUseableStates();
	if (Self && Self->EnemyBehavior)
	{
		Self->AttackCompleted = true;
		Self->EState = EnemyState::ES_Wandering;
		UBlackboardData& data = *Self->EnemyBehavior->BlackboardAsset;
		BBComp->InitializeBlackboard(data);

		BTreeComp->StartTree(*Self->EnemyBehavior);

		SelfID = BBComp->GetKeyID("SelfActor");
		EStateID = BBComp->GetKeyID("State");
		ECStateID = BBComp->GetKeyID("CState");
		TargetKeyID = BBComp->GetKeyID("Target");
		AlertTargetID = BBComp->GetKeyID("AlertTarget");
		TargetLocationID = BBComp->GetKeyID("TargetLocation");
		DistanceFromTargetID = BBComp->GetKeyID("DistanceFromTarget");
		TargetsLastKnownPositionID = BBComp->GetKeyID("TargetLastKnownPosition");
		SearchPositionID = BBComp->GetKeyID("SearchPosition");
		MovedToLastKnownPositionID = BBComp->GetKeyID("MovedToLastKnownPosition");
		EnemyLocationID = BBComp->GetKeyID("EnemyLocation");
		AlertLocationID = BBComp->GetKeyID("AlertLocation");

		SetState(EnemyState::ES_Wandering, "Wandering");
		SetConditionState(EnemyConditionState::ECS_Normal, "Normal");
		BBComp->SetValue<UBlackboardKeyType_Object>(SelfID, Self);

		if (GetWorld())
			World = GetWorld();
	}
}

void ABaseController::SetUseableStates()
{
	EnemyState EStates[] = { EnemyState::ES_Attacking, EnemyState::ES_Chasing, EnemyState::ES_Idle, EnemyState::ES_Searching, EnemyState::ES_Wandering };
	EnemyConditionState ECStates[] = { EnemyConditionState::ECS_Normal, EnemyConditionState::ECS_Enraged };

	Self->UseableEStates.Append(EStates, ARRAY_COUNT(EStates));
	Self->UseableECStates.Append(ECStates, ARRAY_COUNT(ECStates));
}
#pragma endregion

#pragma region Tick
void ABaseController::Tick(float deltaTime)
{
	BBComp->SetValue<UBlackboardKeyType_Vector>(EnemyLocationID, Self->GetActorLocation());

	if (StartIdleTimer)
		IdleTimer(deltaTime);
	else
		TimeElaspedSinceEnemyWentIdle = 0;
}
#pragma endregion

#pragma region Set State
void ABaseController::SetState(EnemyState eState, FString sName)
{
	Self->EState = eState;
	BBComp->SetValue<UBlackboardKeyType_String>(EStateID, sName);

	if (eState == EnemyState::ES_Chasing)
		Self->GetCharacterMovement()->MaxWalkSpeed = Self->ChasingMovementSpeed;
	else if (eState == EnemyState::ES_Wandering)
		Self->GetCharacterMovement()->MaxWalkSpeed = Self->WanderingMovementSpeed;
	else if (eState == EnemyState::ES_Scattering)
		Self->GetCharacterMovement()->MaxWalkSpeed = Self->ScatteringMovementSpeed;
}

void ABaseController::SetConditionState(EnemyConditionState eCState, FString sName)
{
	Self->ECState = eCState;
	BBComp->SetValue<UBlackboardKeyType_String>(ECStateID, sName);
}
#pragma endregion

#pragma region Set/Reset Focus Actor
void ABaseController::SetFocusActor(AActor* target)
{
	//Self->CharacterMovement->bOrientRotationToMovement = false;
	//Self->CharacterMovement->bUseControllerDesiredRotation = true;
	//Self->bUseControllerRotationYaw = true;

	//this->SetFocalPoint();
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Target->GetName());
	//this->SetFocus(Target);
}

void ABaseController::ResetFocusActor()
{
	//Self->CharacterMovement->bUseControllerDesiredRotation = false;
	//Self->CharacterMovement->bOrientRotationToMovement = true;
	//Self->bUseControllerRotationYaw = false;
	//this->SetFocus(NULL);
}
#pragma endregion

#pragma region Idle
void ABaseController::GetIdleTimeLimit()
{
	Self->IdleTimeLimit = FMath::FRandRange(Self->IdleMinimuimTimeLimit, Self->IdleMaximuimTimeLimit);
}

void ABaseController::Idle()
{
	if (!StartIdleTimer)
	{
		StartIdleTimer = true;
		GetIdleTimeLimit();

		ResetFocusActor();
		SetState(EnemyState::ES_Idle, "Idle");
	}
}

void ABaseController::IdleTimer(float deltaTime)
{
	if (TimeElaspedSinceEnemyWentIdle < Self->IdleTimeLimit)
		TimeElaspedSinceEnemyWentIdle += deltaTime;
	else
	{
		StartIdleTimer = false;
		TimeElaspedSinceEnemyWentIdle = 0;

		ResetFocusActor();
		SetState(EnemyState::ES_Wandering, "Wandering");
	}
}
#pragma endregion

#pragma region Wander
FVector ABaseController::GetNewWanderPosition()
{
	FNavLocation Location;
	UNavigationSystem* NavSys = World->GetNavigationSystem();

	FHitResult hit(ForceInit);
	FCollisionQueryParams traceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, Self);
	traceParams.bTraceComplex = true;
	traceParams.bTraceAsyncScene = true;

	int times = 0;
	float minimuimDist = Self->MinimuimDistanceBetweenWanderLocations;

	do{
		if (times >= 50)
		{
			times = 0;
			minimuimDist -= 500.f;
		}

		times++;
		NavSys->GetRandomPointInRadius(Self->GetActorLocation(), Self->SightRange, Location);
	} while (FVector::Dist(Location.Location, Self->GetActorLocation()) < minimuimDist);

	bool bHit = World->LineTraceSingle(hit, Self->EyeLocation, Location.Location, ECC_Visibility, traceParams);

	return Location.Location;
}
#pragma endregion

#pragma region Alert Others
void ABaseController::AlertEnemiesInRange(ACharacter* target)
{
	for (FConstPawnIterator i = World->GetPawnIterator(); i; ++i)
	{
		AEnemyBaseClass* otherEnemy = Cast<AEnemyBaseClass>(*i);
		if (otherEnemy != NULL && otherEnemy != Self)
		{
			float distance = FVector::Dist(Self->GetActorLocation(), otherEnemy->GetActorLocation());
			if ((otherEnemy->EState == EnemyState::ES_Wandering || otherEnemy->EState == EnemyState::ES_Idle)
				&& Self->AlertRange >= distance)
			{
				FNavLocation location;
				World->GetNavigationSystem()->GetRandomPointInRadius(Self->GetActorLocation(), 2000.f, location);

				ABaseController* otherEnemyController = Cast<ABaseController>(otherEnemy->GetController());
				otherEnemyController->SetState(EnemyState::ES_Alerted, "Alerted");
				otherEnemyController->BBComp->SetValue<UBlackboardKeyType_Vector>(AlertLocationID, location.Location);
				otherEnemyController->BBComp->SetValue<UBlackboardKeyType_Object>(AlertTargetID, target);

				otherEnemy->GetCharacterMovement()->StopActiveMovement();
				World->GetNavigationSystem()->SimpleMoveToLocation(otherEnemy->GetController(), location.Location);
			}
		}
	}
}
#pragma endregion

#pragma region Search For Target
void ABaseController::SearchForTarget()
{
	if (GetPawn() == NULL || StopSearching)
	{
		return;
	}

	bool canSeePlayer = false;
	FHitResult hit(ForceInit);
	FCollisionQueryParams traceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, Self);
	traceParams.bTraceComplex = true;
	traceParams.bTraceAsyncScene = true;

	FVector EnemyLocation = Self->GetActorLocation();
	for (FConstPawnIterator i = World->GetPawnIterator(); i; ++i)
	{
		ACharacter* poesibleTarget = Cast<ACharacter>(*i);
		if (poesibleTarget != Cast<ACharacter>(Self) && poesibleTarget != NULL)
		{

			FVector possibleTargetLocation = poesibleTarget->GetActorLocation();
			bool bHit = World->LineTraceSingle(hit, Self->EyeLocation, possibleTargetLocation, ECC_Visibility, traceParams);

			bool bPersistent = true;
			float LifeTime = 5.f;

			// @fixme, draw line with thickneES = 2.f?
			if (bHit && hit.bBlockingHit)
			{
				// Red up to the blocking hit, green thereafter
				//DrawDebugLine(World, Self->EyeLocation, hit.ImpactPoint, FColor::Red, bPersistent, LifeTime);
				//DrawDebugLine(World, hit.ImpactPoint, possibleTargetLocation, FColor::Green, bPersistent, LifeTime);
				//DrawDebugPoint(World, hit.ImpactPoint, 16.f, FColor::Red, bPersistent, LifeTime);
			}
			else
			{
				// no hit means all red
				//DrawDebugLine(World, Self->EyeLocation, possibleTargetLocation, FLinearColor::Red, bPersistent, LifeTime);
			}

			if (hit.GetActor() && hit.GetActor()->GetName() == poesibleTarget->GetName() && hit.bBlockingHit)
			{
				if (Self->EState != EnemyState::ES_Searching)
					TargetsLastKnownPosition = hit.GetActor()->GetActorLocation();

				BBComp->SetValue<UBlackboardKeyType_Vector>(TargetsLastKnownPositionID, TargetsLastKnownPosition);
				float distanceFromPoESibleTarget = FVector::Dist(EnemyLocation, possibleTargetLocation);
				if (distanceFromPoESibleTarget <= Self->SightRange && distanceFromPoESibleTarget > Self->AttackRange && Self->AttackCompleted)
				{
					SetTarget(poesibleTarget, distanceFromPoESibleTarget);

					ResetFocusActor();
					SetState(EnemyState::ES_Chasing, "Chasing");

					BBComp->SetValue<UBlackboardKeyType_Bool>(MovedToLastKnownPositionID, false);
				}
				else if (distanceFromPoESibleTarget <= Self->AttackRange && Self->AttackCompleted)
				{
					Self->AttackStarted = true;
					Self->AttackCompleted = false;
					SetTarget(poesibleTarget, distanceFromPoESibleTarget);
					SetFocusActor(Cast<AActor>(Target));

					AttackLocation = poesibleTarget->GetActorLocation();

					SetState(EnemyState::ES_Attacking, "Attacking");

					BBComp->SetValue<UBlackboardKeyType_Bool>(MovedToLastKnownPositionID, false);
				}
				else if (Self->AttackCompleted)
				{
					ResetFocusActor();
					SetState(EnemyState::ES_Searching, "Searching");

					SearchForTargetAtLastKnownPosition();
				}
			}
			else if (hit.GetActor() && hit.GetActor()->GetName() != poesibleTarget->GetName() && hit.bBlockingHit)
			{
				if (Self->EState != EnemyState::ES_Searching)
					SearchForTargetAtLastKnownPosition();

				Self->AttackStarted = false;
				Self->AttackCompleted = true;

				ResetFocusActor();
				SetState(EnemyState::ES_Searching, "Searching");
			}
		}
	}
}

void ABaseController::SetTarget(APawn *Target, float distanceFromTarget)
{
	this->Target = Cast<ACharacter>(Target);
	BBComp->SetValue<UBlackboardKeyType_Object>(TargetKeyID, Target);
	BBComp->SetValue<UBlackboardKeyType_Float>(DistanceFromTargetID, distanceFromTarget);
	BBComp->SetValue<UBlackboardKeyType_Vector>(TargetLocationID, Target->GetActorLocation());
}
#pragma endregion

#pragma region Search For Target at last known position
FVector ABaseController::GetNewSearchPosition()
{
	FNavLocation Location;
	UNavigationSystem* NavSys = World->GetNavigationSystem();
	NavSys->GetRandomPointInRadius(TargetsLastKnownPosition, Self->SearchRange, Location);
	return Location.Location;
}

void ABaseController::SearchForTargetAtLastKnownPosition()
{
	SearchPosition = GetNewSearchPosition();
	BBComp->SetValue<UBlackboardKeyType_Vector>(SearchPositionID, SearchPosition);
}
#pragma endregion

#pragma region AttackTarget
FVector ABaseController::GetHitPosition()
{
	FNavLocation location;
	UNavigationSystem* navSys = UNavigationSystem::GetCurrent(World);
	navSys->GetRandomPointInRadius(AttackLocation, 200.f, location);
	return location.Location;
}

void ABaseController::AttackTarget()
{
	Self->AttackStarted = false;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Function AttackTarget hasn't been implemented!"));
}

void ABaseController::TargetAttacked()
{
	Self->AttackStarted = false;
	Self->AttackCompleted = true;
}
#pragma endregion




