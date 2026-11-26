#include "Proto/AI/ProtoEnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Proto/AI/ProtoAIController.h"
#include "Proto/Core/ProtoGameMode.h"
#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Core/ProtoLog.h"
#include "Proto/Weapons/ProtoWeapon.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Engine basic shapes are 100 cm across. */
	constexpr float BasicShapeSize = 100.0f;
}

AProtoEnemyCharacter::AProtoEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = MaterialFinder.Object;

	BodyMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
	BodyAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")));
	WeaponClass = AProtoWeapon::StaticClass();

	Health = CreateDefaultSubobject<UProtoHealthComponent>(TEXT("Health"));

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(GetCapsuleComponent());
	WeaponRoot->SetRelativeLocation(WeaponRootOffset);

	FallbackBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackBody"));
	FallbackBody->SetupAttachment(GetCapsuleComponent());
	FallbackBody->SetStaticMesh(CylinderFinder.Object);
	FallbackBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackBody->SetCanEverAffectNavigation(false);
	FallbackBody->SetVisibility(false);

	FallbackHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackHead"));
	FallbackHead->SetupAttachment(FallbackBody);
	FallbackHead->SetStaticMesh(CubeFinder.Object);
	FallbackHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackHead->SetCanEverAffectNavigation(false);
	FallbackHead->SetVisibility(false);
	FallbackHead->SetAbsolute(false, false, true);

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()), MeshRotation);
	GetMesh()->SetCanEverAffectNavigation(false);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, TurnRate, 0.0f);
	Movement->MaxWalkSpeed = MoveSpeed;

	AIControllerClass = AProtoAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AProtoEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	WeaponRoot->SetRelativeLocation(WeaponRootOffset);
	GetCharacterMovement()->RotationRate = FRotator(0.0f, TurnRate, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	SetupBody();
	SpawnWeapon();
	Health->OnDeath.AddUObject(this, &AProtoEnemyCharacter::HandleDeath);
}

void AProtoEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AProtoEnemyCharacter::SetupBody()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	USkeletalMesh* Skeletal = BodyMesh.LoadSynchronous();
	UClass* AnimClass = BodyAnimClass.LoadSynchronous();
	bUsingMannequin = Skeletal && AnimClass;

	if (bUsingMannequin)
	{
		USkeletalMeshComponent* MeshComponent = GetMesh();
		MeshComponent->SetSkeletalMesh(Skeletal);
		MeshComponent->SetAnimInstanceClass(AnimClass);
		MeshComponent->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -Capsule->GetUnscaledCapsuleHalfHeight()), MeshRotation);
		// Bullets hit the physics-asset bodies (so the head bone counts) instead of the capsule.
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComponent->SetCollisionResponseToChannel(ECC_ProtoWeapon, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_ProtoWeapon, ECR_Ignore);
		return;
	}

	UE_LOG(LogProto, Warning, TEXT("%s: mannequin mesh or anim class missing, using the block fallback body."), *GetName());

	const float Radius = Capsule->GetUnscaledCapsuleRadius();
	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const float BodyHeight = HalfHeight * 2.0f - FallbackHeadSize;

	GetMesh()->SetVisibility(false);
	FallbackBody->SetVisibility(true);
	FallbackHead->SetVisibility(true);
	FallbackBody->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeight + BodyHeight * 0.5f));
	FallbackBody->SetRelativeScale3D(FVector(Radius * 2.0f, Radius * 2.0f, BodyHeight) / BasicShapeSize);
	FallbackHead->SetWorldScale3D(FVector(FallbackHeadSize / BasicShapeSize));
	FallbackHead->SetWorldLocation(Capsule->GetComponentLocation() + FVector(0.0f, 0.0f, HalfHeight - FallbackHeadSize * 0.5f));
	FallbackHead->ComponentTags.AddUnique(TEXT("Head"));

	if (BaseMaterial)
	{
		for (UStaticMeshComponent* Part : { FallbackBody.Get(), FallbackHead.Get() })
		{
			if (UMaterialInstanceDynamic* Mid = Part->CreateDynamicMaterialInstance(0, BaseMaterial))
			{
				Mid->SetVectorParameterValue(ColorParameterName, FallbackColor);
			}
		}
	}
}

void AProtoEnemyCharacter::SpawnWeapon()
{
	AProtoGameMode* GameMode = AProtoGameMode::Get(this);
	UProtoWeaponData* Data = GameMode ? GameMode->GetWeaponData(WeaponType) : nullptr;
	if (!Data || !WeaponClass)
	{
		UE_LOG(LogProto, Warning, TEXT("%s: no weapon data for weapon type %d."), *GetName(), static_cast<int32>(WeaponType));
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Weapon = GetWorld()->SpawnActor<AProtoWeapon>(WeaponClass, WeaponRoot->GetComponentTransform(), Params);
	if (Weapon)
	{
		Weapon->Initialize(Data);
		Weapon->AttachToComponent(WeaponRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void AProtoEnemyCharacter::AimWeaponAt(const FVector& Target)
{
	if (bDead)
	{
		return;
	}
	const FVector Direction = Target - WeaponRoot->GetComponentLocation();
	if (!Direction.IsNearlyZero())
	{
		WeaponRoot->SetWorldRotation(Direction.Rotation());
	}
}

void AProtoEnemyCharacter::SetCombatRotation(bool bCombat)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = !bCombat;
	Movement->bUseControllerDesiredRotation = bCombat;
}

FVector AProtoEnemyCharacter::GetAimOrigin() const
{
	return GetPawnViewLocation();
}

void AProtoEnemyCharacter::HandleDeath(AController* Killer)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	if (Weapon)
	{
		Weapon->ReleaseTrigger();
		Weapon->SetGlint(false, FLinearColor::Black);
		Weapon->SetActorHiddenInGame(true);
		Weapon->SetActorTickEnabled(false);
	}

	DetachFromControllerPendingDestroy();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bUsingMannequin)
	{
		USkeletalMeshComponent* MeshComponent = GetMesh();
		MeshComponent->SetCollisionProfileName(RagdollProfile);
		MeshComponent->SetAllBodiesSimulatePhysics(true);
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->WakeAllRigidBodies();
		MeshComponent->bBlendPhysics = true;
	}
	else
	{
		FallbackBody->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		FallbackBody->SetSimulatePhysics(true);
		FallbackBody->AddImpulse(GetActorForwardVector() * -FallbackTopplePush, NAME_None, true);
	}

	SetLifeSpan(CorpseLifetime);
}
