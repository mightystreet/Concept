// First-person player character: movement, input, weapon slots and fire gating.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UProtoFirstPersonComponent;
class UProtoHealthComponent;
class AProtoWeapon;
struct FInputActionValue;
struct FInputActionInstance;

UCLASS()
class CONCEPT_API AProtoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AProtoCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	bool IsSprinting() const;
	bool WantsToAim() const;
	bool WantsLowered() const;
	bool IsDead() const { return bDead; }
	float GetLeanInput() const;
	float GetStandingCameraHeight() const { return StandingCameraHeight; }
	float GetCrouchedCameraHeight() const { return CrouchedCameraHeight; }
	float GetJogSpeed() const { return JogSpeed; }

	AProtoWeapon* GetCurrentWeapon() const;
	UProtoHealthComponent* GetHealth() const { return Health; }
	UProtoFirstPersonComponent* GetFirstPerson() const { return FirstPerson; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnWeapons();
	void UpdateMovementSpeed(float DeltaSeconds);
	void UpdateWeaponState();
	void FinishWeaponSwitch();
	void HandleWeaponFired(AProtoWeapon* Weapon);
	void HandleDeath(AController* Killer);

	// ---- Input handlers ----
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSprintStarted();
	void OnSprintCompleted();
	void OnCrouchStarted();
	void OnCrouchCompleted();
	void OnJumpStarted();
	void OnJumpCompleted();
	void OnLeanLeftStarted();
	void OnLeanLeftCompleted();
	void OnLeanRightStarted();
	void OnLeanRightCompleted();
	void OnFireStarted();
	void OnFireCompleted();
	void OnAimStarted();
	void OnAimCompleted();
	void OnReload();
	void OnWeaponSlot(const FInputActionInstance& Instance);
	void OnFireMode();

	void EquipSlot(int32 Slot);

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	/** The active weapon hangs from this; the first-person rig moves it relative to the camera. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProtoFirstPersonComponent> FirstPerson;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProtoHealthComponent> Health;

	// ---- Input assets ----
	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	int32 MappingPriority = 0;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> LeanLeftAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> LeanRightAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> ReloadAction;

	/** Index = weapon slot (0 = key 1 = Pistol ... 4 = key 5 = RPG). */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<TSoftObjectPtr<UInputAction>> WeaponSlotActions;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> FireModeAction;

	/** Ctrl toggles crouch when true, holds crouch when false. */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bCrouchToggle = true;

	// ---- Movement ----
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float CrouchSpeed = 150.0f;

	/** Used while aiming down sights (there is no separate walk key). */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float JogSpeed = 420.0f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float SprintSpeed = 620.0f;

	/** Sprint needs the move input this close to forward (dot product). */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float SprintForwardThreshold = 0.6f;

	/** How fast the speed cap changes between states (cm/s per second), so sprint start/stop is not instant. */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float SpeedCapBlendRate = 900.0f;

	/** Fall speeds above this cause a landing slowdown (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Movement|Landing", meta = (ClampMin = "0"))
	float LandingSlowMinFallSpeed = 350.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "s"))
	float LandingSlowDuration = 0.35f;

	/** Speed multiplier right after a hard landing; recovers linearly over LandingSlowDuration. */
	UPROPERTY(EditAnywhere, Category = "Movement|Landing", meta = (ClampMin = "0", ClampMax = "1"))
	float LandingSlowMultiplier = 0.5f;

	/** Camera height above the capsule centre when standing (cm). Also used as BaseEyeHeight. */
	UPROPERTY(EditAnywhere, Category = "Movement|Eyes")
	float StandingCameraHeight = 64.0f;

	/** Camera height above the crouched capsule centre (cm). Also used as CrouchedEyeHeight. */
	UPROPERTY(EditAnywhere, Category = "Movement|Eyes")
	float CrouchedCameraHeight = 38.0f;

	// ---- Weapons ----
	UPROPERTY(EditAnywhere, Category = "Weapons")
	EProtoWeaponType StartingWeapon = EProtoWeaponType::Rifle;

	UPROPERTY(EditAnywhere, Category = "Weapons")
	TSubclassOf<AProtoWeapon> WeaponClass;

	/** Fire is blocked while the gun is lowered more than this. */
	UPROPERTY(EditAnywhere, Category = "Weapons", meta = (ClampMin = "0", ClampMax = "1"))
	float FireBlockLoweredAlpha = 0.15f;

	/** The weapon swap happens once the old gun is lowered this far. */
	UPROPERTY(EditAnywhere, Category = "Weapons", meta = (ClampMin = "0", ClampMax = "1"))
	float SwitchSwapLoweredAlpha = 0.98f;

	// ---- State ----
	UPROPERTY(Transient)
	TArray<TObjectPtr<AProtoWeapon>> Weapons;

	int32 CurrentSlot = INDEX_NONE;
	int32 PendingSlot = INDEX_NONE;
	bool bWantsSprint = false;
	bool bAimHeld = false;
	bool bFireHeld = false;
	bool bLeanLeftHeld = false;
	bool bLeanRightHeld = false;
	bool bFireWasBlocked = false;
	bool bDead = false;
	float SpeedCap = 0.0f;
	float LandingSlowRemaining = 0.0f;
	float LastFallSpeed = 0.0f;
};
