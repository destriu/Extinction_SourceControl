// Fill out your copyright notice in the Description page of Project Settings.

#include "Extinction.h"
#include "DrawDebugHelpers.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/CrowdManager.h"
#include "EnemyAIController.h"

// This holds all the functions that are used by TargetInfo
#pragma region TargetInfo Section
// I don't think anything is needed in this right now
FTInfo::FTInfo(){ Target = nullptr; }
void FTInfo::ClearContinousDamage(){ ContinousDamage = 0.f; }
void FTInfo::SetbAttackingAI(bool bAttackingAI){ this->bAttackingAI = bAttackingAI;  }

void FTInfo::SetTargetHealth(float health)
{
	if (health >= 0) 
	{ 
		Health = health; 
		if (MaxHealth <= 0)
			MaxHealth = Health;
	}
}

void FTInfo::AddDamageDone(float damage)
{
	if (damage > 0)
	{
		DamageDone += damage;
		ContinousDamage += damage;
	}
}

void FTInfo::Clear()
{
	Target = nullptr;
}

bool FTInfo::operator==(FTInfo t) const
{
	if (Target == t.Target)
		return true;

	return false;
}

bool FTInfo::operator==(int i) const
{
	if (Target == nullptr && i == NULL)
		return true;

	return false;
}

bool FTInfo::operator!=(int i) const
{
	if (Target != nullptr && i == NULL)
		return true;

	return false;
}

// These functions are in AEnemyAIController so the functionsinside each TartgetInfo can be accessed from the blueprint
#pragma region Bridge functions from AEnemyAI
FTInfo* AEnemyAIController::GetTarget(ACharacter* target)
{
	for (int i = 0; i < Targets.Num(); i++)
	{
		if (Targets[i].Target == target)
			return &Targets[i];
	}

	return nullptr;
}

void AEnemyAIController::SetAttackingAI(ACharacter* target, bool bAttackingAI)
{
	FTInfo* tInfo = GetTarget(target);
	if (tInfo != nullptr)
		tInfo->SetbAttackingAI(bAttackingAI);
}

void AEnemyAIController::SetTargetHealth(ACharacter* target, float health)
{
	FTInfo* tInfo = GetTarget(target);
	if (tInfo != nullptr)
		tInfo->SetTargetHealth(health);
}

void AEnemyAIController::AddDamageDone(ACharacter* target, float damage)
{
	FTInfo* tInfo = GetTarget(target);
	if (tInfo !=nullptr)
		tInfo->AddDamageDone(damage);
}

void AEnemyAIController::ClearContiousDamage(ACharacter* target)
{
	FTInfo* tInfo = GetTarget(target);
	if (tInfo != nullptr)
		tInfo->ClearContinousDamage();
}
#pragma endregion

#pragma endregion

#pragma region StateFunc and StateFuncDict Section

#pragma endregion

#pragma region Initializing
// This is used for initializing the crowdfollowingcomponent, 
// though it can't be used to initialize too much since some 
// things won't be avaliable at the time this goes through
AEnemyAIController::AEnemyAIController(const FObjectInitializer& OI) : 
Super(OI.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	BBComp = OI.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoardComp"));
	//BTreeComp = OI.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorTreeComp"));

	UCrowdFollowingComponent* crowdFollowingComponent = Cast<UCrowdFollowingComponent>(this->GetPathFollowingComponent());
	crowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
	crowdFollowingComponent->SetCrowdSeparationWeight(10.f);
	crowdFollowingComponent->SetCrowdSeparation(true);
	crowdFollowingComponent->SetCrowdCollisionQueryRange(2000.f);
	crowdFollowingComponent->SetCrowdPathOptimizationRange(2000.f);

	MaxHealth = Health;
}

