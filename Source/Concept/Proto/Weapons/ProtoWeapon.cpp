#include "Proto/Weapons/ProtoWeapon.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Perception/AISense_Hearing.h"
#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Core/ProtoLog.h"
#include "Proto/Weapons/ProtoProjectile.h"
#include "Proto/Weapons/ProtoWeaponData.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Engine basic shapes are 100 cm across. */
	constexpr float BasicShapeSize = 100.0f;
	/** Rotates a basic-shape cylinder (Z axis) so it points along X. */
	const FRotator CylinderAlongX(90.0f, 0.0f, 0.0f);
}

AProtoWeapon::AProtoWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	SphereMesh = SphereFinder.Object;
	BaseMaterial = MaterialFinder.Object;
	ProjectileClass = AProtoProjectile::StaticClass();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateShapeComponent(TEXT("BodyMesh"), CubeMesh);
	GripMesh = CreateShapeComponent(TEXT("GripMesh"), CubeMesh);
	BarrelMesh = CreateShapeComponent(TEXT("BarrelMesh"), CylinderMesh);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Root);

	MuzzleFlashMesh = CreateShapeComponent(TEXT("MuzzleFlashMesh"), SphereMesh);
	MuzzleFlashMesh->SetupAttachment(Muzzle);
	MuzzleFlashMesh->SetVisibility(false);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Muzzle);
	MuzzleLight->SetIntensityUnits(ELightUnits::Unitless);
	MuzzleLight->SetCastShadows(false);
	MuzzleLight->SetVisibility(false);

	GlintMesh = CreateShapeComponent(TEXT("GlintMesh"), SphereMesh);
	GlintMesh->SetVisibility(false);

	GlintLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlintLight"));
	GlintLight->SetupAttachment(GlintMesh);
	GlintLight->SetIntensityUnits(ELightUnits::Unitless);
	GlintLight->SetCastShadows(false);
	GlintLight->SetVisibility(false);
}

UStaticMeshComponent* AProtoWeapon::CreateShapeComponent(FName Name, UStaticMesh* Mesh)
{
	UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Component->SetupAttachment(Root);
	Component->SetStaticMesh(Mesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCastShadow(false);
	return Component;
}

void AProtoWeapon::SetShapeColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (Component && BaseMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = Component->CreateDynamicMaterialInstance(0, BaseMaterial))
		{
			Mid->SetVectorParameterValue(ColorParameterName, Color);
		}
	}
}

void AProtoWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMaterial)
	{
		TracerMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		TracerMaterial->SetVectorParameterValue(ColorParameterName, TracerColor);
		ImpactMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		ImpactMaterial->SetVectorParameterValue(ColorParameterName, ImpactMarkColor);
	}
	SetShapeColor(MuzzleFlashMesh, MuzzleFlashColor);
	GlintMaterial = GlintMesh->CreateDynamicMaterialInstance(0, BaseMaterial);
}

