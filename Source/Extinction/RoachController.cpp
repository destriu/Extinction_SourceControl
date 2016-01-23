// Fill out your copyright notice in the Description page of Project Settings.

#include "Extinction.h"
#include "RoachEnemy.h"
#include "Engine.h"
#include "DrawDebugHelpers.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/CrowdManager.h"
#include "RoachController.h"

#pragma region Initializing
ARoachController::ARoachController(const FObjectInitializer& OI) : Super(OI)
{
	AttackAttempts = 0;
	UCrowdFollowingComponent* crowdFollowingComponent = Cast<UCrowdFollowingComponent>(this->GetPathFollowingComponent());
	crowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
	crowdFollowingComponent->SetCrowdSeparationWeight(10.f);
	crowdFollowingComponent->SetCrowdSeparation(true);
	crowdFollowingComponent->SetCrowdCollisionQueryRange(2000.f);
	crowdFollowingComponent->SetCrowdPathOptimizationRange(2000.f);
}

void ARoachController::SetUseableStates()
{
	// Set which EnemyStates the roach(this) can use
	EnemyState EStates[] = { EnemyState::ES_Attacking, EnemyState::ES_Chasing, EnemyState::ES_Idle, EnemyState::ES_Searching, EnemyState::ES_Wandering, EnemyState::ES_Scattering, EnemyState::ES_Searching, EnemyState::ES_Roaring, EnemyState::ES_Alerted, EnemyState::ES_Charging };
	EnemyConditionState ECStates[] = { EnemyConditionState::ECS_Normal, EnemyConditionState::ECS_Enraged };

	Self->UseableEStates.Append(EStates, ARRAY_COUNT(EStates));
	Self->UseableECStates.Append(ECStates, ARRAY_COUNT(ECStates));

	// Get a self reference of the roach(this)
	Roach = Cast<ARoachEnemy>(Self);

	// Set the current state of the roach(this)
	SetState(Roach->UseableEStates[WANDERING], "Wandering");
}
#pragma endregion

#pragma region Check roachs close by for target
void ARoachController::CheckRoachsInRangeForTarget()
{
	for (FConstPawnIterator i = World->GetPawnIterator(); i; ++i)
	{
		AEnemyBaseClass* otherEnemy = Cast<AEnemyBaseClass>(*i);
		if (otherEnemy != NULL && otherEnemy != Self)
		{
			float distance = FVector::Dist(Self->GetActorLocation(), otherEnemy->GetActorLocation());
			if (Self->AlertRange >= distance)
			{
				TimeElaspedSinceSearchingStarted = 0;
				ReachedTargetsLastKnownPosition = false;
				StartSearchTimer = false;
				LostTarget = false;

				ABaseController* otherEnemyController = Cast<ABaseController>(otherEnemy->GetController());
				SetTarget(Cast<APawn>(BBComp->GetValue<UBlackboardKeyType_Object>(TargetKeyID)), distance);

				SetState(Roach->UseableEStates[CHASING], TEXT("Chasing"));
			}
		}
	}
}
#pragma endregion

#pragma region Tick
void ARoachController::Tick(float deltaTime)
{
	// Update the blackboard with the location of the roach(this)
	BBComp->SetValue<UBlackboardKeyType_Vector>(EnemyLocationID, Roach->GetActorLocation());

	CheckChargeRange();

	// Time how long it has been since the roach(this) has lost sight of the target
	ChaseTimer(deltaTime);
	// Time how long it has been sinch the roach(this) has started searching
	SearchTimer(deltaTime);

	MoveToLastKnownLocationTimer(deltaTime);

	// Starts the timer for how long the roach(this) will be idle
	if (StartIdleTimer)
		IdleTimer(deltaTime);
	else
		TimeElaspedSinceEnemyWentIdle = 0;
}

void ARoachController::ChaseTimer(float deltaTime)
{
	if (StartChaseTimer)
	{
		TimeElaspedSinceTargetWasLost += deltaTime;
		LostTarget = (TimeElaspedSinceTargetWasLost >= Roach->TimeUnseenBeforeAttentionSpaneRelapse);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::SanitizeFloat(TimeElaspedSinceTargetWasLost));

		if (LostTarget)
		{
			ResetFocusActor();
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target lost"));
			TimeElaspedSinceTargetWasLost = 0;
			StartChaseTimer = false;
		}
	}
	else
		TimeElaspedSinceTargetWasLost = 0;
}

