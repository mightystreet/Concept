// Manually integrated projectile: the sniper bullet (gravity drop, direct damage) and the RPG rocket (radial falloff).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoProjectile.generated.h"

class AProtoWeapon;
class UProtoWeaponData;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;

UCLASS()
class CONCEPT_API AProtoProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProtoProjectile();

	virtual void Tick(float DeltaSeconds) override;

	/** Copies the stats from InData and starts flying along Direction from the current location. */
	void Launch(AProtoWeapon* InWeapon, const UProtoWeaponData* InData, const FVector& Direction);

protected:
	void HandleImpact(const FHitResult& Hit, const FVector& SegmentStart);
	void Explode(const FVector& Location);
	void ApplyExplosionDamage(const FVector& Location);
	void UpdateExplosion(float DeltaSeconds);
	void SetMeshColor(UStaticMeshComponent* Component, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> TrailLight;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ExplosionMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> ExplosionLight;

	// ---- Assets ----
	UPROPERTY(EditAnywhere, Category = "Projectile|Assets")
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(EditAnywhere, Category = "Projectile|Assets")
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(EditAnywhere, Category = "Projectile|Assets")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "Projectile|Assets")
	FName ColorParameterName = TEXT("Color");

	// ---- Look ----
	UPROPERTY(EditAnywhere, Category = "Projectile|Look")
	FLinearColor BulletColor = FLinearColor(1.0f, 0.85f, 0.4f);

	UPROPERTY(EditAnywhere, Category = "Projectile|Look")
	FLinearColor RocketColor = FLinearColor(0.2f, 0.25f, 0.15f);

	UPROPERTY(EditAnywhere, Category = "Projectile|Look")
	FLinearColor TrailLightColor = FLinearColor(1.0f, 0.6f, 0.2f);

	/** Trail light intensity for rockets (bullets have no light). */
	UPROPERTY(EditAnywhere, Category = "Projectile|Look", meta = (ClampMin = "0"))
	float RocketLightIntensity = 20000.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Look", meta = (ClampMin = "0", Units = "cm"))
	float RocketLightRadius = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Look")
	FLinearColor ExplosionColor = FLinearColor(1.0f, 0.45f, 0.1f);

	/** Seconds the explosion sphere takes to expand and fade. */
	UPROPERTY(EditAnywhere, Category = "Projectile|Look", meta = (ClampMin = "0.01", Units = "s"))
	float ExplosionVisualDuration = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Look", meta = (ClampMin = "0"))
	float ExplosionLightIntensity = 400000.0f;

	/** Explosion light radius as a multiple of the blast radius. */
	UPROPERTY(EditAnywhere, Category = "Projectile|Look", meta = (ClampMin = "0"))
	float ExplosionLightRadiusScale = 3.0f;

	/** Explosion is placed this far off the hit surface so the line-of-sight checks start in open air (cm). */
	UPROPERTY(EditAnywhere, Category = "Projectile|Explosion", meta = (ClampMin = "0", Units = "cm"))
	float ExplosionSurfaceOffset = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Debug", meta = (ClampMin = "0", Units = "s"))
	float DebugDrawDuration = 2.0f;

	// ---- State ----
	TWeakObjectPtr<AProtoWeapon> Weapon;
	TWeakObjectPtr<AActor> IgnoredOwner;
	TWeakObjectPtr<AController> InstigatorController;

	EProtoHitMethod HitMethod = EProtoHitMethod::Projectile;
	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.0f;
	float GravityScale = 0.0f;
	float LifeRemaining = 0.0f;
	float ExplosionRadius = 0.0f;
	float ExplosionInnerRadius = 0.0f;
	float ExplosionMinDamageFraction = 0.0f;
	float ExplosionFalloffExponent = 1.0f;
	bool bLaunched = false;
	bool bExploding = false;
	float ExplosionElapsed = 0.0f;
};
