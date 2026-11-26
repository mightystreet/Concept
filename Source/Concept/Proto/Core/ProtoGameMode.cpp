#include "Proto/Core/ProtoGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Proto/AI/ProtoEnemyCharacter.h"
#include "Proto/Core/ProtoLog.h"
#include "Proto/Core/ProtoSpawnPad.h"
#include "Proto/Player/ProtoCharacter.h"
#include "Proto/Player/ProtoHUD.h"
#include "Proto/Player/ProtoPlayerController.h"
#include "Proto/Weapons/ProtoWeaponData.h"

AProtoGameMode::AProtoGameMode()
{
	DefaultPawnClass = AProtoCharacter::StaticClass();
	PlayerControllerClass = AProtoPlayerController::StaticClass();
	HUDClass = AProtoHUD::StaticClass();
	EnemyClass = AProtoEnemyCharacter::StaticClass();

	auto AddWeapon = [this](EProtoWeaponType Type, const TCHAR* Path)
	{
		WeaponLoadout.Add(Type, TSoftObjectPtr<UProtoWeaponData>(FSoftObjectPath(Path)));
	};
	AddWeapon(EProtoWeaponType::Pistol, TEXT("/Game/Prototype/Data/DA_Pistol.DA_Pistol"));
	AddWeapon(EProtoWeaponType::Rifle, TEXT("/Game/Prototype/Data/DA_Rifle.DA_Rifle"));
	AddWeapon(EProtoWeaponType::MachineGun, TEXT("/Game/Prototype/Data/DA_MachineGun.DA_MachineGun"));
	AddWeapon(EProtoWeaponType::Sniper, TEXT("/Game/Prototype/Data/DA_Sniper.DA_Sniper"));
	AddWeapon(EProtoWeaponType::RPG, TEXT("/Game/Prototype/Data/DA_RPG.DA_RPG"));
}

AProtoGameMode* AProtoGameMode::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetAuthGameMode<AProtoGameMode>() : nullptr;
}

UProtoWeaponData* AProtoGameMode::GetWeaponData(EProtoWeaponType Type)
{
	if (TObjectPtr<UProtoWeaponData>* Loaded = LoadedWeaponData.Find(Type))
	{
		return *Loaded;
	}

	const TSoftObjectPtr<UProtoWeaponData>* Soft = WeaponLoadout.Find(Type);
	UProtoWeaponData* Data = Soft ? Soft->LoadSynchronous() : nullptr;
	if (!Data)
	{
		UE_LOG(LogProto, Warning, TEXT("Weapon data for type %d is missing (%s)."), static_cast<int32>(Type),
			Soft ? *Soft->ToString() : TEXT("not in WeaponLoadout"));
		return nullptr;
	}
	LoadedWeaponData.Add(Type, Data);
	return Data;
}

AProtoEnemyCharacter* AProtoGameMode::SpawnEnemy(EProtoWeaponType Type)
{
	UWorld* World = GetWorld();
	if (!World || !EnemyClass)
	{
		return nullptr;
	}

	TArray<AProtoSpawnPad*> Pads;
	for (TActorIterator<AProtoSpawnPad> It(World); It; ++It)
	{
		if (It->Allows(Type))
		{
			Pads.Add(*It);
		}
	}
	if (Pads.IsEmpty())
	{
		UE_LOG(LogProto, Warning, TEXT("No spawn pad allows weapon type %d. Place an AProtoSpawnPad in the level."), static_cast<int32>(Type));
		return nullptr;
	}

	// Stable order so the round robin is predictable.
	Pads.Sort([](const AProtoSpawnPad& A, const AProtoSpawnPad& B) { return A.GetName() < B.GetName(); });
	AProtoSpawnPad* Pad = Pads[NextPadIndex % Pads.Num()];
	++NextPadIndex;

	AProtoEnemyCharacter* Enemy = World->SpawnActorDeferred<AProtoEnemyCharacter>(EnemyClass, Pad->GetSpawnTransform(),
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Enemy)
	{
		return nullptr;
	}
	Enemy->SetWeaponType(Type);
	Enemy->FinishSpawning(Pad->GetSpawnTransform());
	UE_LOG(LogProto, Log, TEXT("Spawned %s at %s."), *Enemy->GetName(), *Pad->GetName());
	return Enemy;
}

int32 AProtoGameMode::ClearEnemies()
{
	int32 Count = 0;
	for (TActorIterator<AProtoEnemyCharacter> It(GetWorld()); It; ++It)
	{
		It->Destroy();
		++Count;
	}
	return Count;
}

void AProtoGameMode::NotifyPlayerDied(AController* PlayerController)
{
	if (GetWorldTimerManager().IsTimerActive(RestartTimer))
	{
		return;
	}
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AProtoGameMode::RestartCurrentLevel, FMath::Max(RestartDelay, UE_KINDA_SMALL_NUMBER));
}

void AProtoGameMode::RestartCurrentLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}
