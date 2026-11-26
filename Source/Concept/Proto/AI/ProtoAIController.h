// Enemy brain: a plain C++ state machine (Idle/Patrol -> Alert -> Engage -> Search) driven by AI Perception.
// Per-weapon behaviour comes from FProtoAIWeaponProfile. No behaviour tree, no blackboard.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoAIController.generated.h"

class AProtoEnemyCharacter;
class AProtoWeapon;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;

/** How an enemy fights with one weapon type. */
USTRUCT()
struct FProtoAIWeaponProfile
{
	GENERATED_BODY()

	/** Distance band the enemy tries to stay in (cm). */
	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0", Units = "cm"))
	float MinRange = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0", Units = "cm"))
	float MaxRange = 2000.0f;

	/** Never fires when the target is closer than this (cm). RPG uses 10 m. */
	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0", Units = "cm"))
	float MinFireDistance = 0.0f;

	/** Shots per burst (random in range). 1 = single shots. */
	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "1"))
	int32 BurstMin = 1;

	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "1"))
	int32 BurstMax = 1;

	/** Pause between shots inside a burst for non-auto weapons (s). */
	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float ShotGapMin = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float ShotGapMax = 0.0f;

	/** Pause between bursts (s). */
	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float BurstPauseMin = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float BurstPauseMax = 1.2f;

	/** Aim time before each shot with the warning glint (sniper aim, RPG wind-up). 0 = none. */
	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float WindUpMin = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float WindUpMax = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Firing")
	FLinearColor GlintColor = FLinearColor(1.0f, 1.0f, 0.9f);

	/** Fire mode the enemy selects on its weapon if available. */
	UPROPERTY(EditAnywhere, Category = "Firing")
	EProtoFireMode PreferredFireMode = EProtoFireMode::Semi;

	/** Aim at the target's feet (splash weapons). */
	UPROPERTY(EditAnywhere, Category = "Firing")
	bool bAimAtGround = false;

	/** Keeps firing at the last known position for SuppressTime after losing sight. */
	UPROPERTY(EditAnywhere, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float SuppressTime = 0.0f;

	/** Keep moving while in range (pistol). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bKeepMoving = false;

	/** Sidestep between bursts (rifle). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bSidestep = false;

	/** Stay put once engaged (MG). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bHoldPosition = false;

	/** Move to a new spot after each shot (sniper). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bRelocateAfterShot = false;

	/** Back away from the target while reloading (RPG). */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bRetreatWhileReloading = false;

	/** Sidestep / strafe / relocate distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0", Units = "cm"))
	float RepositionDistance = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float MoveSpeed = 380.0f;
};

UCLASS()
class CONCEPT_API AProtoAIController : public AAIController
{
	GENERATED_BODY()

public:
	AProtoAIController();

	virtual void Tick(float DeltaSeconds) override;

	EProtoAIState GetState() const { return State; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void HandleWeaponFired(AProtoWeapon* Weapon);

	void SetState(EProtoAIState NewState, bool bForce = false);
	void TickIdle(float DeltaSeconds);
	void TickAlert(float DeltaSeconds);
	void TickEngage(float DeltaSeconds);
	void TickSearch(float DeltaSeconds);

	/** Positioning for the current weapon while engaged. */
	void UpdateEngageMovement(float DeltaSeconds, float Distance);
	/** Fire control: bursts, wind-up, suppression. AimPoint is where the gun points (error included). */
	void UpdateFiring(float DeltaSeconds, bool bHasShot, float Distance);
	void UpdateAim(float DeltaSeconds, const FVector& BaseAimPoint);
	void StopFiring();
	void StartBurstPause();
	void BindWeapon(AProtoWeapon* Weapon);
	void UpdateSearchCenter();
	void SetMoveSpeed(float Speed);

	bool MoveToNavPoint(const FVector& Desired, float SearchRadius);
	FVector GetTargetAimPoint() const;
	bool IsTargetValid() const;
	const FProtoAIWeaponProfile& GetProfile() const;
	AProtoWeapon* GetWeapon() const;
	void DrawDebug() const;

	// ---- Perception ----
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY(Transient)
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(Transient)
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(EditAnywhere, Category = "AI|Perception", meta = (ClampMin = "0", Units = "cm"))
	float SightRadius = 16000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception", meta = (ClampMin = "0", Units = "cm"))
	float LoseSightRadius = 17000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float PeripheralVisionHalfAngle = 70.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception", meta = (ClampMin = "0", Units = "cm"))
	float HearingRange = 8000.0f;

	/** Seconds a stimulus is remembered. */
	UPROPERTY(EditAnywhere, Category = "AI|Perception", meta = (ClampMin = "0", Units = "s"))
	float StimulusMaxAge = 5.0f;

	// ---- Fairness ----
	/** Delay between first seeing the player and the first shot (s, random in range). */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0"))
	FVector2D ReactionTime = FVector2D(0.5f, 0.9f);

	/** Aim error cone half-angle when the target is first seen (degrees). */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0", Units = "Degrees"))
	float AimErrorStart = 6.0f;

	/** Aim error cone half-angle after tracking the target for AimSettleTime (degrees). */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0", Units = "Degrees"))
	float AimErrorMin = 0.8f;

	/** Seconds of continuous sight for the error to shrink from start to min. */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0.01", Units = "s"))
	float AimSettleTime = 4.0f;

	/** How often a new aim-error offset is picked (s). The aim drifts toward it. */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0.01", Units = "s"))
	float AimJitterInterval = 0.35f;

	/** How fast the aim drifts toward the new error offset (per second). */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0"))
	float AimDriftSpeed = 6.0f;

	/** Share of the settled aim kept after losing sight (0 = start over). */
	UPROPERTY(EditAnywhere, Category = "AI|Fairness", meta = (ClampMin = "0", ClampMax = "1"))
	float AimSettleKeptOnLoss = 0.5f;

	// ---- Patrol / search ----
	UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "0", Units = "cm"))
	float PatrolRadius = 1500.0f;

	/** Seconds to wait at each patrol point (random in range). */
	UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "0"))
	FVector2D PatrolWait = FVector2D(1.5f, 4.0f);

	UPROPERTY(EditAnywhere, Category = "AI|Patrol", meta = (ClampMin = "0"))
	float PatrolSpeed = 180.0f;

	/** Seconds spent searching before giving up and returning to patrol. */
	UPROPERTY(EditAnywhere, Category = "AI|Search", meta = (ClampMin = "0", Units = "s"))
	float SearchDuration = 6.0f;

	/** Radius around the last known position to look around in (cm). */
	UPROPERTY(EditAnywhere, Category = "AI|Search", meta = (ClampMin = "0", Units = "cm"))
	float SearchRadius = 600.0f;

	/** Movement goal acceptance radius (cm). */
	UPROPERTY(EditAnywhere, Category = "AI|Movement", meta = (ClampMin = "0", Units = "cm"))
	float AcceptanceRadius = 50.0f;

	/** Seconds between positioning decisions while engaged. */
	UPROPERTY(EditAnywhere, Category = "AI|Movement", meta = (ClampMin = "0.05", Units = "s"))
	float RepositionInterval = 1.2f;

	/** Nav projection search extent when picking move goals (cm). */
	UPROPERTY(EditAnywhere, Category = "AI|Movement", meta = (ClampMin = "0", Units = "cm"))
	float NavSearchRadius = 300.0f;

	/** Aim height above the target's feet when aiming at the ground (cm). */
	UPROPERTY(EditAnywhere, Category = "AI|Firing", meta = (Units = "cm"))
	float GroundAimHeight = 5.0f;

	/** Per-weapon behaviour. */
	UPROPERTY(EditAnywhere, Category = "AI|Weapons")
	TMap<EProtoWeaponType, FProtoAIWeaponProfile> WeaponProfiles;

	UPROPERTY(EditAnywhere, Category = "AI|Debug", meta = (ClampMin = "0"))
	float DebugTextHeight = 120.0f;

	// ---- State ----
	UPROPERTY(Transient)
	TObjectPtr<AProtoEnemyCharacter> Enemy;

	TWeakObjectPtr<APawn> Target;
	EProtoAIState State = EProtoAIState::Idle;
	FVector HomeLocation = FVector::ZeroVector;
	FVector LastKnownLocation = FVector::ZeroVector;
	/** Where the search looks around; kept at the weapon's minimum range from the last known position. */
	FVector SearchCenter = FVector::ZeroVector;
	bool bHasLastKnown = false;
	bool bCanSeeTarget = false;
	float StateTime = 0.0f;
	float StateTimer = 0.0f;
	float TimeSinceSeen = 0.0f;
	float SightTime = 0.0f;
	float RepositionTimer = 0.0f;
	float FireTimer = 0.0f;
	float WindUpRemaining = 0.0f;
	float AimJitterTimer = 0.0f;
	int32 ShotsLeftInBurst = 0;
	int32 SidestepSign = 1;
	bool bWindingUp = false;
	bool bRelocating = false;
	bool bMoving = false;
	bool bWasFrozen = false;
	FVector2D AimError = FVector2D::ZeroVector;
	FVector2D AimErrorTarget = FVector2D::ZeroVector;
	FDelegateHandle FiredHandle;
	TWeakObjectPtr<AProtoWeapon> BoundWeapon;
};
