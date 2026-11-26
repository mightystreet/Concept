// One weapon actor class for every gun, used by the player and enemies. Built from engine basic shapes.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoWeapon.generated.h"

class UProtoWeaponData;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AProtoProjectile;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnProtoWeaponFired, AProtoWeapon* /*Weapon*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnProtoWeaponReload, AProtoWeapon* /*Weapon*/);

UCLASS()
class CONCEPT_API AProtoWeapon : public AActor
{
	GENERATED_BODY()

public:
	AProtoWeapon();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** Sets the stats and rebuilds the block visuals. Fills the magazine. */
	void Initialize(UProtoWeaponData* InData);

	/** Pulls the trigger. Fires immediately if possible; auto weapons keep firing while held. */
	void PullTrigger();
	void ReleaseTrigger();

	/** Starts a reload if the magazine is not full. Returns true if a reload started. */
	bool StartReload();
	void CancelReload();

	/** Cycles to the next fire mode in the data asset. */
	void CycleFireMode();

	/** Selects a fire mode if the weapon supports it. Returns true on success. */
	bool SetFireMode(EProtoFireMode Mode);

	/** Spread cone half-angle (degrees) used for the next shots. Set every frame by the owner. */
	void SetSpread(float InSpreadDegrees) { SpreadDegrees = InSpreadDegrees; }

	/** While true the weapon will not fire (sprinting, lowered, switching). Releases the trigger. */
	void SetFireBlocked(bool bBlocked);

	/** Shows or hides the scope glint / wind-up warning light. */
	void SetGlint(bool bVisible, const FLinearColor& Color);

	/** Spawns an optional tracer and an impact mark and applies damage for a bullet hit. Shared with projectiles. */
	void HandleBulletHit(const FHitResult& Hit, const FVector& TracerStart, float Damage, bool bSpawnTracer);

	/** Fills Params with this weapon, its owner and everything attached to the owner. */
	void AddIgnoredActors(FCollisionQueryParams& Params) const;

	/** Applies damage to whatever Hit landed on, with headshot detection. */
	static void ApplyHitDamage(const FHitResult& Hit, float Damage, AActor* DamageCauser, AController* InstigatedBy);

	bool CanFire() const;
	bool IsReloading() const { return bReloading; }
	float GetReloadProgress() const;
	int32 GetAmmo() const { return CurrentAmmo; }
	int32 GetMagazineSize() const;
	EProtoFireMode GetFireMode() const;
	UProtoWeaponData* GetData() const { return Data; }
	USceneComponent* GetMuzzle() const { return Muzzle; }
	bool IsTriggerHeld() const { return bTriggerHeld; }
	AController* GetInstigatorController() const;

	FOnProtoWeaponFired OnFired;
	FOnProtoWeaponReload OnReloadStarted;

protected:
	void FireShot();
	void FinishReload();
	void SpawnTracer(const FVector& Start, const FVector& End);
	void SpawnImpactMark(const FHitResult& Hit);
	void UpdateMuzzleFlash(float DeltaSeconds);
	void UpdateGlint(float DeltaSeconds);
	UStaticMeshComponent* CreateShapeComponent(FName Name, UStaticMesh* Mesh);
	void SetShapeColor(UStaticMeshComponent* Component, const FLinearColor& Color);

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GripMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MuzzleFlashMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> MuzzleLight;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GlintMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> GlintLight;

	// ---- Assets ----
	UPROPERTY(EditAnywhere, Category = "Weapon|Assets")
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(EditAnywhere, Category = "Weapon|Assets")
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(EditAnywhere, Category = "Weapon|Assets")
	TObjectPtr<UStaticMesh> SphereMesh;

	/** Engine BasicShapeMaterial; tinted through its "Color" parameter. */
	UPROPERTY(EditAnywhere, Category = "Weapon|Assets")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "Weapon|Assets")
	FName ColorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, Category = "Weapon|Projectile")
	TSubclassOf<AProtoProjectile> ProjectileClass;

	// ---- Shape ----
	UPROPERTY(EditAnywhere, Category = "Weapon|Shape")
	FVector GripSize = FVector(4.0f, 2.5f, 9.0f);

	/** Barrel height as a fraction of body height above the body centre. */
	UPROPERTY(EditAnywhere, Category = "Weapon|Shape", meta = (ClampMin = "-0.5", ClampMax = "0.5"))
	float BarrelHeightFraction = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Shape")
	FLinearColor GripColor = FLinearColor(0.08f, 0.06f, 0.04f);

	// ---- Effects ----
	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "s"))
	float MuzzleFlashDuration = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float MuzzleFlashSize = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	FLinearColor MuzzleFlashColor = FLinearColor(1.0f, 0.7f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0"))
	float MuzzleLightIntensity = 30000.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float MuzzleLightRadius = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "s"))
	float TracerDuration = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float TracerThickness = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	FLinearColor TracerColor = FLinearColor(1.0f, 0.85f, 0.4f);

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float ImpactMarkSize = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float ImpactMarkDepth = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "s"))
	float ImpactMarkLifetime = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects")
	FLinearColor ImpactMarkColor = FLinearColor(0.01f, 0.01f, 0.01f);

	/** Scope glint size (cm). Large so it reads at 60 m+. */
	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float GlintSize = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0"))
	float GlintLightIntensity = 50000.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", Units = "cm"))
	float GlintLightRadius = 300.0f;

	/** Glint pulses per second. */
	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0"))
	float GlintPulseRate = 3.0f;

	/** Lowest glint scale during a pulse (fraction of GlintSize). */
	UPROPERTY(EditAnywhere, Category = "Weapon|Effects", meta = (ClampMin = "0", ClampMax = "1"))
	float GlintPulseMinScale = 0.5f;

	/** Noise tag reported to AI hearing on every shot. */
	UPROPERTY(EditAnywhere, Category = "Weapon|AI")
	FName NoiseTag = TEXT("Gunshot");

	/** Noise loudness reported to AI hearing on every shot. */
	UPROPERTY(EditAnywhere, Category = "Weapon|AI", meta = (ClampMin = "0"))
	float NoiseLoudness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Debug", meta = (ClampMin = "0", Units = "s"))
	float DebugTraceDuration = 2.0f;

	// ---- State ----
	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|State")
	TObjectPtr<UProtoWeaponData> Data;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TracerMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ImpactMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlintMaterial;

	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|State")
	int32 CurrentAmmo = 0;

	int32 FireModeIndex = 0;
	bool bTriggerHeld = false;
	/** A semi/bolt/single press waiting for the cycle time to finish. */
	bool bShotQueued = false;
	bool bFireBlocked = false;
	bool bReloading = false;
	bool bGlintVisible = false;
	float FireCooldown = 0.0f;
	float ReloadElapsed = 0.0f;
	float MuzzleFlashRemaining = 0.0f;
	float GlintTime = 0.0f;
	float SpreadDegrees = 0.0f;
};
