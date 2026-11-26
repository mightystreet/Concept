#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UProtoDebugSubsystem* UProtoDebugSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UProtoDebugSubsystem>() : nullptr;
}

bool UProtoDebugSubsystem::IsDebugDrawEnabled(const UObject* WorldContext)
{
	const UProtoDebugSubsystem* Subsystem = Get(WorldContext);
	return Subsystem && Subsystem->bDebugDraw;
}
