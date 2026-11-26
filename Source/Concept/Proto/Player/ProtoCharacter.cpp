#include "Proto/Player/ProtoCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Proto/Core/ProtoGameMode.h"
#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Core/ProtoLog.h"
#include "Proto/Player/ProtoFirstPersonComponent.h"
#include "Proto/Weapons/ProtoWeapon.h"
#include "Proto/Weapons/ProtoWeaponData.h"

namespace
{
	// Construction defaults for engine components. They stay editable on the components in the details panel.
	constexpr float DefaultCapsuleRadius = 34.0f;
	constexpr float DefaultCapsuleHalfHeight = 88.0f;
	constexpr float DefaultCrouchedHalfHeight = 60.0f;
	constexpr float DefaultMaxAcceleration = 1400.0f;
	constexpr float DefaultBrakingDeceleration = 1100.0f;
	constexpr float DefaultGroundFriction = 4.0f;
	constexpr float DefaultAirControl = 0.1f;
	constexpr float DefaultJumpZVelocity = 420.0f;
}

AProtoCharacter::AProtoCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(DefaultCapsuleRadius, DefaultCapsuleHalfHeight);

	BaseEyeHeight = StandingCameraHeight;
	CrouchedEyeHeight = CrouchedCameraHeight;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, StandingCameraHeight));
	Camera->bUsePawnControlRotation = false; // The first-person rig drives the camera.

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(Camera);

	FirstPerson = CreateDefaultSubobject<UProtoFirstPersonComponent>(TEXT("FirstPerson"));
	Health = CreateDefaultSubobject<UProtoHealthComponent>(TEXT("Health"));

	GetMesh()->SetHiddenInGame(true);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = JogSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	Movement->MaxAcceleration = DefaultMaxAcceleration;
	Movement->BrakingDecelerationWalking = DefaultBrakingDeceleration;
	Movement->GroundFriction = DefaultGroundFriction;
	Movement->AirControl = DefaultAirControl;
	Movement->JumpZVelocity = DefaultJumpZVelocity;
	Movement->NavAgentProps.bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(DefaultCrouchedHalfHeight);
	Movement->bCanWalkOffLedgesWhenCrouching = true;
	Movement->bOrientRotationToMovement = false;

	// Input assets created in the editor under /Game/Prototype/Input.
	auto Soft = [](const TCHAR* Name) { return TSoftObjectPtr<UInputAction>(FSoftObjectPath(FString::Printf(TEXT("/Game/Prototype/Input/%s.%s"), Name, Name))); };
	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Prototype/Input/IMC_Default.IMC_Default")));
	MoveAction = Soft(TEXT("IA_Move"));
	LookAction = Soft(TEXT("IA_Look"));
	SprintAction = Soft(TEXT("IA_Sprint"));
	CrouchAction = Soft(TEXT("IA_Crouch"));
	JumpAction = Soft(TEXT("IA_Jump"));
	LeanLeftAction = Soft(TEXT("IA_LeanLeft"));
	LeanRightAction = Soft(TEXT("IA_LeanRight"));
	FireAction = Soft(TEXT("IA_Fire"));
	AimAction = Soft(TEXT("IA_Aim"));
	ReloadAction = Soft(TEXT("IA_Reload"));
	FireModeAction = Soft(TEXT("IA_FireMode"));
	for (int32 Slot = 1; Slot <= static_cast<int32>(EProtoWeaponType::Count); ++Slot)
	{
		WeaponSlotActions.Add(Soft(*FString::Printf(TEXT("IA_Weapon%d"), Slot)));
	}

	WeaponClass = AProtoWeapon::StaticClass();
}

void AProtoCharacter::BeginPlay()
{
	// The rig must know the camera before its own BeginPlay runs (inside Super::BeginPlay).
	FirstPerson->Setup(Camera, WeaponRoot);

	BaseEyeHeight = StandingCameraHeight;
	CrouchedEyeHeight = CrouchedCameraHeight;

	Super::BeginPlay();

	SpeedCap = JogSpeed;
	Health->OnDeath.AddUObject(this, &AProtoCharacter::HandleDeath);
	SpawnWeapons();
}

void AProtoCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AProtoWeapon* Weapon : Weapons)
	{
		if (Weapon)
		{
			Weapon->Destroy();
		}
	}
	Weapons.Reset();
	Super::EndPlay(EndPlayReason);
}

