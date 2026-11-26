#include "Proto/Weapons/ProtoProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Weapons/ProtoWeapon.h"
#include "Proto/Weapons/ProtoWeaponData.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Engine basic shapes are 100 cm across. */
	constexpr float BasicShapeSize = 100.0f;
}

AProtoProjectile::AProtoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	SphereMesh = SphereFinder.Object;
	BaseMaterial = MaterialFinder.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetStaticMesh(CubeMesh);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCastShadow(false);

	TrailLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TrailLight"));
	TrailLight->SetupAttachment(Root);
	TrailLight->SetIntensityUnits(ELightUnits::Unitless);
	TrailLight->SetCastShadows(false);
	TrailLight->SetVisibility(false);

	ExplosionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExplosionMesh"));
	ExplosionMesh->SetupAttachment(Root);
	ExplosionMesh->SetStaticMesh(SphereMesh);
	ExplosionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExplosionMesh->SetCastShadow(false);
	ExplosionMesh->SetVisibility(false);

	ExplosionLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ExplosionLight"));
	ExplosionLight->SetupAttachment(Root);
	ExplosionLight->SetIntensityUnits(ELightUnits::Unitless);
	ExplosionLight->SetCastShadows(false);
	ExplosionLight->SetVisibility(false);
}

void AProtoProjectile::SetMeshColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (Component && BaseMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = Component->CreateDynamicMaterialInstance(0, BaseMaterial))
		{
			Mid->SetVectorParameterValue(ColorParameterName, Color);
		}
	}
}

void AProtoProjectile::Launch(AProtoWeapon* InWeapon, const UProtoWeaponData* InData, const FVector& Direction)
{
	if (!InData)
	{
		Destroy();
		return;
	}

	Weapon = InWeapon;
	IgnoredOwner = InWeapon ? InWeapon->GetOwner() : nullptr;
	InstigatorController = InWeapon ? InWeapon->GetInstigatorController() : nullptr;

	HitMethod = InData->HitMethod;
	Velocity = Direction.GetSafeNormal() * InData->ProjectileSpeed;
	Damage = InData->Damage;
	GravityScale = InData->ProjectileGravityScale;
	LifeRemaining = InData->ProjectileLifetime;
	ExplosionRadius = InData->ExplosionRadius;
	ExplosionInnerRadius = FMath::Min(InData->ExplosionInnerRadius, InData->ExplosionRadius);
	ExplosionMinDamageFraction = InData->ExplosionMinDamageFraction;
	ExplosionFalloffExponent = InData->ExplosionFalloffExponent;

	BodyMesh->SetRelativeScale3D(InData->ProjectileVisualSize / BasicShapeSize);
	const bool bRocket = HitMethod == EProtoHitMethod::Rocket;
	SetMeshColor(BodyMesh, bRocket ? RocketColor : BulletColor);
	SetMeshColor(ExplosionMesh, ExplosionColor);

	TrailLight->SetVisibility(bRocket);
	TrailLight->SetIntensity(RocketLightIntensity);
	TrailLight->SetAttenuationRadius(RocketLightRadius);
	TrailLight->SetLightColor(TrailLightColor);

	SetActorRotation(Velocity.Rotation());
	bLaunched = true;
}

void AProtoProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bExploding)
	{
		UpdateExplosion(DeltaSeconds);
		return;
	}
	if (!bLaunched)
	{
		return;
	}

	LifeRemaining -= DeltaSeconds;
	if (LifeRemaining <= 0.0f)
	{
		if (HitMethod == EProtoHitMethod::Rocket)
		{
			Explode(GetActorLocation());
		}
		else
		{
			Destroy();
		}
		return;
	}

	const FVector Start = GetActorLocation();
	Velocity.Z += GetWorld()->GetGravityZ() * GravityScale * DeltaSeconds;
	const FVector End = Start + Velocity * DeltaSeconds;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProtoProjectile), true, this);
	if (AProtoWeapon* OwningWeapon = Weapon.Get())
	{
		OwningWeapon->AddIgnoredActors(Params);
	}
	else if (AActor* OwnerActor = IgnoredOwner.Get())
	{
		Params.AddIgnoredActor(OwnerActor);
	}
	Params.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_ProtoWeapon, Params);

	if (UProtoDebugSubsystem::IsDebugDrawEnabled(this))
	{
		DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End, FColor::Orange, false, DebugDrawDuration);
	}

	if (bHit)
	{
		HandleImpact(Hit, Start);
		return;
	}

	SetActorLocationAndRotation(End, Velocity.Rotation());
}

