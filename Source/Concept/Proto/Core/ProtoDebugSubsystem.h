// Global debug toggles (god mode, AI freeze, debug drawing). Lives on the game instance so it survives level restarts.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProtoDebugSubsystem.generated.h"

UCLASS()
class CONCEPT_API UProtoDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Returns the subsystem for the world of WorldContext, or null. */
	static UProtoDebugSubsystem* Get(const UObject* WorldContext);

	/** Convenience: true when debug drawing is on for WorldContext's game instance. */
	static bool IsDebugDrawEnabled(const UObject* WorldContext);

	bool bGodMode = false;
	bool bFreezeAI = false;
	bool bDebugDraw = false;
};
