#include "Proto/AI/ProtoAIController.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Proto/AI/ProtoEnemyCharacter.h"
#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Weapons/ProtoWeapon.h"
#include "Proto/Weapons/ProtoWeaponData.h"

namespace
{
	/** Distance of the idle aim point in front of the enemy (cm). Only the direction matters. */
	constexpr float IdleAimDistance = 1000.0f;

	/** Radius of the debug spheres (cm). */
	constexpr float DebugSphereRadius = 30.0f;
	constexpr int32 DebugSphereSegments = 8;

	const TCHAR* AIStateLabel(EProtoAIState State)
	{
		switch (State)
		{
		case EProtoAIState::Idle:	return TEXT("Idle");
		case EProtoAIState::Alert:	return TEXT("Alert");
		case EProtoAIState::Engage:	return TEXT("Engage");
		case EProtoAIState::Search:	return TEXT("Search");
		default:					return TEXT("?");
		}
	}

	float RandomInRange(const FVector2D& Range)
	{
		return FMath::FRandRange(Range.X, FMath::Max(Range.X, Range.Y));
	}
}

AProtoAIController::AProtoAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(StimulusMaxAge);
	// No teams in the prototype: detect everything, the handler keeps only player-controlled pawns.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	Perception->ConfigureSense(*SightConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(StimulusMaxAge);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	Perception->ConfigureSense(*HearingConfig);

	Perception->SetDominantSense(UAISense_Sight::StaticClass());

	// ---- Default per-weapon behaviour (from the brief) ----
	{
		FProtoAIWeaponProfile Pistol;
		Pistol.MinRange = 1000.0f;
		Pistol.MaxRange = 2000.0f;
		Pistol.BurstMin = 1;
		Pistol.BurstMax = 1;
		Pistol.ShotGapMin = 0.4f;
		Pistol.ShotGapMax = 0.8f;
		Pistol.BurstPauseMin = 0.4f;
		Pistol.BurstPauseMax = 0.8f;
		Pistol.PreferredFireMode = EProtoFireMode::Semi;
		Pistol.bKeepMoving = true;
		Pistol.RepositionDistance = 400.0f;
		Pistol.MoveSpeed = 380.0f;
		WeaponProfiles.Add(EProtoWeaponType::Pistol, Pistol);
	}
	{
		FProtoAIWeaponProfile Rifle;
		Rifle.MinRange = 2000.0f;
		Rifle.MaxRange = 5000.0f;
		Rifle.BurstMin = 3;
		Rifle.BurstMax = 5;
		Rifle.BurstPauseMin = 0.8f;
		Rifle.BurstPauseMax = 1.5f;
		Rifle.PreferredFireMode = EProtoFireMode::Auto;
		Rifle.bSidestep = true;
		Rifle.RepositionDistance = 300.0f;
		Rifle.MoveSpeed = 350.0f;
		WeaponProfiles.Add(EProtoWeaponType::Rifle, Rifle);
	}
	{
		FProtoAIWeaponProfile MachineGun;
		MachineGun.MinRange = 0.0f;
		MachineGun.MaxRange = 6000.0f;
		MachineGun.BurstMin = 8;
		MachineGun.BurstMax = 15;
		MachineGun.BurstPauseMin = 0.8f;
		MachineGun.BurstPauseMax = 1.4f;
		MachineGun.PreferredFireMode = EProtoFireMode::Auto;
		MachineGun.SuppressTime = 2.0f;
		MachineGun.bHoldPosition = true;
		MachineGun.MoveSpeed = 250.0f;
		WeaponProfiles.Add(EProtoWeaponType::MachineGun, MachineGun);
	}
	{
		FProtoAIWeaponProfile Sniper;
		Sniper.MinRange = 6000.0f;
		Sniper.MaxRange = 15000.0f;
		Sniper.BurstMin = 1;
		Sniper.BurstMax = 1;
		Sniper.BurstPauseMin = 0.5f;
		Sniper.BurstPauseMax = 1.0f;
		Sniper.WindUpMin = 1.5f;
		Sniper.WindUpMax = 2.0f;
		Sniper.GlintColor = FLinearColor(1.0f, 1.0f, 0.9f);
		Sniper.PreferredFireMode = EProtoFireMode::Bolt;
		Sniper.bRelocateAfterShot = true;
		Sniper.RepositionDistance = 800.0f;
		Sniper.MoveSpeed = 400.0f;
		WeaponProfiles.Add(EProtoWeaponType::Sniper, Sniper);
	}
	{
		FProtoAIWeaponProfile Rpg;
		Rpg.MinRange = 2000.0f;
		Rpg.MaxRange = 6000.0f;
		Rpg.MinFireDistance = 1000.0f;
		Rpg.BurstMin = 1;
		Rpg.BurstMax = 1;
		Rpg.BurstPauseMin = 0.5f;
		Rpg.BurstPauseMax = 1.0f;
		Rpg.WindUpMin = 0.8f;
		Rpg.WindUpMax = 1.2f;
		Rpg.GlintColor = FLinearColor(1.0f, 0.4f, 0.05f);
		Rpg.PreferredFireMode = EProtoFireMode::Single;
		Rpg.bAimAtGround = true;
		Rpg.bRetreatWhileReloading = true;
		Rpg.RepositionDistance = 600.0f;
		Rpg.MoveSpeed = 300.0f;
		WeaponProfiles.Add(EProtoWeaponType::RPG, Rpg);
	}
}

void AProtoAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Enemy = Cast<AProtoEnemyCharacter>(InPawn);
	HomeLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;

	// Re-apply the perception tunables in case they were edited on this instance.
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(StimulusMaxAge);
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(StimulusMaxAge);
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AProtoAIController::HandlePerceptionUpdated);

	SetState(EProtoAIState::Idle, true);
}

void AProtoAIController::OnUnPossess()
{
	StopFiring();
	BindWeapon(nullptr);
	Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &AProtoAIController::HandlePerceptionUpdated);
	Enemy = nullptr;
	Super::OnUnPossess();
}

void AProtoAIController::BindWeapon(AProtoWeapon* Weapon)
{
	if (AProtoWeapon* Old = BoundWeapon.Get())
	{
		Old->OnFired.Remove(FiredHandle);
	}
	FiredHandle.Reset();
	BoundWeapon = Weapon;

	if (Weapon)
	{
		FiredHandle = Weapon->OnFired.AddUObject(this, &AProtoAIController::HandleWeaponFired);
		Weapon->SetFireMode(GetProfile().PreferredFireMode);
		Weapon->SetSpread(0.0f); // The aim error cone replaces weapon spread for enemies.
	}
}

AProtoWeapon* AProtoAIController::GetWeapon() const
{
	return Enemy ? Enemy->GetWeapon() : nullptr;
}

const FProtoAIWeaponProfile& AProtoAIController::GetProfile() const
{
	static const FProtoAIWeaponProfile DefaultProfile;
	const FProtoAIWeaponProfile* Profile = Enemy ? WeaponProfiles.Find(Enemy->GetWeaponType()) : nullptr;
	return Profile ? *Profile : DefaultProfile;
}

bool AProtoAIController::IsTargetValid() const
{
	const APawn* TargetPawn = Target.Get();
	if (!TargetPawn)
	{
		return false;
	}
	const UProtoHealthComponent* TargetHealth = UProtoHealthComponent::FindOn(TargetPawn);
	return !TargetHealth || !TargetHealth->IsDead();
}

FVector AProtoAIController::GetTargetAimPoint() const
{
	const APawn* TargetPawn = Target.Get();
	if (!TargetPawn)
	{
		return LastKnownLocation;
	}

	if (GetProfile().bAimAtGround)
	{
		const float HalfHeight = TargetPawn->GetSimpleCollisionHalfHeight();
		return TargetPawn->GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight - GroundAimHeight);
	}
	// Centre mass: the capsule centre.
	return TargetPawn->GetActorLocation();
}

void AProtoAIController::SetMoveSpeed(float Speed)
{
	if (Enemy)
	{
		Enemy->GetCharacterMovement()->MaxWalkSpeed = Speed;
	}
}

