// Game mode: weapon loadout, enemy spawning at spawn pads, and level restart after the player dies.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Proto/Core/ProtoTypes.h"
#include "ProtoGameMode.generated.h"

class UProtoWeaponData;
class AProtoEnemyCharacter;

UCLASS()
class CONCEPT_API AProtoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AProtoGameMode();

	/** Returns the game mode of WorldContext's world, or null. */
	static AProtoGameMode* Get(const UObject* WorldContext);

	/** Weapon stats for Type (loaded on first use). Null if the data asset is missing. */
	UProtoWeaponData* GetWeaponData(EProtoWeaponType Type);

	/** Spawns an enemy carrying Type at the next spawn pad that allows it. Returns null if there is no pad. */
	AProtoEnemyCharacter* SpawnEnemy(EProtoWeaponType Type);

	/** Destroys every enemy. Returns how many were removed. */
	int32 ClearEnemies();

	/** Schedules the level restart. */
	void NotifyPlayerDied(AController* PlayerController);

protected:
	void RestartCurrentLevel();

	/** Weapon data per slot. */
	UPROPERTY(EditAnywhere, Category = "Proto|Weapons")
	TMap<EProtoWeaponType, TSoftObjectPtr<UProtoWeaponData>> WeaponLoadout;

	UPROPERTY(EditAnywhere, Category = "Proto|Enemies")
	TSubclassOf<AProtoEnemyCharacter> EnemyClass;

	/** Seconds from player death to the level reload. */
	UPROPERTY(EditAnywhere, Category = "Proto|Restart", meta = (ClampMin = "0", Units = "s"))
	float RestartDelay = 2.0f;

	/** Strong references to the loaded data so it is not garbage collected. */
	UPROPERTY(Transient)
	TMap<EProtoWeaponType, TObjectPtr<UProtoWeaponData>> LoadedWeaponData;

	FTimerHandle RestartTimer;
	int32 NextPadIndex = 0;
};
