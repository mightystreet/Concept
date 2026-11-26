// Shared enums and constants for the bodycam prototype.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ProtoTypes.generated.h"

/** Custom trace channel used by every weapon trace and projectile sweep ("ProtoWeapon" in DefaultEngine.ini). */
inline constexpr ECollisionChannel ECC_ProtoWeapon = ECC_GameTraceChannel1;

/** Weapon slots 1-5. Also selects enemy behaviour. */
UENUM(BlueprintType)
enum class EProtoWeaponType : uint8
{
	Pistol,
	Rifle,
	MachineGun,
	Sniper,
	RPG,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EProtoWeaponType, EProtoWeaponType::Count);

UENUM(BlueprintType)
enum class EProtoFireMode : uint8
{
	/** One shot per trigger pull. */
	Semi,
	/** Fires while the trigger is held. */
	Auto,
	/** One shot per pull, then a long cycle time. */
	Bolt,
	/** Single shot launcher. */
	Single
};

UENUM(BlueprintType)
enum class EProtoHitMethod : uint8
{
	LineTrace,
	/** Bullet projectile with gravity drop. */
	Projectile,
	/** Slow rocket with radial falloff damage. */
	Rocket
};

UENUM(BlueprintType)
enum class EProtoAIState : uint8
{
	Idle,
	Alert,
	Engage,
	Search
};