bool AProtoAIController::MoveToNavPoint(const FVector& Desired, float SearchExtent)
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSystem)
	{
		return false;
	}

	FNavLocation NavLocation;
	if (!NavSystem->ProjectPointToNavigation(Desired, NavLocation, FVector(SearchExtent)))
	{
		return false;
	}

	const EPathFollowingRequestResult::Type Result = MoveToLocation(NavLocation.Location, AcceptanceRadius,
		/*bStopOnOverlap*/ true, /*bUsePathfinding*/ true, /*bProjectDestinationToNavigation*/ false, /*bCanStrafe*/ true);
	bMoving = Result != EPathFollowingRequestResult::Failed;
	return bMoving;
}

// ---------------------------------------------------------------------------------------------------------------------
// Perception

void AProtoAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	APawn* SensedPawn = Cast<APawn>(Actor);
	if (!Enemy || Enemy->IsDead() || !SensedPawn || !SensedPawn->IsPlayerControlled())
	{
		return;
	}
	if (const UProtoHealthComponent* SensedHealth = UProtoHealthComponent::FindOn(SensedPawn))
	{
		if (SensedHealth->IsDead())
		{
			return;
		}
	}

	const bool bSensed = Stimulus.WasSuccessfullySensed();

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (bSensed)
		{
			Target = SensedPawn;
			bCanSeeTarget = true;
			LastKnownLocation = SensedPawn->GetActorLocation();
			bHasLastKnown = true;
			if (State == EProtoAIState::Idle || State == EProtoAIState::Search)
			{
				SetState(EProtoAIState::Alert);
			}
		}
		else if (Target.Get() == SensedPawn)
		{
			bCanSeeTarget = false;
			LastKnownLocation = Stimulus.StimulusLocation;
			bHasLastKnown = true;
			SightTime *= AimSettleKeptOnLoss;
		}
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && bSensed)
	{
		if (!Target.IsValid())
		{
			Target = SensedPawn;
		}
		if (bCanSeeTarget)
		{
			return; // Sight is more accurate than a heard shot.
		}
		LastKnownLocation = Stimulus.StimulusLocation;
		bHasLastKnown = true;
		if (State == EProtoAIState::Idle)
		{
			SetState(EProtoAIState::Alert);
		}
		else if (State == EProtoAIState::Search)
		{
			SetState(EProtoAIState::Search, true); // Restart the search around the new noise.
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// State machine

void AProtoAIController::SetState(EProtoAIState NewState, bool bForce)
{
	if (NewState == State && !bForce)
	{
		return;
	}

	if (State == EProtoAIState::Engage && NewState != EProtoAIState::Engage)
	{
		StopFiring();
		bRelocating = false;
	}

	State = NewState;
	StateTime = 0.0f;
	StateTimer = 0.0f;
	bMoving = false;
	StopMovement();

	const FProtoAIWeaponProfile& Profile = GetProfile();

	switch (NewState)
	{
	case EProtoAIState::Idle:
		ClearFocus(EAIFocusPriority::Gameplay);
		if (Enemy)
		{
			Enemy->SetCombatRotation(false);
		}
		SetMoveSpeed(PatrolSpeed);
		Target.Reset();
		bCanSeeTarget = false;
		bHasLastKnown = false;
		SightTime = 0.0f;
		break;

	case EProtoAIState::Alert:
		StateTimer = RandomInRange(ReactionTime);
		if (Enemy)
		{
			Enemy->SetCombatRotation(true);
		}
		SetFocalPoint(LastKnownLocation);
		break;

	case EProtoAIState::Engage:
		if (Enemy)
		{
			Enemy->SetCombatRotation(true);
		}
		SetMoveSpeed(Profile.MoveSpeed);
		ShotsLeftInBurst = 0;
		FireTimer = 0.0f;
		RepositionTimer = 0.0f;
		AimJitterTimer = 0.0f;
		TimeSinceSeen = 0.0f;
		bRelocating = false;
		bWindingUp = false;
		break;

	case EProtoAIState::Search:
		if (Enemy)
		{
			Enemy->SetCombatRotation(false);
		}
		SetMoveSpeed(Profile.MoveSpeed);
		UpdateSearchCenter();
		SetFocalPoint(LastKnownLocation);
		break;

	default:
		break;
	}
}

void AProtoAIController::UpdateSearchCenter()
{
	if (!Enemy)
	{
		SearchCenter = LastKnownLocation;
		return;
	}

	// Ranged enemies search from their preferred minimum range instead of walking onto the spot.
	const FVector ToEnemy = Enemy->GetActorLocation() - LastKnownLocation;
	const float Distance = ToEnemy.Size2D();
	const float Standoff = FMath::Min(Distance, GetProfile().MinRange);
	SearchCenter = LastKnownLocation + ToEnemy.GetSafeNormal2D() * Standoff;
}

void AProtoAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Enemy || Enemy->IsDead())
	{
		return;
	}

	if (BoundWeapon.Get() != GetWeapon())
	{
		// The weapon is spawned in the pawn's BeginPlay, which runs after possession.
		BindWeapon(GetWeapon());
	}

	const UProtoDebugSubsystem* Debug = UProtoDebugSubsystem::Get(this);
	if (Debug && Debug->bFreezeAI)
	{
		if (!bWasFrozen)
		{
			StopMovement();
			StopFiring();
			bMoving = false;
			bWasFrozen = true;
		}
		if (Debug->bDebugDraw)
		{
			DrawDebug();
		}
		return;
	}
	bWasFrozen = false;

	if (State != EProtoAIState::Idle && Target.IsValid() && !IsTargetValid())
	{
		SetState(EProtoAIState::Idle); // Target died.
	}

	StateTime += DeltaSeconds;

	switch (State)
	{
	case EProtoAIState::Idle:	TickIdle(DeltaSeconds);		break;
	case EProtoAIState::Alert:	TickAlert(DeltaSeconds);	break;
	case EProtoAIState::Engage:	TickEngage(DeltaSeconds);	break;
	case EProtoAIState::Search:	TickSearch(DeltaSeconds);	break;
	default: break;
	}

	if (State != EProtoAIState::Engage)
	{
		// Out of combat the gun points where the body faces.
		Enemy->AimWeaponAt(Enemy->GetAimOrigin() + Enemy->GetActorForwardVector() * IdleAimDistance);
	}

	if (Debug && Debug->bDebugDraw)
	{
		DrawDebug();
	}
}