void ARoachController::MoveToLastKnownLocationTimer(float deltaTime)
{
	if (StartSearchTimer && !ReachedTargetsLastKnownPosition)
	{
		TimeElaspedSinceMovingToTargetLastKnownPositionStarted += deltaTime;

		if (TimeElaspedSinceMovingToTargetLastKnownPositionStarted >= Roach->MoveToTargetsLastKnownPositionTimeLimit)
		{
			ReachedTargetsLastKnownPosition = true;
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Unable to reach targets last known location"));
		}
	}
	else
		TimeElaspedSinceMovingToTargetLastKnownPositionStarted = 0;
}

void ARoachController::SearchTimer(float deltaTime)
{
	if (StartSearchTimer && ReachedTargetsLastKnownPosition)
	{
		TimeElaspedSinceSearchingStarted += deltaTime;
		SearchingCompleted = (TimeElaspedSinceSearchingStarted >= Roach->SearchTimeLimit);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::SanitizeFloat(TimeElaspedSinceSearchingStarted));

		if (SearchingCompleted)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Done searching"));
			TimeElaspedSinceSearchingStarted = 0;
			ReachedTargetsLastKnownPosition = false;
			StartSearchTimer = false;
			LostTarget = false;

			
			this->GetPathFollowingComponent()->AbortMove(TEXT("Roach is roaring"));

			SetState(Roach->UseableEStates[ROARING], "Roaring");
			SetConditionState(Roach->UseableECStates[NORMAL], "Normal");

			StateBeforeRoar = "Searching";
		}
	}
	else
		TimeElaspedSinceSearchingStarted = 0;
}
#pragma endregion

#pragma region Search For Target
void ARoachController::ResetVariablesForStateChange(int state)
{
	if (state == ATTACKING)
	{
		StartIdleTimer = false;

		LostTarget = false;
		StartChaseTimer = false;

		StartSearchTimer = false;

		Roach->AttackStarted = true;
		Roach->AttackHitTarget = false;
		Roach->AttackCompleted = false;
	}
	else if (state == CHASING)
	{
		StartIdleTimer = false;

		LostTarget = false;
		StartChaseTimer = false;

		StartSearchTimer = false;
	}
}

