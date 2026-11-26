#include "Proto/Player/ProtoPlayerController.h"

#include "Engine/Engine.h"
#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Proto/Core/ProtoGameMode.h"
#include "Proto/Core/ProtoLog.h"

namespace
{
	/** Shared on-screen message slot so repeated toggles replace each other instead of stacking. */
	constexpr int32 DebugMessageKey = 0x50524F54;
}

AProtoPlayerController::AProtoPlayerController()
{
	SpawnEnemyKeys = { EKeys::F1, EKeys::F2, EKeys::F3, EKeys::F4, EKeys::F5 };
}

void AProtoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void AProtoPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	for (int32 Index = 0; Index < SpawnEnemyKeys.Num(); ++Index)
	{
		if (WasInputKeyJustPressed(SpawnEnemyKeys[Index]))
		{
			ProtoSpawnEnemy(Index + 1);
		}
	}
	if (WasInputKeyJustPressed(ClearEnemiesKey))
	{
		ProtoClearEnemies();
	}
	if (WasInputKeyJustPressed(GodModeKey))
	{
		ProtoGod();
	}
	if (WasInputKeyJustPressed(FreezeAIKey))
	{
		ProtoFreezeAI();
	}
	if (WasInputKeyJustPressed(DebugDrawKey))
	{
		ProtoDebug();
	}
}

void AProtoPlayerController::ProtoSpawnEnemy(int32 WeaponSlot)
{
	const int32 TypeIndex = WeaponSlot - 1;
	if (TypeIndex < 0 || TypeIndex >= static_cast<int32>(EProtoWeaponType::Count))
	{
		ShowMessage(FString::Printf(TEXT("ProtoSpawnEnemy: slot must be 1-%d"), static_cast<int32>(EProtoWeaponType::Count)));
		return;
	}
	AProtoGameMode* GameMode = AProtoGameMode::Get(this);
	if (!GameMode)
	{
		ShowMessage(TEXT("ProtoSpawnEnemy: the level is not using AProtoGameMode"));
		return;
	}
	const EProtoWeaponType Type = static_cast<EProtoWeaponType>(TypeIndex);
	const bool bSpawned = GameMode->SpawnEnemy(Type) != nullptr;
	ShowMessage(FString::Printf(TEXT("%s %s enemy"), bSpawned ? TEXT("Spawned") : TEXT("Could not spawn"),
		*UEnum::GetDisplayValueAsText(Type).ToString()));
}

void AProtoPlayerController::ProtoClearEnemies()
{
	if (AProtoGameMode* GameMode = AProtoGameMode::Get(this))
	{
		ShowMessage(FString::Printf(TEXT("Cleared %d enemies"), GameMode->ClearEnemies()));
	}
}

void AProtoPlayerController::ProtoGod()
{
	if (UProtoDebugSubsystem* Debug = UProtoDebugSubsystem::Get(this))
	{
		Debug->bGodMode = !Debug->bGodMode;
		ShowMessage(FString::Printf(TEXT("God mode %s"), Debug->bGodMode ? TEXT("ON") : TEXT("OFF")));
	}
}

void AProtoPlayerController::ProtoFreezeAI()
{
	if (UProtoDebugSubsystem* Debug = UProtoDebugSubsystem::Get(this))
	{
		Debug->bFreezeAI = !Debug->bFreezeAI;
		ShowMessage(FString::Printf(TEXT("AI freeze %s"), Debug->bFreezeAI ? TEXT("ON") : TEXT("OFF")));
	}
}

void AProtoPlayerController::ProtoDebug()
{
	if (UProtoDebugSubsystem* Debug = UProtoDebugSubsystem::Get(this))
	{
		Debug->bDebugDraw = !Debug->bDebugDraw;
		ShowMessage(FString::Printf(TEXT("Debug draw %s"), Debug->bDebugDraw ? TEXT("ON") : TEXT("OFF")));
	}
}

void AProtoPlayerController::ShowMessage(const FString& Message) const
{
	UE_LOG(LogProto, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(DebugMessageKey, ToggleMessageDuration, FColor::Yellow, Message);
	}
}