void AProtoAIController::TickIdle(float DeltaSeconds)
{
	if (bMoving)
	{
		if (GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			return;
		}
		bMoving = false;
		StateTimer = RandomInRange(PatrolWait);
	}

	StateTimer -= DeltaSeconds;
	if (StateTimer > 0.0f)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation PatrolPoint;
	if (NavSystem && NavSystem->GetRandomReachablePointInRadius(HomeLocation, PatrolRadius, PatrolPoint))
	{
		MoveToNavPoint(PatrolPoint.Location, NavSearchRadius);
	}
	if (!bMoving)
	{
		StateTimer = RandomInRange(PatrolWait); // No nav mesh or no point: try again later.
	}
}

void AProtoAIController::TickAlert(float DeltaSeconds)
{
	// Reaction delay: turn toward the stimulus, then commit.
	if (bCanSeeTarget && Target.IsValid())
	{
		LastKnownLocation = Target->GetActorLocation();
	}
	SetFocalPoint(LastKnownLocation);

	StateTimer -= DeltaSeconds;
	if (StateTimer > 0.0f)
	{
		return;
	}

	if (bCanSeeTarget && IsTargetValid())
	{
		SetState(EProtoAIState::Engage);
	}
	else if (bHasLastKnown)
	{
		SetState(EProtoAIState::Search);
	}
	else
	{
		SetState(EProtoAIState::Idle);
	}
}

