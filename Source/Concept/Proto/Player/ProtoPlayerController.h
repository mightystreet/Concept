// Player controller: debug hotkeys (F1-F9) and matching console commands.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ProtoPlayerController.generated.h"

UCLASS()
class CONCEPT_API AProtoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AProtoPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	/** Spawns an enemy with weapon slot 1-5 at a spawn pad. */
	UFUNCTION(Exec)
	void ProtoSpawnEnemy(int32 WeaponSlot);

	UFUNCTION(Exec)
	void ProtoClearEnemies();

	UFUNCTION(Exec)
	void ProtoGod();

	UFUNCTION(Exec)
	void ProtoFreezeAI();

	UFUNCTION(Exec)
	void ProtoDebug();

protected:
	virtual void BeginPlay() override;

	/** Index 0 spawns a pistol enemy ... index 4 an RPG enemy. */
	UPROPERTY(EditAnywhere, Category = "Debug Keys")
	TArray<FKey> SpawnEnemyKeys;

	UPROPERTY(EditAnywhere, Category = "Debug Keys")
	FKey ClearEnemiesKey = EKeys::F6;

	UPROPERTY(EditAnywhere, Category = "Debug Keys")
	FKey GodModeKey = EKeys::F7;

	UPROPERTY(EditAnywhere, Category = "Debug Keys")
	FKey FreezeAIKey = EKeys::F8;

	UPROPERTY(EditAnywhere, Category = "Debug Keys")
	FKey DebugDrawKey = EKeys::F9;

	/** How long the on-screen confirmation of a debug toggle stays up. */
	UPROPERTY(EditAnywhere, Category = "Debug Keys", meta = (ClampMin = "0", Units = "s"))
	float ToggleMessageDuration = 2.0f;

private:
	void ShowMessage(const FString& Message) const;
};