// This will setup the blackboard keyids and do some other initializing if nessecary
void AEnemyAIController::Possess(APawn *Pawn)
{
	Super::Possess(Pawn);
	ControledCharacter = Cast<ACharacter>(Pawn);
	if (ControledCharacter && BBTree)
	{
		// adding state functions to TMap to make them easier to call later
		StateFuncs.Add(AEnemyState::Idle, &AEnemyAIController::Idle);
		StateFuncs.Add(AEnemyState::Wandering, &AEnemyAIController::Wandering);
		StateFuncs.Add(AEnemyState::Chasing, &AEnemyAIController::Chasing);
		StateFuncs.Add(AEnemyState::Attacking, &AEnemyAIController::Attacking);
		//StateFuncs.Add(AEnemyState::Alerted, &Alerted);
		//StateFuncs.Add(AEnemyState::Idle, &Idle);

		//BTreeComp->StartTree(*BBTree);
		UBlackboardData& data = *BBTree->BlackboardAsset;
		BBComp->InitializeBlackboard(data);

		EStateID = BBComp->GetKeyID("State");
		ECStateID = BBComp->GetKeyID("CState");
		TargetID = BBComp->GetKeyID("Target");
		AlertTargetID = BBComp->GetKeyID("AlertTarget");
		AlertLocationID = BBComp->GetKeyID("AlertLocation");
		AILocationID = BBComp->GetKeyID("AILocation");
		TargetLocationID = BBComp->GetKeyID("TargetLocation");
		DistanceFromTargetID = BBComp->GetKeyID("DistanceFromTarget");
		ControledCharacterID = BBComp->GetKeyID("ControledCharacter");
		TargetsLastKnownPositionID = BBComp->GetKeyID("TargetLastKnownPosition");
		
		SetState(AEnemyState::Wandering, AEnemyConditionState::Normal);

		if (GetWorld())
			World = GetWorld();
	}
}

// This function should only becalled once and will not set anything it BBComp already has a value
void AEnemyAIController::SetBlackBoardCompontent(UBlackboardComponent* bbComp)
{
	if (BBComp == nullptr && bbComp)
		BBComp = bbComp;
	else
		UE_LOG(LogTemp, Warning, TEXT("BBComp has already been set! Stop trying to set it!!!"));
}
#pragma endregion

// Functions in this region either need to be called every frame ex. Tick
#pragma region Every frame functions
// Makes calls every frame, so about 60 times in 1 second
void AEnemyAIController::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	FindTargets();// This will look for targets(Players)
	UpdateTargetInfos();// This will update info for every target in Targets before they are analyzed
	if (Targets.Num() > 0)
	{ 
		FTInfo* t = AnalyzeTargets();
		if (t->Target != NULL && AggroTimer <= 0.f)
			AggroTarget(t);
	}

	CheckTimers(deltaTime);
	UpdateBlackboardVars();

	// Check to see if the current state of the ai needs to be changed
	TPair<AEnemyState, AEnemyConditionState> t =SuggestState();
	if (t.Key != EState || t.Value != ECState)
		SetState(t.Key, t.Value);
}

void AEnemyAIController::CheckTimers(float deltaTime)
{
	switch (EState)
	{
	case AEnemyState::Idle:
		if (IdleTimer > 0.f)
			IdleTimer -= deltaTime;
		break;
	case AEnemyState::Alerted:
		break;
	case AEnemyState::Attacking:
		if (Target.Target && AttackDelayTimer > 0.f)
			AttackDelayTimer -= deltaTime;
	case AEnemyState::Chasing:
		if (Target.Target && AggroTimer > 0.f)
			AggroTimer -= deltaTime;
		break;
	case AEnemyState::Searching:
		break;
	case AEnemyState::Wandering:
		if (WanderingTimer > 0.f)
			WanderingTimer -= deltaTime;
		break;
	default:
		break;
	} 
}

void AEnemyAIController::UpdateBlackboardVars()
{
	if (BBComp)
	{
		BBComp->SetValue<UBlackboardKeyType_Object>(ControledCharacterID, ControledCharacter);
		BBComp->SetValue<UBlackboardKeyType_String>(EStateID, StateName);
		BBComp->SetValue<UBlackboardKeyType_Vector>(AILocationID, ControledCharacter->GetActorLocation());

		if (Target.Target)
		{
			BBComp->SetValue<UBlackboardKeyType_Object>(TargetID, Target.Target);
			BBComp->SetValue<UBlackboardKeyType_Vector>(TargetLocationID, Target.Location);
			BBComp->SetValue<UBlackboardKeyType_Float>(DistanceFromTargetID, Target.Distance);
		}
	}
}

#pragma region Helpers
// This should return wether the ai can see a target at all
bool AEnemyAIController::CanBeSeen(FTInfo* target)
{
	FHitResult hit = LineTraceToTarget(target->Location);
	if (hit.GetActor() && hit.GetActor()->GetName() == target->Target->GetName() && hit.bBlockingHit)
	{
		float distance = target->Distance;
		if (distance <= 0)// only do this if distance is less or is 0, since this function can be called from a spot where distance won't get set
		{
			distance = FVector::Dist(ControledCharacter->GetActorLocation(), target->Location);
		}

		return (distance < SightRange || distance < AlertRange);
	}

	return false;
}

// This will check the Targets array to make sure the character isn't already in it
bool AEnemyAIController::ContainsTarget(ACharacter* target)
{
	int numElements = Targets.Num();
	for (int i = 0; i < numElements; i++)
	{
		if (Targets[i].Target == target) { return true; }
	}

	return false;
}