void AProtoAIController::TickEngage(float DeltaSeconds)
{
	const FProtoAIWeaponProfile& Profile = GetProfile();

	if (!IsTargetValid())
	{
		SetState(EProtoAIState::Idle);
		return;
	}

	if (bCanSeeTarget)
	{
		TimeSinceSeen = 0.0f;
		SightTime += DeltaSeconds;
		LastKnownLocation = Target->GetActorLocation();
		bHasLastKnown = true;
	}
	else
	{
		TimeSinceSeen += DeltaSeconds;
		if (TimeSinceSeen > Profile.SuppressTime)
		{
			SetState(EProtoAIState::Search);
			return;
		}
	}

	const FVector BaseAimPoint = bCanSeeTarget ? GetTargetAimPoint() : LastKnownLocation;
	SetFocalPoint(BaseAimPoint);

	const float Distance = FVector::Dist(Enemy->GetActorLocation(), LastKnownLocation);
	UpdateEngageMovement(DeltaSeconds, Distance);
	UpdateAim(DeltaSeconds, BaseAimPoint);

	// Suppression (MG): keep shooting at the last known position for a short time after losing sight.
	const bool bHasShot = bCanSeeTarget || Profile.SuppressTime > 0.0f;
	UpdateFiring(DeltaSeconds, bHasShot, Distance);
}