void AProtoWeapon::Initialize(UProtoWeaponData* InData)
{
	Data = InData;
	if (!Data)
	{
		UE_LOG(LogProto, Warning, TEXT("%s: Initialize called with no weapon data."), *GetName());
		return;
	}

	CurrentAmmo = Data->MagazineSize;
	FireModeIndex = 0;
	FireCooldown = 0.0f;
	bReloading = false;
	bShotQueued = false;
	bTriggerHeld = false;

	// Layout: root sits at the rear of the body centre line; the body extends forward along X.
	const FVector Body = Data->BodySize;
	BodyMesh->SetRelativeLocation(FVector(Body.X * 0.5f, 0.0f, 0.0f));
	BodyMesh->SetRelativeScale3D(Body / BasicShapeSize);

	GripMesh->SetRelativeLocation(FVector(GripSize.X, 0.0f, -(Body.Z + GripSize.Z) * 0.5f));
	GripMesh->SetRelativeScale3D(GripSize / BasicShapeSize);

	const float BarrelZ = Body.Z * BarrelHeightFraction;
	BarrelMesh->SetRelativeLocation(FVector(Body.X + Data->BarrelLength * 0.5f, 0.0f, BarrelZ));
	BarrelMesh->SetRelativeRotation(CylinderAlongX);
	BarrelMesh->SetRelativeScale3D(FVector(Data->BarrelDiameter, Data->BarrelDiameter, Data->BarrelLength) / BasicShapeSize);
	BarrelMesh->SetVisibility(Data->BarrelLength > 0.0f);

	Muzzle->SetRelativeLocation(FVector(Body.X + Data->BarrelLength, 0.0f, BarrelZ));

	MuzzleFlashMesh->SetRelativeScale3D(FVector(MuzzleFlashSize / BasicShapeSize));
	MuzzleLight->SetIntensity(MuzzleLightIntensity);
	MuzzleLight->SetAttenuationRadius(MuzzleLightRadius);
	MuzzleLight->SetLightColor(MuzzleFlashColor);

	// Glint / wind-up light sits on top of the front of the body (the "scope").
	GlintMesh->SetRelativeLocation(FVector(Body.X * 0.75f, 0.0f, Body.Z * 0.5f));
	GlintMesh->SetRelativeScale3D(FVector(GlintSize / BasicShapeSize));
	GlintLight->SetIntensity(GlintLightIntensity);
	GlintLight->SetAttenuationRadius(GlintLightRadius);

	SetShapeColor(BodyMesh, Data->BodyColor);
	SetShapeColor(BarrelMesh, Data->BodyColor);
	SetShapeColor(GripMesh, GripColor);
}

void AProtoWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FireCooldown = FMath::Max(FireCooldown - DeltaSeconds, 0.0f);

	if (bReloading && Data)
	{
		ReloadElapsed += DeltaSeconds;
		if (ReloadElapsed >= Data->ReloadTime)
		{
			FinishReload();
		}
	}

	if (CanFire())
	{
		const bool bAuto = GetFireMode() == EProtoFireMode::Auto;
		if (bShotQueued || (bAuto && bTriggerHeld))
		{
			bShotQueued = false;
			FireShot();
		}
	}

	UpdateMuzzleFlash(DeltaSeconds);
	UpdateGlint(DeltaSeconds);
}

void AProtoWeapon::PullTrigger()
{
	if (!Data || bFireBlocked)
	{
		return;
	}

	bTriggerHeld = true;

	if (CurrentAmmo <= 0)
	{
		StartReload();
		return;
	}

	if (CanFire())
	{
		FireShot();
	}
	else if (GetFireMode() != EProtoFireMode::Auto && !bReloading)
	{
		// Pressed during the cycle time: fire as soon as the gun is ready.
		bShotQueued = true;
	}
}

void AProtoWeapon::ReleaseTrigger()
{
	bTriggerHeld = false;
}

bool AProtoWeapon::CanFire() const
{
	return Data && !bReloading && !bFireBlocked && FireCooldown <= 0.0f && CurrentAmmo > 0;
}

void AProtoWeapon::SetFireBlocked(bool bBlocked)
{
	bFireBlocked = bBlocked;
	if (bBlocked)
	{
		bTriggerHeld = false;
		bShotQueued = false;
	}
}

bool AProtoWeapon::StartReload()
{
	if (!Data || bReloading || CurrentAmmo >= Data->MagazineSize)
	{
		return false;
	}

	bReloading = true;
	bShotQueued = false;
	ReloadElapsed = 0.0f;
	if (Data->ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Data->ReloadSound, GetActorLocation());
	}
	OnReloadStarted.Broadcast(this);
	return true;
}

void AProtoWeapon::CancelReload()
{
	bReloading = false;
	ReloadElapsed = 0.0f;
}

void AProtoWeapon::FinishReload()
{
	bReloading = false;
	ReloadElapsed = 0.0f;
	CurrentAmmo = Data ? Data->MagazineSize : 0;
}

