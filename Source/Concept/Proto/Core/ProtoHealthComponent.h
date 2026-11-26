// Shared health component for the player and enemies.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProtoHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnProtoDamaged, float /*Amount*/, AActor* /*DamageCauser*/, AController* /*InstigatedBy*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnProtoDeath, AController* /*Killer*/);

UCLASS(ClassGroup = (Proto), meta = (BlueprintSpawnableComponent))
class CONCEPT_API UProtoHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UProtoHealthComponent();

	/** Finds the health component on Actor, or null. */
	static UProtoHealthComponent* FindOn(const AActor* Actor);

	/**
	 * Applies BaseDamage, doubled (HeadshotMultiplier) for headshots. Ignored when dead, or when god mode is on and
	 * this component belongs to a player-controlled pawn. Returns the damage actually applied.
	 */
	float ApplyDamage(float BaseDamage, bool bHeadshot, AActor* DamageCauser, AController* InstigatedBy);

	/** True if Hit lands on the head: a head bone on a skeletal mesh, or the top HeadZoneHeight of a capsule. */
	bool IsHeadshot(const FHitResult& Hit) const;

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsDead() const { return bIsDead; }

	FOnProtoDamaged OnDamaged;
	FOnProtoDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float HeadshotMultiplier = 2.0f;

	/** Bones that count as the head on skeletal meshes. */
	UPROPERTY(EditAnywhere, Category = "Health")
	TArray<FName> HeadBoneNames;

	/** For capsule-only bodies: hits within this distance of the capsule top count as headshots (cm). */
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "0"))
	float HeadZoneHeight = 30.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	float Health = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	bool bIsDead = false;
};
