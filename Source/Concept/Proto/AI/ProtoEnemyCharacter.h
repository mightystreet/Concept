// Enemy body: the stock mannequin (or a block fallback), one weapon, health and ragdoll death.
// All decisions are made by AProtoAIController.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoEnemyCharacter.generated.h"

class AProtoWeapon;
class UAnimInstance;
class UProtoHealthComponent;
class USkeletalMesh;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class CONCEPT_API AProtoEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AProtoEnemyCharacter();

	/** Set before FinishSpawning (deferred spawn) to choose the weapon and behaviour. */
	void SetWeaponType(EProtoWeaponType InType) { WeaponType = InType; }
	EProtoWeaponType GetWeaponType() const { return WeaponType; }

	AProtoWeapon* GetWeapon() const { return Weapon; }
	UProtoHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const { return bDead; }

	/** Points the gun at a world location (the AI aims with this). */
	void AimWeaponAt(const FVector& Target);

	/** Combat: face the controller's focus. Otherwise: face the walking direction. */
	void SetCombatRotation(bool bCombat);

	/** Where the AI sees from and aims from. */
	FVector GetAimOrigin() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetupBody();
	void SpawnWeapon();
	void HandleDeath(AController* Killer);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProtoHealthComponent> Health;

	/** The gun hangs from this at chest height; the controller rotates it to aim. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	/** Fallback body used when the mannequin is not available. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FallbackBody;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FallbackHead;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ExposeOnSpawn = "true"))
	EProtoWeaponType WeaponType = EProtoWeaponType::Rifle;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	TSubclassOf<AProtoWeapon> WeaponClass;

	/** Stock Third Person mannequin. If it or the anim class is missing, the block fallback body is shown. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	TSoftClassPtr<UAnimInstance> BodyAnimClass;

	/** Fallback head cube size (cm). The body cylinder fills the rest of the capsule. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Body", meta = (ClampMin = "1", Units = "cm"))
	float FallbackHeadSize = 28.0f;

	/** Mesh offset so the mannequin's feet touch the capsule bottom and it faces +X. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	FRotator MeshRotation = FRotator(0.0f, -90.0f, 0.0f);

	/** Gun pivot relative to the capsule centre (cm): in front of the right shoulder. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	FVector WeaponRootOffset = FVector(20.0f, 18.0f, 40.0f);

	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	FLinearColor FallbackColor = FLinearColor(0.5f, 0.15f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	FName ColorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, Category = "Enemy|Body")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Collision profile the mesh switches to on death. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Death")
	FName RagdollProfile = TEXT("Ragdoll");

	/** Seconds the corpse stays after death. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Death", meta = (ClampMin = "0", Units = "s"))
	float CorpseLifetime = 20.0f;

	/** Impulse applied to the fallback body when it topples (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Enemy|Death", meta = (ClampMin = "0"))
	float FallbackTopplePush = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Movement", meta = (ClampMin = "0"))
	float MoveSpeed = 380.0f;

	/** Turn speed toward the focus or walking direction (degrees/s). */
	UPROPERTY(EditAnywhere, Category = "Enemy|Movement", meta = (ClampMin = "0"))
	float TurnRate = 360.0f;

	UPROPERTY(Transient)
	TObjectPtr<AProtoWeapon> Weapon;

	bool bUsingMannequin = false;
	bool bDead = false;
};