float AProtoWeapon::GetReloadProgress() const
{
	if (!bReloading || !Data || Data->ReloadTime <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(ReloadElapsed / Data->ReloadTime, 0.0f, 1.0f);
}

int32 AProtoWeapon::GetMagazineSize() const
{
	return Data ? Data->MagazineSize : 0;
}

EProtoFireMode AProtoWeapon::GetFireMode() const
{
	if (Data && Data->FireModes.IsValidIndex(FireModeIndex))
	{
		return Data->FireModes[FireModeIndex];
	}
	return EProtoFireMode::Semi;
}

void AProtoWeapon::CycleFireMode()
{
	if (Data && Data->FireModes.Num() > 1)
	{
		FireModeIndex = (FireModeIndex + 1) % Data->FireModes.Num();
		bShotQueued = false;
	}
}

bool AProtoWeapon::SetFireMode(EProtoFireMode Mode)
{
	if (!Data)
	{
		return false;
	}
	const int32 Index = Data->FireModes.Find(Mode);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	FireModeIndex = Index;
	return true;
}

AController* AProtoWeapon::GetInstigatorController() const
{
	const APawn* InstigatorPawn = GetInstigator();
	return InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
}

void AProtoWeapon::AddIgnoredActors(FCollisionQueryParams& Params) const
{
	Params.AddIgnoredActor(this);
	if (AActor* OwnerActor = GetOwner())
	{
		Params.AddIgnoredActor(OwnerActor);
		TArray<AActor*> Attached;
		OwnerActor->GetAttachedActors(Attached, true, true);
		Params.AddIgnoredActors(Attached);
	}
}

void AProtoWeapon::FireShot()
{
	UWorld* World = GetWorld();
	if (!World || !Data)
	{
		return;
	}

	--CurrentAmmo;
	FireCooldown = Data->GetShotInterval();

	const FVector MuzzleLocation = Muzzle->GetComponentLocation();
	const FVector Aim = Muzzle->GetForwardVector();
	const FVector Direction = SpreadDegrees > 0.0f
		? FMath::VRandCone(Aim, FMath::DegreesToRadians(SpreadDegrees))
		: Aim;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProtoWeaponFire), true);
	AddIgnoredActors(Params);

	const bool bDebug = UProtoDebugSubsystem::IsDebugDrawEnabled(this);

	// Stop shots passing through walls when the muzzle pokes through: trace from the owner's eyes to the muzzle first.
	FVector ShotStart = MuzzleLocation;
	bool bMuzzleBlocked = false;
	FHitResult BlockHit;
	if (const AActor* OwnerActor = GetOwner())
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		OwnerActor->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		bMuzzleBlocked = World->LineTraceSingleByChannel(BlockHit, EyeLocation, MuzzleLocation, ECC_ProtoWeapon, Params);
	}

	if (bMuzzleBlocked)
	{
		if (Data->HitMethod == EProtoHitMethod::LineTrace)
		{
			HandleBulletHit(BlockHit, BlockHit.TraceStart, Data->Damage, false);
		}
		else
		{
			// Projectiles start just in front of the wall and hit it straight away.
			ShotStart = BlockHit.ImpactPoint + BlockHit.ImpactNormal;
		}
	}

	if (Data->HitMethod == EProtoHitMethod::LineTrace)
	{
		if (!bMuzzleBlocked)
		{
			const FVector End = ShotStart + Direction * Data->Range;
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, ShotStart, End, ECC_ProtoWeapon, Params))
			{
				HandleBulletHit(Hit, ShotStart, Data->Damage, true);
			}
			else
			{
				SpawnTracer(ShotStart, End);
			}
			if (bDebug)
			{
				DrawDebugLine(World, ShotStart, Hit.bBlockingHit ? Hit.ImpactPoint : End, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, DebugTraceDuration);
			}
		}
	}
	else if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AProtoProjectile* Projectile = World->SpawnActor<AProtoProjectile>(ProjectileClass, ShotStart, Direction.Rotation(), SpawnParams))
		{
			Projectile->Launch(this, Data, Direction);
		}
	}

	MuzzleFlashRemaining = MuzzleFlashDuration;
	if (Data->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Data->FireSound, MuzzleLocation);
	}
	UAISense_Hearing::ReportNoiseEvent(this, MuzzleLocation, NoiseLoudness, GetInstigator(), Data->NoiseRange, NoiseTag);

	OnFired.Broadcast(this);
}

