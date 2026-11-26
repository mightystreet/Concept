// Stats for one weapon. Five instances (DA_Pistol ... DA_RPG) live in /Game/Prototype/Data.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoWeaponData.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class CONCEPT_API UProtoWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Seconds between shots from RoundsPerMinute. */
	float GetShotInterval() const { return RoundsPerMinute > 0.0f ? SecondsPerMinute / RoundsPerMinute : 0.0f; }

	static constexpr float SecondsPerMinute = 60.0f;

	// ---- Identity ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EProtoWeaponType WeaponType = EProtoWeaponType::Pistol;

	// ---- Firing ----
	/** Fire modes V cycles through. The first entry is the default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	TArray<EProtoFireMode> FireModes = { EProtoFireMode::Semi };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "1"))
	int32 MagazineSize = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0"))
	float Damage = 30.0f;

	/** Rate of fire. For bolt/single weapons this sets the cycle time between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "1"))
	float RoundsPerMinute = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0", Units = "s"))
	float ReloadTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	EProtoHitMethod HitMethod = EProtoHitMethod::LineTrace;

	/** Line trace range (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "100", Units = "cm"))
	float Range = 20000.0f;

	/** Hearing range of the shot for AI (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0", Units = "cm"))
	float NoiseRange = 6000.0f;

	// ---- Projectile / Rocket ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1"))
	float ProjectileSpeed = 60000.0f;

	/** 1 = normal gravity. Rockets use 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0"))
	float ProjectileGravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.1", Units = "s"))
	float ProjectileLifetime = 5.0f;

	/** Visual size of the projectile mesh (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileVisualSize = FVector(8.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosion", meta = (ClampMin = "0", Units = "cm"))
	float ExplosionRadius = 500.0f;

	/** Full damage inside this radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosion", meta = (ClampMin = "0", Units = "cm"))
	float ExplosionInnerRadius = 100.0f;

	/** Damage fraction at the edge of ExplosionRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosion", meta = (ClampMin = "0", ClampMax = "1"))
	float ExplosionMinDamageFraction = 0.1f;

	/** Falloff curve exponent between inner and outer radius (1 = linear). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Explosion", meta = (ClampMin = "0.1"))
	float ExplosionFalloffExponent = 1.0f;

	// ---- Accuracy ----
	/** Spread cone half-angle from the hip (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accuracy", meta = (ClampMin = "0", Units = "Degrees"))
	float HipSpread = 1.5f;

	/** Spread cone half-angle aimed down sights (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accuracy", meta = (ClampMin = "0", Units = "Degrees"))
	float AimSpread = 0.3f;

	/** Extra spread at full jog speed (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accuracy", meta = (ClampMin = "0", Units = "Degrees"))
	float MovingSpread = 1.0f;

	// ---- Recoil ----
	/** Upward gun kick per shot (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0"))
	float RecoilPitch = 2.0f;

	/** Random sideways kick per shot, +/- (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0"))
	float RecoilYaw = 0.6f;

	/** Backwards kick of the gun per shot (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0"))
	float RecoilKickBack = 3.0f;

	/** Share of the pitch kick that also moves the camera (recovers over time). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0", ClampMax = "1"))
	float RecoilCameraFraction = 0.4f;

	/** Share of the pitch kick added permanently to the view (climb you must pull down). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0", ClampMax = "1"))
	float RecoilClimbFraction = 0.15f;

	/** Recoil recovery speed multiplier (higher = snappier). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.1"))
	float RecoilRecovery = 1.0f;

	// ---- Handling ----
	/** Multiplier on sway, bob and breathing for this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0"))
	float SwayAmount = 1.0f;

	/** Seconds to fully aim down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0.01", Units = "s"))
	float AimTime = 0.15f;

	/** Camera FOV when fully aimed. Sniper uses a low value for its zoomed view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "5", ClampMax = "170", Units = "Degrees"))
	float AimFOV = 90.0f;

	/** Movement speed multiplier while carried (0.85 for heavy weapons). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0.1", ClampMax = "1"))
	float MovementSpeedMultiplier = 1.0f;

	/** Gun position relative to the camera at the hip (cm, X forward, Y right, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling")
	FVector HipOffset = FVector(30.0f, 12.0f, -14.0f);

	/** Gun position relative to the camera when aimed. Z should put the sight line on the camera axis. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling")
	FVector AimOffset = FVector(22.0f, 0.0f, -6.0f);

	// ---- Visuals (block shapes) ----
	/** Receiver block size (cm). The weapon root is the grip; the body extends forward from it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FVector BodySize = FVector(18.0f, 3.0f, 10.0f);

	/** Barrel length (cm) beyond the body. The muzzle is at its tip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0"))
	float BarrelLength = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0.1"))
	float BarrelDiameter = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FLinearColor BodyColor = FLinearColor(0.05f, 0.05f, 0.05f);

	/** Optional placeholder fire sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USoundBase> FireSound;

	/** Optional placeholder reload sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USoundBase> ReloadSound;
};
