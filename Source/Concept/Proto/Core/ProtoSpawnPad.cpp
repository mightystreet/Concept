#include "Proto/Core/ProtoSpawnPad.h"

#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Engine basic shapes are 100 cm across. */
	constexpr float BasicShapeSize = 100.0f;
}

AProtoSpawnPad::AProtoSpawnPad()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = MaterialFinder.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DiscMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiscMesh"));
	DiscMesh->SetupAttachment(Root);
	DiscMesh->SetStaticMesh(CylinderFinder.Object);
	DiscMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DiscMesh->SetCanEverAffectNavigation(false);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	Arrow->SetHiddenInGame(false);
}

void AProtoSpawnPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	DiscMesh->SetRelativeScale3D(FVector(DiscDiameter, DiscDiameter, DiscThickness) / BasicShapeSize);
	DiscMesh->SetRelativeLocation(FVector(0.0f, 0.0f, DiscThickness * 0.5f));
	if (BaseMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = DiscMesh->CreateDynamicMaterialInstance(0, BaseMaterial))
		{
			Mid->SetVectorParameterValue(ColorParameterName, DiscColor);
		}
	}
}

bool AProtoSpawnPad::Allows(EProtoWeaponType Type) const
{
	return AllowedWeapons.IsEmpty() || AllowedWeapons.Contains(Type);
}

FTransform AProtoSpawnPad::GetSpawnTransform() const
{
	const FVector Location = GetActorLocation() + FVector::UpVector * SpawnHeight;
	return FTransform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), Location);
}