void AProtoWeapon::HandleBulletHit(const FHitResult& Hit, const FVector& TracerStart, float Damage, bool bSpawnTracer)
{
	if (bSpawnTracer)
	{
		SpawnTracer(TracerStart, Hit.ImpactPoint);
	}

	// No marks on things that can die (they move, and ragdolls would leave floating marks).
	if (!UProtoHealthComponent::FindOn(Hit.GetActor()))
	{
		SpawnImpactMark(Hit);
	}

	ApplyHitDamage(Hit, Damage, this, GetInstigatorController());
}

void AProtoWeapon::ApplyHitDamage(const FHitResult& Hit, float Damage, AActor* DamageCauser, AController* InstigatedBy)
{
	if (UProtoHealthComponent* Health = UProtoHealthComponent::FindOn(Hit.GetActor()))
	{
		Health->ApplyDamage(Damage, Health->IsHeadshot(Hit), DamageCauser, InstigatedBy);
	}
}

void AProtoWeapon::SpawnTracer(const FVector& Start, const FVector& End)
{
	UWorld* World = GetWorld();
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (!World || !CubeMesh || Length <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Tracer = World->SpawnActor<AStaticMeshActor>(Start + Delta * 0.5f, Delta.Rotation(), SpawnParams);
	if (!Tracer)
	{
		return;
	}

	UStaticMeshComponent* Mesh = Tracer->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(CubeMesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetMaterial(0, TracerMaterial);
	Tracer->SetActorScale3D(FVector(Length, TracerThickness, TracerThickness) / BasicShapeSize);
	Tracer->SetLifeSpan(TracerDuration);
}

void AProtoWeapon::SpawnImpactMark(const FHitResult& Hit)
{
	UWorld* World = GetWorld();
	if (!World || !CubeMesh)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Mark = World->SpawnActor<AStaticMeshActor>(Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), SpawnParams);
	if (!Mark)
	{
		return;
	}

	UStaticMeshComponent* Mesh = Mark->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(CubeMesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetMaterial(0, ImpactMaterial);
	Mark->SetActorScale3D(FVector(ImpactMarkDepth, ImpactMarkSize, ImpactMarkSize) / BasicShapeSize);
	Mark->SetLifeSpan(ImpactMarkLifetime);
}

void AProtoWeapon::UpdateMuzzleFlash(float DeltaSeconds)
{
	const bool bWasVisible = MuzzleFlashMesh->IsVisible();
	const bool bVisible = MuzzleFlashRemaining > 0.0f;
	MuzzleFlashRemaining = FMath::Max(MuzzleFlashRemaining - DeltaSeconds, 0.0f);

	if (bVisible != bWasVisible)
	{
		MuzzleFlashMesh->SetVisibility(bVisible);
		MuzzleLight->SetVisibility(bVisible);
	}
	if (bVisible)
	{
		constexpr float FullTurn = 360.0f;
		MuzzleFlashMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, FMath::FRandRange(0.0f, FullTurn)));
	}
}

void AProtoWeapon::SetGlint(bool bVisible, const FLinearColor& Color)
{
	if (bVisible && !bGlintVisible)
	{
		GlintTime = 0.0f;
	}
	bGlintVisible = bVisible;
	GlintMesh->SetVisibility(bVisible);
	GlintLight->SetVisibility(bVisible);
	if (bVisible)
	{
		GlintLight->SetLightColor(Color);
		if (GlintMaterial)
		{
			GlintMaterial->SetVectorParameterValue(ColorParameterName, Color);
		}
	}
}

void AProtoWeapon::UpdateGlint(float DeltaSeconds)
{
	if (!bGlintVisible)
	{
		return;
	}
	GlintTime += DeltaSeconds;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(GlintTime * GlintPulseRate * UE_TWO_PI);
	const float Scale = FMath::Lerp(GlintPulseMinScale, 1.0f, Pulse);
	GlintMesh->SetRelativeScale3D(FVector(GlintSize * Scale / BasicShapeSize));
	GlintLight->SetIntensity(GlintLightIntensity * Scale);
}