void ARoachController::SearchForTarget()
{
	if (GetPawn() == NULL || StopSearching)
	{
		return;
	}

	bool canSeePlayer = false;
	FHitResult hit(ForceInit);
	FCollisionQueryParams traceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true);
	traceParams.bTraceComplex = true;
	traceParams.bTraceAsyncScene = true;

	FVector RoachLocation = Roach->GetActorLocation();
	for (FConstPawnIterator i = World->GetPawnIterator(); i; ++i)
	{
		ARoachEnemy* otherRoach = Cast<ARoachEnemy>(*i);
		ACharacter* possibleTarget = Cast<ACharacter>(*i);
		// Check ti make sure the possible target is actually a character and not null either
		if (possibleTarget != Cast<ACharacter>(Roach) && otherRoach == NULL && possibleTarget != NULL)
		{
			FVector possibleTargetLocation = possibleTarget->GetActorLocation();// Get location of the possible target
			bool bHit = World->LineTraceSingle(hit, Roach->EyeLocation, possibleTargetLocation, ECC_Visibility, traceParams);// Start a line trace towards the targets position

			bool bPersistent = true;
			float LifeTime = 5.f;

			// @fixme, draw line with thickneES = 2.f?
			if (bHit && hit.bBlockingHit)
			{
				// Red up to the blocking hit, green thereafter
				//DrawDebugLine(World, Roach->EyeLocation, hit.ImpactPoint, FColor::Red, bPersistent, LifeTime);
				//DrawDebugLine(World, hit.ImpactPoint, possibleTargetLocation, FColor::Green, bPersistent, LifeTime);
				//DrawDebugPoint(World, hit.ImpactPoint, 16.f, FColor::Red, bPersistent, LifeTime);
			}
			else
			{
				// no hit means all red
				DrawDebugLine(World, Roach->EyeLocation, possibleTargetLocation, FLinearColor::Red, bPersistent, LifeTime);
			}

			// Check if the line trace hit a actor and if that actor is the target
			if (hit.GetActor() && hit.GetActor()->GetName() == possibleTarget->GetName() && hit.bBlockingHit)
			{
				// Get the distance from the target and the roach(this)
				float distanceFromPossibleTarget = FVector::Dist(RoachLocation, possibleTargetLocation);
				// Checks to see if the target is in the roach(this) sight, not close enough to attack, and that the roach(this) isn't finishing a attack on the target
				if (distanceFromPossibleTarget <= Roach->SightRange && distanceFromPossibleTarget > Self->AttackRange && Roach->AttackCompleted && !Roach->AttackHitTarget)
				{
					ResetVariablesForStateChange(CHASING);

					// Set the position the target was last seen in TargetsLastKnownPosition
					if (Target != NULL)
					{
						TargetsLastKnownPosition = Target->GetActorLocation();
						BBComp->SetValue<UBlackboardKeyType_Vector>(TargetsLastKnownPositionID, TargetsLastKnownPosition);
					}

					// Set the target to this one
					SetTarget(possibleTarget, distanceFromPossibleTarget);
					// Checks to see if the roach is wandering and if it is the roach(this) will roar since it just saw a target
					if (Roach->EState == Roach->UseableEStates[WANDERING] || Roach->EState == Roach->UseableEStates[IDLE] || Roach->EState == Roach->UseableEStates[ALERTED])
					{
						this->GetPathFollowingComponent()->AbortMove(TEXT("Roach is roaring"));
						Roach->JustSpottedTarget = true;

						if (Roach->EState != Roach->UseableEStates[ALERTED])
							AlertEnemiesInRange(possibleTarget);

						SetState(Roach->UseableEStates[ROARING], "Roaring");

						StateBeforeRoar = "WanderingC";
					}
					// Checks to see if the roach(this) is doing anything other than chasing the target and that it has stopped roaring, so it can begin chasing the target
					else if (Roach->EState != Roach->UseableEStates[CHASING] && Roach->EState != Roach->UseableEStates[CHARGING] && Target != NULL && !Roach->JustSpottedTarget)
					{
						SetState(Roach->UseableEStates[CHASING], "Chasing");
					}
				}
				// Checks to see if the target is close enough to attack and that the roach(this) isn't attacking, roraring or, just completed a attack
				else if (distanceFromPossibleTarget <= Roach->AttackRange && Roach->AttackCompleted && !Roach->AttackHitTarget && !Roach->JustSpottedTarget && !Roach->IsCharging)
				{
					ResetVariablesForStateChange(ATTACKING);
					SetTarget(possibleTarget, distanceFromPossibleTarget);
					// Checks to see if the roach is wandering and if it is the roach(this) will roar since it just saw a target
					if (Roach->EState == Roach->UseableEStates[WANDERING] || Roach->EState == Roach->UseableEStates[IDLE])
					{
						this->GetPathFollowingComponent()->AbortMove(TEXT("Roach is roaring"));
						Roach->JustSpottedTarget = true;

						SetState(Roach->UseableEStates[ROARING], "Roaring");

						StateBeforeRoar = "WanderingA";
					}
					// Checks to see if the roach(this) is doing anything other than attacking, besides roaring, so it can begin a attack
					else if (Roach->EState != Roach->UseableEStates[ATTACKING])
					{
						//SetFocusActor(Cast<AActor>(Target));
						SetState(Roach->UseableEStates[ATTACKING], "Attacking");
					}
				}
			}
			// Checks to see if the line trace hit a actor and if the actor hit is the same actor as the target
			else if (hit.GetActor() && hit.GetActor()->GetName() != possibleTarget->GetName() && hit.bBlockingHit)
			{
				// Checks if the roach is chasing a target and starts the chase timer since it can't see the target now
				if (Roach->EState == Roach->UseableEStates[CHASING])
				{
					StartChaseTimer = true;
					//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Lost sight of taregt"));
				}

				// Check if the roach(this) has completely lost the current target
				if (LostTarget)
				{
					AttackAttempts = 0;
					Roach->AttackStarted = false;
					Roach->AttackHitTarget = false;
					Roach->AttackCompleted = true;

					// Checks to see if there are any roachs close by with a target. 
					// The goal is to not have a roach lose sight of the player if they are in a group
					// It just wouldn't make sense if they did
					CheckRoachsInRangeForTarget();

					// Checks to see if the roach is roaring and if not changes the state so it can roar
					if (Roach->EState != Roach->UseableEStates[SEARCHING])
					{
						ResetFocusActor();
						SearchForTargetAtLastKnownPosition();
					}
				}
			}
		}
	}
}