void AProtoCharacter::SpawnWeapons()
{
	AProtoGameMode* GameMode = AProtoGameMode::Get(this);
	UWorld* World = GetWorld();
	if (!GameMode || !World || !WeaponClass)
	{
		UE_LOG(LogProto, Warning, TEXT("%s: cannot spawn weapons (needs AProtoGameMode as the level's game mode)."), *GetName());
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (EProtoWeaponType Type : TEnumRange<EProtoWeaponType>())
	{
		AProtoWeapon* Weapon = nullptr;
		if (UProtoWeaponData* Data = GameMode->GetWeaponData(Type))
		{
			Weapon = World->SpawnActor<AProtoWeapon>(WeaponClass, WeaponRoot->GetComponentTransform(), Params);
			if (Weapon)
			{
				Weapon->Initialize(Data);
				Weapon->AttachToComponent(WeaponRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				Weapon->SetActorHiddenInGame(true);
				Weapon->SetFireBlocked(true);
				Weapon->OnFired.AddUObject(this, &AProtoCharacter::HandleWeaponFired);
			}
		}
		Weapons.Add(Weapon);
	}

	EquipSlot(static_cast<int32>(StartingWeapon));
	if (CurrentSlot == INDEX_NONE)
	{
		// Starting weapon missing: take the first one that exists.
		for (int32 Slot = 0; Slot < Weapons.Num() && CurrentSlot == INDEX_NONE; ++Slot)
		{
			EquipSlot(Slot);
		}
	}
}

AProtoWeapon* AProtoCharacter::GetCurrentWeapon() const
{
	return Weapons.IsValidIndex(CurrentSlot) ? Weapons[CurrentSlot].Get() : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// Movement state

bool AProtoCharacter::IsSprinting() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (bDead || !bWantsSprint || bIsCrouched || bAimHeld || !Movement->IsMovingOnGround())
	{
		return false;
	}
	const FVector MoveDirection = Movement->GetCurrentAcceleration().GetSafeNormal2D();
	return FVector::DotProduct(MoveDirection, GetActorForwardVector()) > SprintForwardThreshold;
}

bool AProtoCharacter::WantsToAim() const
{
	const AProtoWeapon* Weapon = GetCurrentWeapon();
	return bAimHeld && !bDead && Weapon && !Weapon->IsReloading() && PendingSlot == INDEX_NONE && !IsSprinting();
}

bool AProtoCharacter::WantsLowered() const
{
	return IsSprinting() || PendingSlot != INDEX_NONE || !GetCurrentWeapon();
}

float AProtoCharacter::GetLeanInput() const
{
	return (bLeanRightHeld ? 1.0f : 0.0f) - (bLeanLeftHeld ? 1.0f : 0.0f);
}

void AProtoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetCharacterMovement()->IsFalling())
	{
		LastFallSpeed = FMath::Max(LastFallSpeed, -GetVelocity().Z);
	}

	UpdateMovementSpeed(DeltaSeconds);
	UpdateWeaponState();
}

void AProtoCharacter::UpdateMovementSpeed(float DeltaSeconds)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	float LandingFactor = 1.0f;
	if (LandingSlowRemaining > 0.0f)
	{
		LandingSlowRemaining = FMath::Max(LandingSlowRemaining - DeltaSeconds, 0.0f);
		const float Recovery = LandingSlowDuration > 0.0f ? 1.0f - LandingSlowRemaining / LandingSlowDuration : 1.0f;
		LandingFactor = FMath::Lerp(LandingSlowMultiplier, 1.0f, Recovery);
	}

	const AProtoWeapon* Weapon = GetCurrentWeapon();
	const float WeaponFactor = (Weapon && Weapon->GetData()) ? Weapon->GetData()->MovementSpeedMultiplier : 1.0f;

	float Target = JogSpeed;
	if (IsSprinting())
	{
		Target = SprintSpeed;
	}
	else if (WantsToAim())
	{
		Target = WalkSpeed;
	}
	Target *= WeaponFactor * LandingFactor;

	SpeedCap = FMath::FInterpConstantTo(SpeedCap, Target, DeltaSeconds, SpeedCapBlendRate);
	Movement->MaxWalkSpeed = SpeedCap;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed * WeaponFactor * LandingFactor;
}

// ---------------------------------------------------------------------------------------------------------------------
// Weapons

