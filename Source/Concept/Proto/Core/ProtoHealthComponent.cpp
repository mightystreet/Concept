#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/Pawn.h"

UProtoHealthComponent::UProtoHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	HeadBoneNames = { TEXT("head") };
}

UProtoHealthComponent* UProtoHealthComponent::FindOn(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UProtoHealthComponent>() : nullptr;
}

void UProtoHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	bIsDead = false;
}

float UProtoHealthComponent::ApplyDamage(float BaseDamage, bool bHeadshot, AActor* DamageCauser, AController* InstigatedBy)
{
	if (bIsDead || BaseDamage <= 0.0f)
	{
		return 0.0f;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const UProtoDebugSubsystem* Debug = UProtoDebugSubsystem::Get(this);
	if (Debug && Debug->bGodMode && OwnerPawn && OwnerPawn->IsPlayerControlled())
	{
		return 0.0f;
	}

	const float Amount = bHeadshot ? BaseDamage * HeadshotMultiplier : BaseDamage;
	Health = FMath::Max(0.0f, Health - Amount);
	OnDamaged.Broadcast(Amount, DamageCauser, InstigatedBy);

	if (Health <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast(InstigatedBy);
	}
	return Amount;
}

bool UProtoHealthComponent::IsHeadshot(const FHitResult& Hit) const
{
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(HitComponent))
	{
		for (const FName& HeadBone : HeadBoneNames)
		{
			if (Hit.BoneName == HeadBone || (Hit.BoneName != NAME_None && Skinned->BoneIsChildOf(Hit.BoneName, HeadBone)))
			{
				return true;
			}
		}
		return false;
	}

	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(HitComponent))
	{
		const float CapsuleTop = Capsule->GetComponentLocation().Z + Capsule->GetScaledCapsuleHalfHeight();
		return Hit.ImpactPoint.Z >= CapsuleTop - HeadZoneHeight;
	}

	// Fallback for primitive bodies (e.g. a sphere head component tagged "Head").
	return HitComponent && HitComponent->ComponentHasTag(TEXT("Head"));
}
