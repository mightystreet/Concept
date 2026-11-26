// No HUD by default. With debug draw on (F9) it prints player health, weapon and ammo state.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ProtoHUD.generated.h"

UCLASS()
class CONCEPT_API AProtoHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	/** Top-left corner of the debug text block, in screen pixels. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	FVector2D TextOrigin = FVector2D(40.0f, 40.0f);

	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0"))
	float LineHeight = 18.0f;

	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0.1"))
	float TextScale = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	FLinearColor TextColor = FLinearColor(0.2f, 1.0f, 0.2f);

private:
	void DrawLine(const FString& Text, int32& Line);
};