// This function should do a line trace to a specified location. Turn Debug on for debug lines of the trace to be drawn
FHitResult AEnemyAIController::LineTraceToTarget(FVector targetLocation)
{
	// Setting up the line trace and hit
	FHitResult hit(ForceInit);
	FCollisionQueryParams traceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true);
	traceParams.bTraceComplex = true;
	traceParams.bTraceAsyncScene = true;

	bool bHit = World->LineTraceSingle(hit, EyeLocation, targetLocation, ECC_Visibility, traceParams);// Start a line trace towards the targets position
	if (Debug){ DrawLineToTarget(bHit, hit, targetLocation); }// Draws a line to the possible target

	return hit;
}

// This should only show up if Debug is on(True)
// This should draw a line to a target location, which should corrispond to a actual character being targeted by the ai. 
//There should be a red line if the target isn't in sight and a red line with a box at the target followed by a green line through the target if it is seen
void AEnemyAIController::DrawLineToTarget(bool bHit, FHitResult hit, FVector targetLocation)
{
	bool bPersistent = true;// determines wether or not the drawn debug line will stay after it's drawn once
	float lifeTime = 5.f;// determines how long the debug line will stay

	if (bHit && hit.bBlockingHit)
	{
		// Red up to the blocking hit, green thereafter
		DrawDebugLine(World, EyeLocation, hit.ImpactPoint, FColor::Red, bPersistent, lifeTime);
		DrawDebugLine(World, hit.ImpactPoint, targetLocation, FColor::Green, bPersistent, lifeTime);
		DrawDebugPoint(World, hit.ImpactPoint, 16.f, FColor::Red, bPersistent, lifeTime);
	}
	else
	{
		// no hit means all red
		DrawDebugLine(World, EyeLocation, targetLocation, FLinearColor::Red, bPersistent, lifeTime);
	}
}
#pragma endregion

void AEnemyAIController::FindTargets()
{
	if (GetPawn() == NULL){ return; }// If there is no pawn then we need to stop this
	for (FConstPawnIterator i = World->GetPawnIterator(); i; ++i)// Goes through every pawn in the world
	{
		ACharacter* possibleTarget = Cast<ACharacter>(*i);// Get the possible target pawn and cast it to a character so we can later check the controller
		if (Debug && possibleTarget != nullptr)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, GetDebugName(possibleTarget));
			//DrawDebugString(World, possibleTarget->GetActorLocation(), GetDebugName(possibleTarget));
		}
		// Check to make sure the possible target is the player or at least the companion
		if (possibleTarget != nullptr && (GetDebugName(possibleTarget).Contains(TEXT("Player")) || GetDebugName(possibleTarget).Contains(TEXT("Companion"))))
		{
			// Make a temp TargetInfo to house info of this possible target
			FTInfo* target = new FTInfo();
			target->Target = possibleTarget;
			target->Location = possibleTarget->GetActorLocation();

			// Only add the target to the list of known targets if it can be seen and it isn;t already in te list
			if (CanBeSeen(target) && !ContainsTarget(possibleTarget)){ Targets.AddUnique(*target); }
		}
	}
}

void AEnemyAIController::UpdateTargetInfos()
{
	int numElements = Targets.Num();
	for (int i = 0; i < numElements; i++)
	{
		FTInfo* target = &Targets[i];
		target->Location = target->Target->GetActorLocation();
		target->Distance = FVector::Dist(ControledCharacter->GetActorLocation(),target->Location);
		target->bInSight = CanBeSeen(target);

		if (Target == *target)
			Target = *target;
	}
}

void AEnemyAIController::ScoreTargets()
{
	float distScore = 0.f;
	float damageScore = 0.f;
	float healthScore = 0.f;

	for (int i = 0; i < Targets.Num(); i++)
	{
		ACharacter* target = Targets[i].Target;
		float dist = FVector::Dist(ControledCharacter->GetActorLocation(), target->GetActorLocation());
		distScore = FMath::RoundToFloat((dist / SightRange) * 100.f);

		float healthScore = FMath::RoundToFloat((Targets[i].Health / Targets[i].MaxHealth) * 100.f);
		float damageScore = FMath::RoundToFloat((Targets[i].DamageDone / MaxHealth) * 200.f);

		Targets[i].AggroScore = distScore + healthScore + damageScore;
	}
}

