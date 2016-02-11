// Fill out your copyright notice in the Description page of Project Settings.

#ifndef MARKER
#define MARKER
#endif

#pragma once

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "EnemyAIController.generated.h"

/**
 * 
 */
#pragma region Enemy States
// Holds all the different states a enemy can be basic enemy
UENUM(BlueprintType)
enum class AEnemyState : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Alerted			UMETA(DisplayName = "Alerted"),
	Attacking		UMETA(DisplayName = "Attacking"),
	Chasing			UMETA(DisplayName = "Chasing"),
	Searching		UMETA(DisplayName = "Searching"),
	Wandering		UMETA(DisplayName = "Wandering"),
	InValid			UMETA(DisplayName = "InValid")
};

inline	uint8 GetTypeHash(const AEnemyState es){ return (uint8)es; }

// Holds all the condition states a basic enemy can be in
UENUM(BlueprintType)
enum class AEnemyConditionState : uint8
{
	Normal			UMETA(DisplayName = "Normal"),
	Enraged			UMETA(DisplayName = "Enraged"),
	InValid			UMETA(DisplayName = "InValid")
};
#pragma endregion

#pragma region Target Info
// Holds information about the target to figure out aggro if there is more than one target
USTRUCT(BlueprintType)
struct FTInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		 ACharacter* Target;// This will hold the actual target
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		FVector Location;// Holds the current location of the target
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		float Distance;// Holds the distance the target is from the ai
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		float ContinousDamage;// Holds how much continous damage the target has done to the ai
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		float DamageDone;// Holds how much damage the target has done to the ai
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		float MaxHealth;// Holds the health of the target
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		float Health;// Holds the health of the target
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		int32 AttackAttempts;// Holds how many times the ai has tried to attack this target and missed
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		bool bAttackingAI;// Holds wether this target is attacking this specific ai
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		bool bInSight;// Holds wether this target is in the ai's sight
	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
		float AggroScore;// Holds what this current targets aggro score is

	FTInfo();// Constructor for class
	void Clear();// This will clear out everything in this TargetInfo

	//inline void operator=(FTInfo* t) const;
	inline bool operator==(FTInfo t) const;
	inline bool operator==(int i) const;
	inline bool operator!=(int i) const;

	void SetbAttackingAI(bool bAttackingAI);//This functions is for setting wether the ai is being attacked by the target. This is going to have to be set in the blueprint, I just don't want to access the variable directly
	void SetTargetHealth(float health);// This function will set the health of the player. I need this to be set from the blueprint becuase I'm sure we aren't using the default health variable in character
	void AddDamageDone(float damage);// Since I don't want to access the variable directly to set anything, this function will add to DamageDone
	void ClearContinousDamage();//  I'll have a ContinousDamage timer in the blueprint, if it goes over this function will be called to clear out ContinousDamageDone
};
#pragma endregion

UCLASS()
class EXTINCTION_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AEnemyAIController(const FObjectInitializer&);// Constructor for the class

#pragma region Components
	UPROPERTY(transient)
		UBlackboardComponent*		BBComp; // Holds the blackboard or the brain of the ai. This should hold any data that needs to be kept for the behaviour tree
	//UPROPERTY(transient)
		//UBehaviorTreeComponent*		BTreeComp; // Controls certain enemy ai behaviour. It figures out what the ai should be doing sort of
	UPROPERTY(transient)
		UCrowdFollowingComponent*	CFComp; // Controls how the enemy will move
#pragma endregion

	//These functions should never be used inside the blueprint!!!!