void AProtoProjectile::HandleImpact(const FHitResult& Hit, const FVector& SegmentStart)
{
	if (HitMethod == EProtoHitMethod::Rocket)
	{
		Explode(Hit.ImpactPoint + Hit.ImpactNormal * ExplosionSurfaceOffset);
		return;
	}

	if (AProtoWeapon* OwningWeapon = Weapon.Get())
	{
		OwningWeapon->HandleBulletHit(Hit, SegmentStart, Damage, false);
	}
	else
	{
		AProtoWeapon::ApplyHitDamage(Hit, Damage, this, InstigatorController.Get());
	}
	Destroy();
}

void AProtoProjectile::Explode(const FVector& Location)
{
	bExploding = true;
	ExplosionElapsed = 0.0f;
	SetActorLocation(Location);

	BodyMesh->SetVisibility(false);
	TrailLight->SetVisibility(false);
	ExplosionMesh->SetVisibility(true);
	ExplosionMesh->SetRelativeScale3D(FVector::ZeroVector);
	ExplosionLight->SetVisibility(true);
	ExplosionLight->SetLightColor(ExplosionColor);
	ExplosionLight->SetAttenuationRadius(ExplosionRadius * ExplosionLightRadiusScale);
	ExplosionLight->SetIntensity(ExplosionLightIntensity);

	ApplyExplosionDamage(Location);

	if (UProtoDebugSubsystem::IsDebugDrawEnabled(this))
	{
		constexpr int32 DebugSphereSegments = 16;
		DrawDebugSphere(GetWorld(), Location, ExplosionInnerRadius, DebugSphereSegments, FColor::Red, false, DebugDrawDuration);
		DrawDebugSphere(GetWorld(), Location, ExplosionRadius, DebugSphereSegments, FColor::Yellow, false, DebugDrawDuration);
	}
}

void AProtoProjectile::ApplyExplosionDamage(const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World || ExplosionRadius <= 0.0f)
	{
		return;
	}

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		UProtoHealthComponent* Health = UProtoHealthComponent::FindOn(Pawn);
		if (!Health || Health->IsDead())
		{
			continue;
		}

		const FVector Target = Pawn->GetActorLocation();
		const float Distance = FVector::Dist(Location, Target);
		if (Distance > ExplosionRadius)
		{
			continue;
		}

		// Line of sight from the blast to the pawn: walls block the explosion.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ProtoExplosionLOS), false, this);
		Params.AddIgnoredActor(Pawn);
		TArray<AActor*> Attached;
		Pawn->GetAttachedActors(Attached);
		Params.AddIgnoredActors(Attached);
		if (World->LineTraceTestByChannel(Location, Target, ECC_Visibility, Params))
		{
			continue;
		}

		float Fraction = 1.0f;
		if (Distance > ExplosionInnerRadius)
		{
			const float Range = FMath::Max(ExplosionRadius - ExplosionInnerRadius, UE_KINDA_SMALL_NUMBER);
			const float T = FMath::Clamp((Distance - ExplosionInnerRadius) / Range, 0.0f, 1.0f);
			Fraction = FMath::Lerp(1.0f, ExplosionMinDamageFraction, FMath::Pow(T, ExplosionFalloffExponent));
		}

		Health->ApplyDamage(Damage * Fraction, false, this, InstigatorController.Get());
	}
}

void AProtoProjectile::UpdateExplosion(float DeltaSeconds)
{
	ExplosionElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(ExplosionElapsed / ExplosionVisualDuration, 0.0f, 1.0f);

	const float Diameter = ExplosionRadius * 2.0f * FMath::Sin(Alpha * HALF_PI);
	ExplosionMesh->SetRelativeScale3D(FVector(Diameter / BasicShapeSize));
	ExplosionLight->SetIntensity(ExplosionLightIntensity * (1.0f - Alpha));

	if (Alpha >= 1.0f)
	{
		Destroy();
	}
}