void ARoachController::SearchForTargetAtLastKnownPosition()
{
	if (Roach->EState != Roach->UseableEStates[SEARCHING])
	{
		StartSearchTimer = true;

		ResetFocusActor();
		SetState(Roach->UseableEStates[SEARCHING], "Searching");
	}
}
#pragma endregion

#pragma region Force Actions
void ARoachController::ForceAttack()
{
	ResetVariablesForStateChange(ATTACKING);
	SetState(Roach->UseableEStates[ATTACKING], "Attacking");
}

void ARoachController::ForceRoar(FString stateBeforeRoar, ACharacter* target)
{
	this->GetPathFollowingComponent()->AbortMove(TEXT("Roach is roaring"));
	Roach->JustSpottedTarget = true;

	if (Roach->EState != Roach->UseableEStates[ALERTED])
		AlertEnemiesInRange(target);

	SetState(Roach->UseableEStates[ROARING], "Roaring");

	StateBeforeRoar = stateBeforeRoar;
}
#pragma endregion

#pragma region Attack Stuff
void ARoachController::CheckChargeRange()
{
	if (Target != NULL)
	{
		float distFromTarget = FVector::Dist(Roach->GetActorLocation(), Target->GetActorLocation());
		CanCharge = distFromTarget > Roach->ChargeMinimuimDistance && distFromTarget < Roach->ChargeMaximuimDistance;
	}
}
 
void ARoachController::TargetAttacked()
{
	Roach->AttackStarted = false;
	Roach->AttackCompleted = true;

	// Check to see if the roach hot its target 
	if (Roach->AttackHitTarget)
	{
		ResetFocusActor();

		AttackAttempts = 0;

		ResetFocusActor();
		SetState(Roach->UseableEStates[SCATTERING], "Scattering");
		SetConditionState(Roach->UseableECStates[NORMAL], "Normal");

		Roach->AttackHitTarget = false;
		StopSearching = true;

		ScatterStarted();
	}
	else
	{
		AttackAttempts++;// Increment the number of attempted attacks done by the roach
		// Check to see if the number of attempted attacks has reached the limit. If it has make the roach roar other wise have it keep chasing the target
		if (AttackAttempts == Roach->NumAttackAttempts)
		{
			ResetFocusActor();
			this->GetPathFollowingComponent()->AbortMove(TEXT("Roach is roaring"));
			Roach->JustSpottedTarget = true;

			SetState(Roach->UseableEStates[ROARING], "Roaring");
			SetConditionState(Roach->UseableECStates[ENRAGED], "Enraged");

			StateBeforeRoar = "Attacking";
		}
		else
		{
			ResetFocusActor();
			SetState(Roach->UseableEStates[CHASING], "Chasing");
		}
	}
}
#pragma endregion

#pragma region Scatter Stuff
FVector ARoachController::GetScatterLocation(FVector previousLocation)
{
	FNavLocation Location;
	UNavigationSystem* NavSys = World->GetNavigationSystem();

	int times = 0;
	float scatterMinimuimDist = Roach->ScatterMinimuimDistanceFromStartPosition;

	do{
		if (times > 50)
		{
			times = 0;
			scatterMinimuimDist -= 500.f;
		}

		times++;
		NavSys->GetRandomPointInRadius(previousLocation, Roach->ScatterRange, Location);
	} while (FVector::Dist(Location.Location, previousLocation) < scatterMinimuimDist);

	return Location.Location;
}

void ARoachController::ScatterStarted()
{
	for (int i = 0; i < Roach->NumScatterPoints; i++)
	{
		StopSearching = true;

		if (i == 0)
			Roach->ScatterPoints.Add(GetScatterLocation(Roach->GetActorLocation()));
		else
			Roach->ScatterPoints.Add(GetScatterLocation(Roach->ScatterPoints.Last()));
	}
}

void ARoachController::ScatterCompleted()
{
	Roach->ScatterPoints.Empty();
	StopSearching = false;

	ResetFocusActor();
	SetState(Roach->UseableEStates[WANDERING], "Chasing");
	SetConditionState(Roach->UseableECStates[NORMAL], "Normal");
}
#pragma endregion