#pragma region Code Only Functions
	virtual void Possess(class APawn *Self);
	virtual void Tick(float deltaTime) override;
	virtual void SetFocusActor(AActor* target);
	virtual void ResetFocusActor();
	virtual void CheckTimers(float deltaTime);// This will check each timer and decrease the amount of time left on it if it is active
	virtual void FindTargets();// This function will find any targets in the sight range and alert range of the ai
	virtual void ScoreTargets();// This functino will give each target in Targets a aggro score
	virtual FTInfo* AnalyzeTargets();// This function will figure out which target should be aggroed
	virtual void AggroTarget(FTInfo* target);// This function will actualy set the target return from AbalyzeTargets to be the actual target and do anyother setup needed
	virtual TPair<AEnemyState, AEnemyConditionState> SuggestState();// This function will suggest what state th ai should be in
	void DrawLineToTarget(bool bHit, FHitResult hit, FVector targetLocation);// This will draw a debug line to a target location specified by targetLocation. This is mostly for debugging purposes
	bool ContainsTarget(ACharacter* target);// Checks to see if the target is already contained in the array Targets
	void ClearTargets();// This will get rid of any targets being held in Targets
	void UpdateTargetInfos();// This will update information in each target in Targets that can be updated in the code alone
	void UpdateBlackboardVars();// This function is just for sending data to the blackboard
	bool CanBeSeen(FTInfo* target);// Returns wether or not a target in Targets can be seen
	FString StateToString(AEnemyState state);// This will return the string repersentation of the state sent in
	FTInfo* GetTarget(ACharacter* target);// This returns a TargetInfo that contains the character sent in if any do else returns null
	FHitResult LineTraceToTarget(FVector targetLocation);// This will start a line trace to the location given
#pragma endregion
	
	// These functions can be used inside the blueprint
#pragma region Functions Callable by Blueprint 
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetAttackingAI(ACharacter* target, bool bAttackingAI);// This function is just for accessing the function SetAttackingAI on the target sent in
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetTargetHealth(ACharacter* target, float health);// This function is just for accessing the function SetTargetHealth on the target sent in
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void AddDamageDone(ACharacter* target, float damage);// This function is just for accessing the function AddDamageDone on the target sent in
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void ClearContiousDamage(ACharacter* target);// This function is just for accessing the function ClearContiousDamage on the target sent in

	//UFUNCTION(BlueprintCallable, Category = Behavior)
		//virtual void SearchForTarget();// This will change the ai's state to SEARCHING and make the ai serach for it's current target without moving to the target's last known location (Should only be used if there are no more targets and the current target is out of sight)
	//UFUNCTION(BlueprintCallable, Category = Behavior)
		//virtual void SearchForTargetAtLastKnownPosition();// This will change the ai's state to SEARCHING and make the ai serach for it's current target at the last known position of it (Should only be used if the ai has lost sight of it's target and there are no other targets)
	//UFUNCTION(BlueprintCallable, Category = Behavior)
		//virtual void AttackTarget();// This function will change the ai's state to ATTACKING and start the attacking process
	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void TargetAttacked();// This function should be called after the ai has finished attacking
	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void Idle();// This will change the ai's state to IDLE
	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void Wandering();// This will change the ai's state to WANDERING
	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void Chasing();// This will change the ai's state to CHASING
	UFUNCTION(BlueprintCallable, Category = Behavior)
		virtual void Attacking();// This will change the ai's state to ATTACKING
	//UFUNCTION(BlueprintCallable, Category = Behavior)
		//virtual void StopSearh(bool foundTarget);// This will change the ai's state to IDLE/WANDERING or CHASING/ROARING if foundTarget is true
	//UFUNCTION(BlueprintCallable, Category = Behavior)
		//virtual void AlertEnemiesInRange(ACharacter* target); // This should alert anny enemies of the same type (or maybe any type) that are in range
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetState(AEnemyState eState, AEnemyConditionState eCState);// This should be used if there is a reason to manually set the ai's enemy state
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetConditionState(AEnemyConditionState eCState);// This should be used if there is a reason to manually set the ai's condition state
	UFUNCTION(BlueprintCallable, Category = Targeting)
		void SetTarget(FTInfo target); // This should be used if there is a reason to manually set the ai's target
	UFUNCTION(BlueprintCallable, Category = Behavior)
		void SetBlackBoardCompontent(UBlackboardComponent* BComp);// This function will set the variable BBComp with the blackboard component of the behaviortree set to the ControledCharacter. This should only be called once
#pragma endregion
	
protected:
	// These are only used in code and are only for accessing blackboard variables