void AProtoCharacter::UpdateWeaponState()
{
	if (PendingSlot != INDEX_NONE && FirstPerson->GetLoweredAlpha() >= SwitchSwapLoweredAlpha)
	{
		FinishWeaponSwitch();
	}

	AProtoWeapon* Weapon = GetCurrentWeapon();
	if (!Weapon)
	{
		return;
	}

	const bool bBlocked = bDead || IsSprinting() || PendingSlot != INDEX_NONE
		|| FirstPerson->GetLoweredAlpha() > FireBlockLoweredAlpha;
	Weapon->SetFireBlocked(bBlocked);
	if (bFireWasBlocked && !bBlocked && bFireHeld)
	{
		Weapon->PullTrigger(); // Fire was held through the block (e.g. pressed while sprinting).
	}
	bFireWasBlocked = bBlocked;

	if (const UProtoWeaponData* Data = Weapon->GetData())
	{
		const float MoveRatio = JogSpeed > 0.0f ? FMath::Clamp(GetVelocity().Size2D() / JogSpeed, 0.0f, 1.0f) : 0.0f;
		const float Spread = FMath::Lerp(Data->HipSpread, Data->AimSpread, FirstPerson->GetAimAlpha()) + Data->MovingSpread * MoveRatio;
		Weapon->SetSpread(Spread);
	}
}

void AProtoCharacter::EquipSlot(int32 Slot)
{
	if (bDead || !Weapons.IsValidIndex(Slot) || !Weapons[Slot])
	{
		return;
	}
	if (Slot == CurrentSlot)
	{
		PendingSlot = INDEX_NONE; // Changed our mind mid-switch: raise the same gun again.
		return;
	}

	if (AProtoWeapon* Current = GetCurrentWeapon())
	{
		Current->CancelReload();
		Current->ReleaseTrigger();
		PendingSlot = Slot; // The swap happens once the current gun is lowered.
		return;
	}

	PendingSlot = Slot;
	FinishWeaponSwitch();
}

void AProtoCharacter::FinishWeaponSwitch()
{
	if (AProtoWeapon* Old = GetCurrentWeapon())
	{
		Old->CancelReload();
		Old->ReleaseTrigger();
		Old->SetFireBlocked(true);
		Old->SetActorHiddenInGame(true);
	}

	CurrentSlot = PendingSlot;
	PendingSlot = INDEX_NONE;

	if (AProtoWeapon* New = GetCurrentWeapon())
	{
		New->SetActorHiddenInGame(false);
	}
}

void AProtoCharacter::HandleWeaponFired(AProtoWeapon* Weapon)
{
	FirstPerson->AddRecoil(Weapon);
}

void AProtoCharacter::HandleDeath(AController* Killer)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	bFireHeld = false;
	bAimHeld = false;
	bWantsSprint = false;
	bLeanLeftHeld = false;
	bLeanRightHeld = false;

	if (AProtoWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->ReleaseTrigger();
		Weapon->SetFireBlocked(true);
	}

	AController* OwnController = GetController();
	if (APlayerController* PlayerController = Cast<APlayerController>(OwnController))
	{
		DisableInput(PlayerController);
	}
	GetCharacterMovement()->StopMovementImmediately();
	FirstPerson->OnDeath();

	if (AProtoGameMode* GameMode = AProtoGameMode::Get(this))
	{
		GameMode->NotifyPlayerDied(OwnController);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Engine events

void AProtoCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float FallSpeed = FMath::Max(LastFallSpeed, -GetVelocity().Z);
	LastFallSpeed = 0.0f;
	FirstPerson->OnLanded(FallSpeed);
	if (FallSpeed > LandingSlowMinFallSpeed)
	{
		LandingSlowRemaining = LandingSlowDuration;
	}
}

void AProtoCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// The capsule centre dropped: raise the camera by the same amount so it can ease down smoothly.
	FirstPerson->OnCrouchHeightChanged(HalfHeightAdjust);
}

void AProtoCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPerson->OnCrouchHeightChanged(-HalfHeightAdjust);
}

// ---------------------------------------------------------------------------------------------------------------------
// Input

void AProtoCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	if (UInputMappingContext* Context = DefaultMappingContext.LoadSynchronous())
	{
		Subsystem->AddMappingContext(Context, MappingPriority);
	}
	else
	{
		UE_LOG(LogProto, Warning, TEXT("Input mapping context %s is missing."), *DefaultMappingContext.ToString());
	}
}

void AProtoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogProto, Error, TEXT("Enhanced Input component missing: check DefaultInput.ini."));
		return;
	}

	auto Load = [](const TSoftObjectPtr<UInputAction>& Soft) -> UInputAction*
	{
		UInputAction* Action = Soft.LoadSynchronous();
		if (!Action)
		{
			UE_LOG(LogProto, Warning, TEXT("Input action %s is missing."), *Soft.ToString());
		}
		return Action;
	};

	if (UInputAction* Action = Load(MoveAction))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AProtoCharacter::OnMove);
	}
	if (UInputAction* Action = Load(LookAction))
	{
		Input->BindAction(Action, ETriggerEvent::Triggered, this, &AProtoCharacter::OnLook);
	}
	if (UInputAction* Action = Load(SprintAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnSprintStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnSprintCompleted);
	}
	if (UInputAction* Action = Load(CrouchAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnCrouchStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnCrouchCompleted);
	}
	if (UInputAction* Action = Load(JumpAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnJumpStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnJumpCompleted);
	}
	if (UInputAction* Action = Load(LeanLeftAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnLeanLeftStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnLeanLeftCompleted);
	}
	if (UInputAction* Action = Load(LeanRightAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnLeanRightStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnLeanRightCompleted);
	}
	if (UInputAction* Action = Load(FireAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnFireStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnFireCompleted);
	}
	if (UInputAction* Action = Load(AimAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnAimStarted);
		Input->BindAction(Action, ETriggerEvent::Completed, this, &AProtoCharacter::OnAimCompleted);
	}
	if (UInputAction* Action = Load(ReloadAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnReload);
	}
	if (UInputAction* Action = Load(FireModeAction))
	{
		Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnFireMode);
	}
	for (const TSoftObjectPtr<UInputAction>& SlotAction : WeaponSlotActions)
	{
		if (UInputAction* Action = Load(SlotAction))
		{
			Input->BindAction(Action, ETriggerEvent::Started, this, &AProtoCharacter::OnWeaponSlot);
		}
	}
}

void AProtoCharacter::OnMove(const FInputActionValue& Value)
{
	if (bDead)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AProtoCharacter::OnLook(const FInputActionValue& Value)
{
	if (!bDead)
	{
		FirstPerson->AddLookInput(Value.Get<FVector2D>());
	}
}

void AProtoCharacter::OnSprintStarted()
{
	bWantsSprint = true;
	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void AProtoCharacter::OnSprintCompleted()
{
	bWantsSprint = false;
}

void AProtoCharacter::OnCrouchStarted()
{
	if (bDead)
	{
		return;
	}
	bWantsSprint = false;
	if (bCrouchToggle && bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AProtoCharacter::OnCrouchCompleted()
{
	if (!bCrouchToggle)
	{
		UnCrouch();
	}
}

void AProtoCharacter::OnJumpStarted()
{
	if (bDead)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch(); // First press stands up.
		return;
	}
	Jump();
}

void AProtoCharacter::OnJumpCompleted()
{
	StopJumping();
}

void AProtoCharacter::OnLeanLeftStarted()
{
	bLeanLeftHeld = true;
}

void AProtoCharacter::OnLeanLeftCompleted()
{
	bLeanLeftHeld = false;
}

void AProtoCharacter::OnLeanRightStarted()
{
	bLeanRightHeld = true;
}

void AProtoCharacter::OnLeanRightCompleted()
{
	bLeanRightHeld = false;
}

void AProtoCharacter::OnFireStarted()
{
	if (bDead)
	{
		return;
	}
	bFireHeld = true;
	bWantsSprint = false; // Firing cancels sprint; the shot goes off once the gun is raised.
	if (AProtoWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->PullTrigger();
	}
}

void AProtoCharacter::OnFireCompleted()
{
	bFireHeld = false;
	if (AProtoWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->ReleaseTrigger();
	}
}

void AProtoCharacter::OnAimStarted()
{
	bAimHeld = true;
}

void AProtoCharacter::OnAimCompleted()
{
	bAimHeld = false;
}

void AProtoCharacter::OnReload()
{
	if (bDead || PendingSlot != INDEX_NONE)
	{
		return;
	}
	if (AProtoWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->StartReload();
	}
}

void AProtoCharacter::OnWeaponSlot(const FInputActionInstance& Instance)
{
	const UInputAction* Pressed = Instance.GetSourceAction();
	for (int32 Slot = 0; Slot < WeaponSlotActions.Num(); ++Slot)
	{
		if (WeaponSlotActions[Slot].Get() == Pressed)
		{
			EquipSlot(Slot);
			return;
		}
	}
}

void AProtoCharacter::OnFireMode()
{
	if (AProtoWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->CycleFireMode();
	}
}