FTInfo* AEnemyAIController::AnalyzeTargets()
{
	ScoreTargets();
	FTInfo* t = nullptr;
	if (Targets.Num() == 1)
		return &Targets[0];

	for (int i = 0; i < Targets.Num(); i++)
	{
		for (int x = 0; x < Targets.Num(); x++)
		{
			if (Targets[i].AggroScore > Targets[x].AggroScore)
				t = &Targets[i];
		}
	}

	return t;
}

void AEnemyAIController::AggroTarget(FTInfo* target)
{
	AggroTimer = AggroTimeLimit;
	SetTarget(*target);
}

void AEnemyAIController::SetTarget(FTInfo target)
{
	Target = target;
}

void AEnemyAIController::ClearTargets()
{
	for (int i = 0; i < Targets.Num(); i++)
	{
		Targets[i].Clear();
		Targets.RemoveAt(i);
	}
}
#pragma endregion

// This section should take care of setting and clearing out the focus actor
// May add in things for focus point, not sure yet
#pragma region Focus Actor
// This will make sure the ai orients its rotation to whatever target it is going after
void AEnemyAIController::SetFocusActor(AActor* target)
{
	this->SetFocus(target);
}

// This will clear out the current focus actor if there is any
void AEnemyAIController::ResetFocusActor()
{
	if (this->GetFocusActor())
		this->ClearFocus(EAIFocusPriority::Gameplay);
}
#pragma endregion 

// This section should hold all the functions that setup or teardown for a state change
#pragma region State Functions
// This should find the state that the ai should be in or be going to
TPair<AEnemyState, AEnemyConditionState> AEnemyAIController::SuggestState()
{
	TPair<AEnemyState, AEnemyConditionState> t;
	t.Key = AEnemyState::InValid;
	if (Targets.Num() <= 0)
	{
		if (EState != AEnemyState::Idle && WanderingTimer <= 0.f)
		{
			t.Key = AEnemyState::Idle;
			t.Value = AEnemyConditionState::Normal;
		}
		else if (EState != AEnemyState::Wandering && IdleTimer <= 0.f)
		{
			t.Key = AEnemyState::Wandering;
			t.Value = AEnemyConditionState::Normal;
		}
	}
	else if (Target.Target)
	{
		if (EState != AEnemyState::Chasing && Target.Distance > AttackRange)
		{
			t.Key = AEnemyState::Chasing;
			t.Value = AEnemyConditionState::Normal;
		}
		else if (EState != AEnemyState::Attacking && Target.Distance <= AttackRange && AttackDelayTimer <= 0.f)
		{
			t.Key = AEnemyState::Attacking;
			t.Value = AEnemyConditionState::Normal;
		}
	}

	if (t.Key == AEnemyState::InValid)
	{
		t.Key = EState;
		t.Value = ECState;
	}

	return t;
}

FString AEnemyAIController::StateToString(AEnemyState state)
{
	switch (state)
	{
	case AEnemyState::Idle:
		return TEXT("Idle");
	case AEnemyState::Alerted:
		return TEXT("Alerted");
	case AEnemyState::Attacking:
		return TEXT("Attacking");
	case AEnemyState::Chasing:
		return TEXT("Chasing");
	case AEnemyState::Searching:
		return TEXT("Searching");
	case AEnemyState::Wandering:
		return TEXT("Wandering");
	default:
		return TEXT("");
	}
}

void AEnemyAIController::SetState(AEnemyState eState, AEnemyConditionState eCState)
{
	EState = eState;
	ECState = eCState;
	PreviousStateName = StateName;
	StateName = StateToString(EState);
	if (PreviousStateName == TEXT("Attacking"))
		AttackDelayTimer = 0.f;

	(this->* (StateFuncs[EState]))();
}

void AEnemyAIController::SetConditionState(AEnemyConditionState eCState)
{
	ECState = eCState;
}

// This function should setup everything needed to switch to the Idle state
void AEnemyAIController::Idle()
{
	ClearTargets();
	IdleTimer = FMath::FRandRange(IdleMinTimeLimit, IdleMaxTimeLimit);
}

void AEnemyAIController::Wandering()
{
	ClearTargets();
	WanderingTimer = FMath::FRandRange(WanderingMinTimeLimit, WanderingMaxTimeLimit);
}

void AEnemyAIController::Chasing()
{
	
}

void AEnemyAIController::Attacking()
{
	AttackStarted = true;
	SetFocusActor(Target.Target);
}

void AEnemyAIController::TargetAttacked()
{
	AttackStarted = false;
	AttackEnded = true;
	ResetFocusActor();
	if (EState == AEnemyState::Attacking)
		AttackDelayTimer = FMath::FRandRange(MinAttackDelay, MaxAttackDelay);
}
#pragma endregion