#pragma region Black Board Key IDs
	FBlackboard::FKey ControledCharacterID;// This blackboard key is for the variable ControledCharacter, which will hold the character being controlled by this controller
	FBlackboard::FKey EStateID;// This blackboard key is for the variable EState, which will holds the Enemy State of this ai
	FBlackboard::FKey ECStateID;// This blackboard key is for the variable ECState, which will holds the Condition State of this ai
	FBlackboard::FKey TargetID;// This blackboard key is for the variable Target, which will hold the current target, if any, of this ai
	FBlackboard::FKey TargetLocationID;// This blackboard key is for the vaiable TargetLocation, which will hold the location of the current target if there is a target
	FBlackboard::FKey DistanceFromTargetID;// This blackboard key is for the variable DistanceFromTarget, which will hold the the distance from the ai to it's current target if there is one
	FBlackboard::FKey AlertTargetID;// This blackboard key is for the variable AlertTarget, hich will hold a possible target if it has alerted the ai
	FBlackboard::FKey AlertLocationID;// This blackboard key is for the variable AlertLocation, which will hold the location of a possible target when it alerted the ai
	FBlackboard::FKey AILocationID;// This blackboard key is for the variable AILocation, which will hold the location of this ai
	FBlackboard::FKey TargetsLastKnownPositionID;// This blackboard key is for the variable TargetsLastKnownPosition, which will hold the last position the ai saw it's current target at
#pragma endregion

public:
	// These variables can be viewed or edited from blueprints
#pragma region Blueprint Accessable Variables
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
		bool Debug;// This variables holds wether or not debug information will be displayed, like line traces
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		FVector EyeLocation;// This holds the location of the eyes of the ai. This will be used for line tracing
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float IdleMaxTimeLimit;// This holds the maximuim amount of time the ai can be idle
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float IdleMinTimeLimit;// This holds the minimuim amount of time the ai can be idle
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float WanderingMaxTimeLimit;// This holds the maximuim amount of time the ai can wander
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float WanderingMinTimeLimit;// This holds the minimuim amount of time the ai can wander
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float SearchMaxTimeLimit;// This holds the maximuim amount of time the ai can search
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float SearchMinTimeLimit;// This holds the minimuim amount of time the ai can search
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float MaxAttackDelay;// This holds the maximuim delay that will occur between attacks, if the ai is in the Attacking state long enough
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float MinAttackDelay;// This holds the minimuim delay that will occur between attacks, if the ai is in the Attacking state long enough
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AggroTimeLimit;// This holds how long the ai will be aggroed to one target before trying to switch
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float SearchRange;// This holds the range the ai can search in
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float SightRange;// This holds the range in which the ai can completely see a target
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AggroRange;// This holds the range in which the AI will stayed aggroed on to a target, I'm going to use this instead of sight range becuase I think the aggro range should be bigger after the target is in range
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AlertRange;// This holds the range in which the AI will notice a target, but not turn hostile. Instead it will start to move towards the target to get a better look
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AIAlertRange;// This holds the range another ai has to be into be alerted by this one
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AttackRange;// This holds the range thisai can attack from
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float AttackPower;// This holds how strong this ai's attacks are
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Attributes")
		float Health;// This holds the health of the ai
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		bool AttackStarted;// This holds wether a attack has started or not
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		bool AttackEnded;// This holds wether a attack has ended or not
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		bool TargetHit;// This variables holds wether or not the ai's attack hit it's target
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		AEnemyState EState;// This variables holds the current state the ai is in
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		AEnemyConditionState ECState;// This variables holds the current condition state the ai is in
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		TArray<FTInfo> Targets;// Holds each target that has been spotted
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Base Attributes")
		FTInfo Target;// This holds the current target if there is one
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Behavior")
		UBehaviorTree* BBTree;
#pragma endregion

protected:
	// These shouldn't be accessed outside of the code
#pragma region Code Only Variables
	class UWorld*				World;// This holds a world object, used for accessing navigation amoung other things
	class ACharacter*			ControledCharacter;// This holds this a reference to this controller
	float						MaxHealth;// This holds the maximuim health that the ai can have
	float						AttackDelayTimer;// This holds the delay that needs to pass before the ai's next attack can compense
	float						WanderingTimer;// This holds how much time the ai has left to wander
	float						IdleTimer;// This will hold how much time has to pass while the ai is idle before it switchs it state
	float						AggroTimer;// This will hold how much time has past while the ai is waiting to aggro choose aggro again
	FString						StateName;// This will hold the name of the state the ai is currently in
	FString						PreviousStateName;// This holds the name of the previous state if there is one
	FVector						TargetsLastKnownPosition;// This will hold the last known location of a target if there is one
	FVector						AttackLocation;// Holds the location of the target after it has gotten close enought o be attacked
	TMap<AEnemyState, void(AEnemyAIController::*)(void)>	StateFuncs;// This is a dictionary of all the state functions (the functions that match each state by name), the State is the key with the function being the value
#pragma endregion
};