void AProtoAIController::TickSearch(float DeltaSeconds)
{
	if (StateTime > SearchDuration)
	{
		SetState(EProtoAIState::Idle);
		return;
	}

	if (bMoving && GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}
	bMoving = false;

	// Head to the search centre first, then look around it.
	const bool bAtCenter = FVector::Dist2D(Enemy->GetActorLocation(), SearchCenter) <= SearchRadius;
	FVector Goal = SearchCenter;
	if (bAtCenter)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		const FVector2D Offset = FMath::RandPointInCircle(SearchRadius);
		Goal = SearchCenter + FVector(Offset.X, Offset.Y, 0.0f);
	}
	if (!MoveToNavPoint(Goal, NavSearchRadius) && bAtCenter)
	{
		// Nothing reachable to look at: give up early.
		SetState(EProtoAIState::Idle);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Combat

void AProtoAIController::UpdateEngageMovement(float DeltaSeconds, float Distance)
{
	const FProtoAIWeaponProfile& Profile = GetProfile();
	AProtoWeapon* Weapon = GetWeapon();
	const FVector EnemyLocation = Enemy->GetActorLocation();
	const FVector AwayFromTarget = (EnemyLocation - LastKnownLocation).GetSafeNormal2D();

	if (bMoving && GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		bMoving = false;
	}

	RepositionTimer -= DeltaSeconds;

	// RPG: back away while reloading.
	if (Profile.bRetreatWhileReloading && Weapon && Weapon->IsReloading())
	{
		if (!bMoving || RepositionTimer <= 0.0f)
		{
			RepositionTimer = RepositionInterval;
			MoveToNavPoint(EnemyLocation + AwayFromTarget * Profile.RepositionDistance, NavSearchRadius);
		}
		return;
	}

	// Sniper: finish relocating before doing anything else.
	if (bRelocating)
	{
		if (!bMoving)
		{
			bRelocating = false;
		}
		return;
	}

	if (Profile.bHoldPosition || RepositionTimer > 0.0f)
	{
		return;
	}
	RepositionTimer = RepositionInterval;

	const bool bTooFar = Distance > Profile.MaxRange;
	const bool bTooClose = Distance < Profile.MinRange;
	if (bTooFar || bTooClose)
	{
		const float BandMiddle = (Profile.MinRange + Profile.MaxRange) * 0.5f;
		MoveToNavPoint(LastKnownLocation + AwayFromTarget * BandMiddle, NavSearchRadius);
		return;
	}

	if (Profile.bKeepMoving && !bMoving)
	{
		// Pistol: strafe back and forth across the line of fire.
		const FVector Side = FVector::CrossProduct(FVector::UpVector, AwayFromTarget) * static_cast<float>(SidestepSign);
		SidestepSign = -SidestepSign;
		MoveToNavPoint(EnemyLocation + Side * Profile.RepositionDistance, NavSearchRadius);
	}
}

void AProtoAIController::UpdateAim(float DeltaSeconds, const FVector& BaseAimPoint)
{
	const float SettleAlpha = FMath::Clamp(SightTime / AimSettleTime, 0.0f, 1.0f);
	const float ErrorDegrees = FMath::Lerp(AimErrorStart, AimErrorMin, SettleAlpha);

	AimJitterTimer -= DeltaSeconds;
	if (AimJitterTimer <= 0.0f)
	{
		AimJitterTimer = AimJitterInterval;
		AimErrorTarget = FMath::RandPointInCircle(ErrorDegrees);
	}
	AimError = FMath::Vector2DInterpTo(AimError, AimErrorTarget, DeltaSeconds, AimDriftSpeed);

	const FVector Origin = Enemy->GetAimOrigin();
	const FVector ToTarget = BaseAimPoint - Origin;
	FRotator AimRotation = ToTarget.Rotation();
	AimRotation.Yaw += AimError.X;
	AimRotation.Pitch += AimError.Y;
	Enemy->AimWeaponAt(Origin + AimRotation.Vector() * ToTarget.Size());
}

void AProtoAIController::UpdateFiring(float DeltaSeconds, bool bHasShot, float Distance)
{
	const FProtoAIWeaponProfile& Profile = GetProfile();
	AProtoWeapon* Weapon = GetWeapon();
	if (!Weapon || !Weapon->GetData())
	{
		return;
	}

	if (Weapon->IsReloading())
	{
		StopFiring();
		ShotsLeftInBurst = 0;
		return;
	}
	if (Weapon->GetAmmo() <= 0)
	{
		StopFiring();
		ShotsLeftInBurst = 0;
		Weapon->StartReload();
		return;
	}

	const bool bCanShoot = bHasShot && !bRelocating && Distance >= Profile.MinFireDistance;
	if (!bCanShoot)
	{
		StopFiring();
		ShotsLeftInBurst = 0;
		return;
	}

	FireTimer -= DeltaSeconds;
	if (FireTimer > 0.0f)
	{
		return;
	}

	if (ShotsLeftInBurst <= 0)
	{
		ShotsLeftInBurst = FMath::RandRange(Profile.BurstMin, FMath::Max(Profile.BurstMin, Profile.BurstMax));
		if (Profile.WindUpMax > 0.0f)
		{
			bWindingUp = true;
			WindUpRemaining = FMath::FRandRange(Profile.WindUpMin, FMath::Max(Profile.WindUpMin, Profile.WindUpMax));
			Weapon->SetGlint(true, Profile.GlintColor);
		}
	}

	if (bWindingUp)
	{
		WindUpRemaining -= DeltaSeconds;
		if (WindUpRemaining > 0.0f)
		{
			return;
		}
		bWindingUp = false;
		Weapon->SetGlint(false, Profile.GlintColor);
	}

	if (Weapon->GetFireMode() == EProtoFireMode::Auto)
	{
		// Hold the trigger; HandleWeaponFired counts the burst down and releases it.
		if (!Weapon->IsTriggerHeld())
		{
			Weapon->PullTrigger();
		}
		return;
	}

	// Semi / bolt / single: one press per shot, spaced by the gap and the weapon's cycle time.
	Weapon->PullTrigger();
	Weapon->ReleaseTrigger();
	const float Gap = FMath::FRandRange(Profile.ShotGapMin, FMath::Max(Profile.ShotGapMin, Profile.ShotGapMax));
	FireTimer = FMath::Max(Gap, Weapon->GetData()->GetShotInterval());
}

void AProtoAIController::HandleWeaponFired(AProtoWeapon* Weapon)
{
	if (State != EProtoAIState::Engage || !Enemy)
	{
		return;
	}

	--ShotsLeftInBurst;
	if (ShotsLeftInBurst > 0)
	{
		return;
	}

	Weapon->ReleaseTrigger();
	StartBurstPause();

	const FProtoAIWeaponProfile& Profile = GetProfile();
	const FVector EnemyLocation = Enemy->GetActorLocation();
	const FVector AwayFromTarget = (EnemyLocation - LastKnownLocation).GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, AwayFromTarget);

	if (Profile.bRelocateAfterShot)
	{
		// Sniper: move sideways to a new spot (random side) after every shot.
		const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;
		bRelocating = MoveToNavPoint(EnemyLocation + Side * SideSign * Profile.RepositionDistance, NavSearchRadius);
	}
	else if (Profile.bSidestep)
	{
		// Rifle: sidestep between bursts.
		MoveToNavPoint(EnemyLocation + Side * static_cast<float>(SidestepSign) * Profile.RepositionDistance, NavSearchRadius);
		SidestepSign = FMath::RandBool() ? 1 : -1;
	}
}

