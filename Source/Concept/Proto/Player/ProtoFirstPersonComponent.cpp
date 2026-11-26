#include "Proto/Player/ProtoFirstPersonComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Proto/Player/ProtoCharacter.h"
#include "Proto/Weapons/ProtoWeapon.h"
#include "Proto/Weapons/ProtoWeaponData.h"

UProtoFirstPersonComponent::UProtoFirstPersonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Run after movement so the camera uses this frame's position. Cameras are updated after all ticks.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	LensMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Prototype/Materials/M_BodycamLens.M_BodycamLens")));
}

void UProtoFirstPersonComponent::Setup(UCameraComponent* InCamera, USceneComponent* InWeaponRoot)
{
	Camera = InCamera;
	WeaponRoot = InWeaponRoot;
}

void UProtoFirstPersonComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AProtoCharacter>(GetOwner());
	if (Character)
	{
		EyeHeight.Value = Character->GetStandingCameraHeight();
	}

	if (Camera)
	{
		if (UMaterialInterface* Lens = LensMaterial.LoadSynchronous())
		{
			LensMID = UMaterialInstanceDynamic::Create(Lens, this);
			Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, LensMID));
		}
		Camera->PostProcessBlendWeight = 1.0f;
		Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
		Camera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
		Camera->PostProcessSettings.bOverride_FilmGrainIntensity = true;
		UpdateLens();
	}
}

void UProtoFirstPersonComponent::ApplyViewDelta(float DeltaYaw, float DeltaPitch)
{
	AController* Controller = Character ? Character->GetController() : nullptr;
	if (!Controller || (FMath::IsNearlyZero(DeltaYaw) && FMath::IsNearlyZero(DeltaPitch)))
	{
		return;
	}

	FRotator Rotation = Controller->GetControlRotation();
	Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + DeltaYaw);
	Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch) + DeltaPitch, -MaxViewPitch, MaxViewPitch);
	Rotation.Roll = 0.0f;
	Controller->SetControlRotation(Rotation);
}

FVector2D UProtoFirstPersonComponent::GetFreeAimLimits() const
{
	return FreeAimDeadzone * FMath::Lerp(1.0f, AimDeadzoneScale, AimAlpha);
}

void UProtoFirstPersonComponent::AddLookInput(const FVector2D& LookInput)
{
	if (bDead)
	{
		return;
	}

	const FVector2D Delta = LookInput * LookSensitivity;
	const FVector2D Limits = GetFreeAimLimits();
	FVector2D Overflow = FVector2D::ZeroVector;

	auto MoveAxis = [](double& Value, double DeltaValue, double Limit, double& OutOverflow)
	{
		// While the deadzone is shrinking the gun may sit outside it; don't turn that leftover into view rotation.
		const double Bound = FMath::Max(Limit, FMath::Abs(Value));
		const double NewValue = Value + DeltaValue;
		const double Clamped = FMath::Clamp(NewValue, -Bound, Bound);
		OutOverflow = NewValue - Clamped;
		Value = Clamped;
	};

	MoveAxis(FreeAim.X, Delta.X, Limits.X, Overflow.X);
	MoveAxis(FreeAim.Y, Delta.Y, Limits.Y, Overflow.Y);
	ApplyViewDelta(static_cast<float>(Overflow.X), static_cast<float>(Overflow.Y));
}

void UProtoFirstPersonComponent::ClampFreeAim()
{
	const FVector2D Limits = GetFreeAimLimits();
	const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	const float MaxStep = FreeAimRecenterSpeed * Dt;

	auto Recenter = [MaxStep](double& Value, double Limit)
	{
		const double Clamped = FMath::Clamp(Value, -Limit, Limit);
		Value = Value + FMath::Clamp(Clamped - Value, -static_cast<double>(MaxStep), static_cast<double>(MaxStep));
	};
	Recenter(FreeAim.X, Limits.X);
	Recenter(FreeAim.Y, Limits.Y);
}

