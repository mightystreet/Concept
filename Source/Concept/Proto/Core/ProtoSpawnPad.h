// Enemy spawn point placed in the arena. F1-F5 spawn enemies at these pads in turn.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoSpawnPad.generated.h"

class UArrowComponent;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS()
class CONCEPT_API AProtoSpawnPad : public AActor
{
	GENERATED_BODY()

public:
	AProtoSpawnPad();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** True if an enemy with Type may spawn here (empty AllowedWeapons = any). */
	bool Allows(EProtoWeaponType Type) const;

	/** Where to spawn: above the pad, facing the arrow. */
	FTransform GetSpawnTransform() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DiscMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> Arrow;

	/** Weapons allowed at this pad. Empty = any. Use it to put snipers at the far end of the lane. */
	UPROPERTY(EditAnywhere, Category = "SpawnPad")
	TArray<EProtoWeaponType> AllowedWeapons;

	/** Spawn height above the pad (cm); roughly the enemy capsule half height. */
	UPROPERTY(EditAnywhere, Category = "SpawnPad", meta = (ClampMin = "0", Units = "cm"))
	float SpawnHeight = 100.0f;

	UPROPERTY(EditAnywhere, Category = "SpawnPad", meta = (ClampMin = "1", Units = "cm"))
	float DiscDiameter = 120.0f;

	UPROPERTY(EditAnywhere, Category = "SpawnPad", meta = (ClampMin = "0.1", Units = "cm"))
	float DiscThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "SpawnPad")
	FLinearColor DiscColor = FLinearColor(0.6f, 0.1f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "SpawnPad")
	FName ColorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, Category = "SpawnPad")
	TObjectPtr<UMaterialInterface> BaseMaterial;
};