void AProtoAIController::StartBurstPause()
{
	const FProtoAIWeaponProfile& Profile = GetProfile();
	ShotsLeftInBurst = 0;
	FireTimer = FMath::FRandRange(Profile.BurstPauseMin, FMath::Max(Profile.BurstPauseMin, Profile.BurstPauseMax));
}

void AProtoAIController::StopFiring()
{
	bWindingUp = false;
	if (AProtoWeapon* Weapon = GetWeapon())
	{
		Weapon->ReleaseTrigger();
		Weapon->SetGlint(false, FLinearColor::Black);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Debug

void AProtoAIController::DrawDebug() const
{
	const UWorld* World = GetWorld();
	if (!World || !Enemy)
	{
		return;
	}

	const FVector EnemyLocation = Enemy->GetActorLocation();
	const AProtoWeapon* Weapon = GetWeapon();
	const UProtoHealthComponent* EnemyHealth = Enemy->GetHealth();
	const float SettleAlpha = FMath::Clamp(SightTime / AimSettleTime, 0.0f, 1.0f);

	const FString Text = FString::Printf(TEXT("%s%s  HP %.0f  Ammo %d\nErr %.1f deg  See %s  Timer %.1f%s%s"),
		AIStateLabel(State),
		bWasFrozen ? TEXT(" (frozen)") : TEXT(""),
		EnemyHealth ? EnemyHealth->GetHealth() : 0.0f,
		Weapon ? Weapon->GetAmmo() : 0,
		FMath::Lerp(AimErrorStart, AimErrorMin, SettleAlpha),
		bCanSeeTarget ? TEXT("yes") : TEXT("no"),
		State == EProtoAIState::Engage ? FireTimer : StateTimer,
		bWindingUp ? TEXT("  WINDUP") : TEXT(""),
		bRelocating ? TEXT("  RELOCATE") : TEXT(""));

	FColor Color = FColor::Green;
	switch (State)
	{
	case EProtoAIState::Alert:	Color = FColor::Yellow;	break;
	case EProtoAIState::Engage:	Color = FColor::Red;	break;
	case EProtoAIState::Search:	Color = FColor::Orange;	break;
	default: break;
	}

	// Attached to the enemy (location is an offset) so each enemy keeps its own line; zero duration = this frame only.
	DrawDebugString(World, FVector(0.0f, 0.0f, DebugTextHeight), Text, Enemy.Get(), Color, 0.0f, true);

	if (bHasLastKnown)
	{
		DrawDebugSphere(World, LastKnownLocation, DebugSphereRadius, DebugSphereSegments, Color);
		DrawDebugLine(World, EnemyLocation, LastKnownLocation, bCanSeeTarget ? FColor::Red : FColor::Silver);
	}
	if (State == EProtoAIState::Search)
	{
		DrawDebugCircle(World, SearchCenter, SearchRadius, DebugSphereSegments * 4, Color, false, -1.0f, 0, 0.0f,
			FVector::ForwardVector, FVector::RightVector, false);
	}
	if (Weapon && Weapon->GetMuzzle())
	{
		const FVector Muzzle = Weapon->GetMuzzle()->GetComponentLocation();
		const float LineLength = bHasLastKnown ? FVector::Dist(Muzzle, LastKnownLocation) : IdleAimDistance;
		DrawDebugLine(World, Muzzle, Muzzle + Weapon->GetMuzzle()->GetForwardVector() * LineLength, FColor::Cyan);
	}
}