void UProtoFirstPersonComponent::AddRecoil(const AProtoWeapon* Weapon)
{
	const UProtoWeaponData* Data = Weapon ? Weapon->GetData() : nullptr;
	if (!Data)
	{
		return;
	}

	const float Scale = FMath::Lerp(1.0f, AimRecoilScale, AimAlpha);
	const float Pitch = Data->RecoilPitch * Scale;
	const float Yaw = FMath::FRandRange(-1.0f, 1.0f) * Data->RecoilYaw * Scale;

	GunRecoil.Value += FVector(Pitch, Yaw, -Yaw * RecoilRollPerYaw);
	CameraRecoil.Value += FVector(Pitch, Yaw, 0.0f) * Data->RecoilCameraFraction;
	KickBack.Value += Data->RecoilKickBack * Scale;
	RecoilRecoveryScale = Data->RecoilRecovery;

	// Permanent climb the player has to pull down.
	ApplyViewDelta(Yaw * Data->RecoilClimbFraction, Pitch * Data->RecoilClimbFraction);
}

void UProtoFirstPersonComponent::OnLanded(float FallSpeed)
{
	LandDip.Velocity -= FMath::Min(FallSpeed * LandDipPerFallSpeed, MaxLandDipVelocity);
}

void UProtoFirstPersonComponent::OnCrouchHeightChanged(float HalfHeightAdjust)
{
	EyeHeight.Value += HalfHeightAdjust;
}

void UProtoFirstPersonComponent::OnDeath()
{
	bDead = true;
}

void UProtoFirstPersonComponent::UpdateLens()
{
	if (!Camera)
	{
		return;
	}
	Camera->PostProcessSettings.VignetteIntensity = VignetteIntensity;
	Camera->PostProcessSettings.SceneFringeIntensity = ChromaticAberration;
	Camera->PostProcessSettings.FilmGrainIntensity = FilmGrainIntensity;
	if (LensMID)
	{
		LensMID->SetScalarParameterValue(DistortionParameterName, DistortionStrength * FMath::Lerp(1.0f, AimDistortionScale, AimAlpha));
	}
}

void UProtoFirstPersonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character || !Camera || !WeaponRoot || DeltaTime <= 0.0f)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaTime, ProtoSpring::MaxFrameTime);

	// ---- View rotation and turn rate ----
	const AController* Controller = Character->GetController();
	FRotator ControlRotation = Controller ? Controller->GetControlRotation() : Character->GetActorRotation();
	ControlRotation.Pitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
	ControlRotation.Roll = 0.0f;

	FVector ViewDelta = FVector::ZeroVector; // (Pitch, Yaw, 0)
	if (bHasLastControlRotation)
	{
		const FRotator Delta = (ControlRotation - LastControlRotation).GetNormalized();
		ViewDelta = FVector(Delta.Pitch, Delta.Yaw, 0.0f);
	}
	LastControlRotation = ControlRotation;
	bHasLastControlRotation = true;
	const FVector TurnRate = ViewDelta / Dt;

	const AProtoWeapon* Weapon = Character->GetCurrentWeapon();
	const UProtoWeaponData* Data = Weapon ? Weapon->GetData() : nullptr;
	const float WeaponSway = Data ? Data->SwayAmount : 1.0f;

	// ---- Aim, lowered, reload blends ----
	const float AimTarget = (!bDead && Data && Character->WantsToAim()) ? 1.0f : 0.0f;
	const float AimRate = Data ? 1.0f / Data->AimTime : 1.0f / LowerTime;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, AimTarget, Dt, AimRate);

	const float LoweredTarget = Character->WantsLowered() ? 1.0f : 0.0f;
	LoweredAlpha = FMath::FInterpConstantTo(LoweredAlpha, LoweredTarget, Dt, 1.0f / LowerTime);

	const float ReloadTarget = (Weapon && Weapon->IsReloading()) ? 1.0f : 0.0f;
	ReloadAlpha = FMath::FInterpTo(ReloadAlpha, ReloadTarget, Dt, ReloadBlendSpeed);

	ClampFreeAim();

	// ---- Camera lag: the camera falls behind view changes and springs back ----
	CameraLag.Value -= ViewDelta;
	CameraLag.Value = CameraLag.Value.BoundToCube(MaxCameraLag);
	CameraLag.Step(FVector::ZeroVector, CameraLagStiffness, CameraLagDamping, Dt);

	// ---- Sway: the gun trails turns ----
	const float SwayScale = WeaponSway * FMath::Lerp(1.0f, AimSwayScale, AimAlpha);
	const FVector SwayTarget = FVector(
		-TurnRate.X * SwayPerTurnRate,
		-TurnRate.Y * SwayPerTurnRate,
		-TurnRate.Y * SwayRollPerTurnRate).BoundToCube(MaxSwayAngle) * SwayScale;
	Sway.Step(SwayTarget, SwayStiffness, SwayDamping, Dt);

	const FVector SwayOffsetTarget = FVector(
		0.0f,
		-TurnRate.Y * SwayOffsetPerTurnRate,
		-TurnRate.X * SwayOffsetPerTurnRate).BoundToCube(MaxSwayOffset) * SwayScale;
	SwayOffset.Step(SwayOffsetTarget, SwayStiffness, SwayDamping, Dt);

	// ---- Bob and footstep shake ----
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const bool bGrounded = Movement && Movement->IsMovingOnGround();
	const float GroundSpeed = Character->GetVelocity().Size2D();
	const float SpeedRatioTarget = (bGrounded && GroundSpeed > BobMinSpeed)
		? FMath::Min(GroundSpeed / BobReferenceSpeed, MaxBobSpeedRatio)
		: 0.0f;
	BobSpeedRatio = FMath::FInterpTo(BobSpeedRatio, SpeedRatioTarget, Dt, BobBlendSpeed);

	if (bGrounded)
	{
		// One footstep every PI of phase.
		StridePhase += Dt * GroundSpeed / StrideLength * UE_PI;
	}
	const int32 StepIndex = FMath::FloorToInt32(StridePhase / UE_PI);
	if (StepIndex != LastStepIndex && SpeedRatioTarget > 0.0f)
	{
		const float Foot = (StepIndex % 2 == 0) ? 1.0f : -1.0f;
		Shake.Velocity += FVector(FootstepShakeImpulse.X, FootstepShakeImpulse.Y * Foot, FootstepShakeImpulse.Z * Foot) * BobSpeedRatio;
	}
	LastStepIndex = StepIndex;
	Shake.Step(FVector::ZeroVector, ShakeStiffness, ShakeDamping, Dt);

	const float AimBob = FMath::Lerp(1.0f, AimBobScale, AimAlpha);
	const float BobScale = BobSpeedRatio * AimBob * WeaponSway;
	const float StepWave = FMath::Cos(2.0f * StridePhase); // 1 at each footfall
	const float SideWave = FMath::Sin(StridePhase);         // one sway cycle per two steps
	const FVector GunBobPosition = FVector(GunBobAmplitude.X * StepWave, GunBobAmplitude.Y * SideWave, -GunBobAmplitude.Z * StepWave) * BobScale;
	const FRotator GunBobRot = FRotator(GunBobRotation.X * StepWave, GunBobRotation.Y * SideWave, GunBobRotation.Z * SideWave) * BobScale;
	const float CameraBobZ = -CameraBobHeight * StepWave * BobSpeedRatio * AimBob;

	// ---- Breathing ----
	BreathTime += Dt;
	const float BreathPhase = BreathTime * BreathRate * UE_TWO_PI;
	const float BreathScale = FMath::Lerp(1.0f, AimBreathScale, AimAlpha) * WeaponSway;
	const FRotator GunBreathRot = FRotator(
		GunBreathAngle.X * FMath::Sin(BreathPhase),
		GunBreathAngle.Y * FMath::Sin(BreathPhase * 0.5f),
		0.0f) * BreathScale;
	const float GunBreathZ = GunBreathHeight * FMath::Sin(BreathPhase) * BreathScale;
	const float CameraBreathPitch = CameraBreathAngle * FMath::Sin(BreathPhase) * BreathScale;

	// ---- Recoil recovery (damping scaled by sqrt to keep the same damping ratio) ----
	const float RecoveryDamping = FMath::Sqrt(RecoilRecoveryScale);
	GunRecoil.Step(FVector::ZeroVector, GunRecoilStiffness * RecoilRecoveryScale, GunRecoilDamping * RecoveryDamping, Dt);
	CameraRecoil.Step(FVector::ZeroVector, CameraRecoilStiffness * RecoilRecoveryScale, CameraRecoilDamping * RecoveryDamping, Dt);
	KickBack.Step(0.0f, KickBackStiffness * RecoilRecoveryScale, KickBackDamping * RecoveryDamping, Dt);

	// ---- Landing dip, lean, eye height, death ----
	LandDip.Step(0.0f, LandDipStiffness, LandDipDamping, Dt);
	Lean.Step(bDead ? 0.0f : Character->GetLeanInput(), LeanStiffness, LeanDamping, Dt);

	const float EyeTarget = bDead
		? DeathEyeHeight
		: (Character->bIsCrouched ? Character->GetCrouchedCameraHeight() : Character->GetStandingCameraHeight());
	EyeHeight.Step(EyeTarget, EyeHeightStiffness, EyeHeightDamping, Dt);
	DeathAlpha = FMath::FInterpConstantTo(DeathAlpha, bDead ? 1.0f : 0.0f, Dt, 1.0f / DeathFallTime);

	// ---- Camera location (relative to the capsule), with the lean stopped by walls ----
	const FVector BaseLocal(0.0f, 0.0f, EyeHeight.Value + LandDip.Value + CameraBobZ);
	const FVector LeanLocal(0.0f, Lean.Value * LeanDistance, 0.0f);
	float LeanFraction = 1.0f;
	if (!FMath::IsNearlyZero(Lean.Value))
	{
		const FTransform CapsuleTransform = Character->GetCapsuleComponent()->GetComponentTransform();
		const FVector Start = CapsuleTransform.TransformPosition(BaseLocal);
		const FVector End = CapsuleTransform.TransformPosition(BaseLocal + LeanLocal);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ProtoLean), false, Character);
		TArray<AActor*> Attached;
		Character->GetAttachedActors(Attached);
		Params.AddIgnoredActors(Attached);
		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(LeanProbeRadius), Params))
		{
			LeanFraction = Hit.Time;
		}
	}
	Camera->SetRelativeLocation(BaseLocal + LeanLocal * LeanFraction);

	// ---- Camera rotation ----
	FRotator CameraRotation = ControlRotation
		+ CameraLag.AsRotator()
		+ CameraRecoil.AsRotator()
		+ Shake.AsRotator();
	CameraRotation.Pitch += LandDip.Value * LandDipPitchPerCm + CameraBreathPitch;
	CameraRotation.Roll += Lean.Value * LeanFraction * LeanRoll + DeathAlpha * DeathRoll;
	Camera->SetWorldRotation(CameraRotation);

	// ---- Gun: aim direction in world space, converted to camera space ----
	const FRotator GunWorldRotation = ControlRotation
		+ FRotator(FreeAim.Y, FreeAim.X, 0.0f)
		+ Sway.AsRotator()
		+ GunRecoil.AsRotator()
		+ GunBreathRot
		+ GunBobRot;
	const FRotator PoseRotation = FRotator(
		LoweredRotation.X * LoweredAlpha + ReloadRotation.X * ReloadAlpha,
		LoweredRotation.Y * LoweredAlpha + ReloadRotation.Y * ReloadAlpha,
		LoweredRotation.Z * LoweredAlpha + ReloadRotation.Z * ReloadAlpha);
	const FQuat RelativeRotation = Camera->GetComponentQuat().Inverse() * GunWorldRotation.Quaternion() * PoseRotation.Quaternion();

	const FVector HipOffset = Data ? Data->HipOffset : FVector::ZeroVector;
	const FVector AimOffset = Data ? Data->AimOffset : FVector::ZeroVector;
	const FVector GunPosition = FMath::Lerp(HipOffset, AimOffset, AimAlpha)
		+ GunBobPosition
		+ FVector(-KickBack.Value, 0.0f, GunBreathZ)
		+ SwayOffset.Value
		+ LoweredOffset * LoweredAlpha
		+ ReloadOffset * ReloadAlpha;
	WeaponRoot->SetRelativeLocationAndRotation(GunPosition, RelativeRotation);

	// ---- FOV and lens ----
	const float AimFOV = Data ? Data->AimFOV : BaseFOV;
	Camera->SetFieldOfView(FMath::Lerp(BaseFOV, AimFOV, AimAlpha));
	UpdateLens();
}
