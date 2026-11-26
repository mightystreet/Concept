#include "Proto/Player/ProtoHUD.h"

#include "Engine/Engine.h"
#include "Proto/Core/ProtoDebugSubsystem.h"
#include "Proto/Core/ProtoHealthComponent.h"
#include "Proto/Player/ProtoCharacter.h"
#include "Proto/Weapons/ProtoWeapon.h"
#include "Proto/Weapons/ProtoWeaponData.h"

namespace
{
	constexpr float PercentScale = 100.0f;
}

void AProtoHUD::DrawHUD()
{
	Super::DrawHUD();

	const UProtoDebugSubsystem* Debug = UProtoDebugSubsystem::Get(this);
	if (!Debug || !Debug->bDebugDraw)
	{
		return;
	}

	int32 Line = 0;
	DrawLine(FString::Printf(TEXT("GOD %s   AI FROZEN %s"), Debug->bGodMode ? TEXT("ON") : TEXT("off"),
		Debug->bFreezeAI ? TEXT("ON") : TEXT("off")), Line);

	const AProtoCharacter* Player = Cast<AProtoCharacter>(GetOwningPawn());
	if (!Player)
	{
		return;
	}

	if (const UProtoHealthComponent* Health = Player->GetHealth())
	{
		DrawLine(FString::Printf(TEXT("Health %.0f / %.0f"), Health->GetHealth(), Health->GetMaxHealth()), Line);
	}
	DrawLine(FString::Printf(TEXT("Speed %.0f%s"), Player->GetVelocity().Size2D(), Player->IsSprinting() ? TEXT("  SPRINT") : TEXT("")), Line);

	const AProtoWeapon* Weapon = Player->GetCurrentWeapon();
	if (!Weapon)
	{
		DrawLine(TEXT("No weapon"), Line);
		return;
	}

	const UProtoWeaponData* Data = Weapon->GetData();
	const FString Name = Data ? Data->DisplayName.ToString() : Weapon->GetName();
	DrawLine(FString::Printf(TEXT("%s  %d / %d  [%s]"), *Name, Weapon->GetAmmo(), Weapon->GetMagazineSize(),
		*UEnum::GetDisplayValueAsText(Weapon->GetFireMode()).ToString()), Line);
	if (Weapon->IsReloading())
	{
		DrawLine(FString::Printf(TEXT("Reloading %.0f%%"), Weapon->GetReloadProgress() * PercentScale), Line);
	}
}

void AProtoHUD::DrawLine(const FString& Text, int32& Line)
{
	DrawText(Text, TextColor, TextOrigin.X, TextOrigin.Y + LineHeight * TextScale * Line, GEngine ? GEngine->GetSmallFont() : nullptr, TextScale);
	++Line;
}
