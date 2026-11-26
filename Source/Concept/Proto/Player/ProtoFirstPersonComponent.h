// Procedural first-person rig: free aim, camera lag, bob, footstep shake, breathing, sway, recoil, landing dip,
// lean, crouch eye-height smoothing, lowered/reload poses, FOV and the bodycam lens. No animation assets.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Proto/Core/ProtoSpring.h"
#include "ProtoFirstPersonComponent.generated.h"

class AProtoCharacter;
class AProtoWeapon;
class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (Proto), meta = (BlueprintSpawnableComponent))
class CONCEPT_API UProtoFirstPersonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UProtoFirstPersonComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sets the camera and the scene component the active weapon hangs from (attached to the camera). */
	void Setup(UCameraComponent* InCamera, USceneComponent* InWeaponRoot);

	/** Raw look input (mouse units). Moves the gun inside the free-aim deadzone, then turns the view. */
	void AddLookInput(const FVector2D& LookInput);

	/** Kicks the recoil springs for one shot of Weapon. */
	void AddRecoil(const AProtoWeapon* Weapon);

	/** Landing camera dip. FallSpeed is the downward speed at impact (cm/s, positive). */
	void OnLanded(float FallSpeed);

	/** Keeps the camera world height continuous when the capsule changes height while crouching. */
	void OnCrouchHeightChanged(float HalfHeightAdjust);

	/** Starts the death camera fall. */
	void OnDeath();

	float GetAimAlpha() const { return AimAlpha; }
	float GetLoweredAlpha() const { return LoweredAlpha; }

protected:
	virtual void BeginPlay() override;

	void ApplyViewDelta(float DeltaYaw, float DeltaPitch);
	FVector2D GetFreeAimLimits() const;
	void ClampFreeAim();
	void UpdateLens();

	// ---- Look ----
	/** Degrees of rotation per unit of look input. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Look", meta = (ClampMin = "0"))
	float LookSensitivity = 2.5f;

	/** Pitch limit of the view (degrees up and down). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Look", meta = (ClampMin = "0", ClampMax = "89.9"))
	float MaxViewPitch = 85.0f;

	/** Free-aim deadzone half-size at the hip: the gun moves this far before the view turns (degrees). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|FreeAim")
	FVector2D FreeAimDeadzone = FVector2D(6.0f, 4.0f);

	/** Deadzone multiplier at full aim (near zero centres the gun). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|FreeAim", meta = (ClampMin = "0", ClampMax = "1"))
	float AimDeadzoneScale = 0.02f;

	/** Speed the gun recentres when the deadzone shrinks (degrees/s). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|FreeAim", meta = (ClampMin = "0"))
	float FreeAimRecenterSpeed = 60.0f;

	// ---- Camera lag ----
	/** Spring stiffness of the camera following the view (higher = less lag). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|CameraLag", meta = (ClampMin = "0"))
	float CameraLagStiffness = 900.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|CameraLag", meta = (ClampMin = "0"))
	float CameraLagDamping = 55.0f;

	/** Largest lag the camera may fall behind (degrees). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|CameraLag", meta = (ClampMin = "0"))
	float MaxCameraLag = 4.0f;

	// ---- Sway ----
	/** Gun rotation lag per degree/second of turn speed. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float SwayPerTurnRate = 0.012f;

	/** Gun roll per degree/second of yaw rate. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float SwayRollPerTurnRate = 0.02f;

	/** Gun sideways shift per degree/second of yaw rate (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float SwayOffsetPerTurnRate = 0.006f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float MaxSwayAngle = 7.0f;

	/** Largest positional sway (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float MaxSwayOffset = 2.5f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float SwayStiffness = 120.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0"))
	float SwayDamping = 14.0f;

	/** Sway multiplier at full aim. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Sway", meta = (ClampMin = "0", ClampMax = "1"))
	float AimSwayScale = 0.35f;

	// ---- Bob and footsteps ----
	/** Distance covered per footstep (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "1"))
	float StrideLength = 90.0f;

	/** Speed the bob amplitudes are normalised to (cm/s). Faster movement bobs more. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "1"))
	float BobReferenceSpeed = 420.0f;

	/** Largest speed ratio applied to bob and shake. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0"))
	float MaxBobSpeedRatio = 1.6f;

	/** Gun bob at reference speed (cm): X forward, Y side, Z vertical. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob")
	FVector GunBobAmplitude = FVector(0.4f, 0.8f, 0.7f);

	/** Gun bob rotation at reference speed (degrees: pitch, yaw, roll). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob")
	FVector GunBobRotation = FVector(0.8f, 0.6f, 1.2f);

	/** Camera vertical bob at reference speed (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0"))
	float CameraBobHeight = 1.2f;

	/** Bob multiplier at full aim. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0", ClampMax = "1"))
	float AimBobScale = 0.25f;

	/** Ground speed below which bob fades out (cm/s). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0"))
	float BobMinSpeed = 20.0f;

	/** Camera shake impulse per footstep at reference speed (degrees/s: pitch, yaw, roll). Roll alternates feet. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob")
	FVector FootstepShakeImpulse = FVector(-14.0f, 3.0f, 10.0f);

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0"))
	float ShakeStiffness = 260.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0"))
	float ShakeDamping = 18.0f;

	/** Bob amplitude blend speed when starting/stopping (per second). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Bob", meta = (ClampMin = "0"))
	float BobBlendSpeed = 6.0f;

	// ---- Breathing ----
	/** Breaths per second. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Breathing", meta = (ClampMin = "0"))
	float BreathRate = 0.25f;

	/** Gun breathing sway (degrees: pitch, yaw). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Breathing")
	FVector2D GunBreathAngle = FVector2D(0.5f, 0.35f);

	/** Gun breathing rise (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Breathing", meta = (ClampMin = "0"))
	float GunBreathHeight = 0.3f;

	/** Camera breathing (degrees pitch). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Breathing", meta = (ClampMin = "0"))
	float CameraBreathAngle = 0.15f;

	/** Breathing multiplier at full aim. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Breathing", meta = (ClampMin = "0", ClampMax = "1"))
	float AimBreathScale = 0.5f;

	// ---- Recoil ----
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float GunRecoilStiffness = 220.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float GunRecoilDamping = 20.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float CameraRecoilStiffness = 120.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float CameraRecoilDamping = 16.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float KickBackStiffness = 400.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float KickBackDamping = 28.0f;

	/** Gun roll per degree of yaw kick. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0"))
	float RecoilRollPerYaw = 1.5f;

	/** Recoil multiplier at full aim. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Recoil", meta = (ClampMin = "0", ClampMax = "1"))
	float AimRecoilScale = 0.6f;

	// ---- Landing ----
	/** Camera drop per cm/s of fall speed (cm per cm/s, applied as spring velocity). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Landing", meta = (ClampMin = "0"))
	float LandDipPerFallSpeed = 0.18f;

	/** Largest dip velocity (cm/s). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Landing", meta = (ClampMin = "0"))
	float MaxLandDipVelocity = 220.0f;

	/** Camera pitch-down per cm of dip (degrees). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Landing", meta = (ClampMin = "0"))
	float LandDipPitchPerCm = 0.25f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Landing", meta = (ClampMin = "0"))
	float LandDipStiffness = 140.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Landing", meta = (ClampMin = "0"))
	float LandDipDamping = 15.0f;

	// ---- Lean ----
	/** Sideways camera shift at full lean (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lean", meta = (ClampMin = "0"))
	float LeanDistance = 35.0f;

	/** Camera roll at full lean (degrees). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lean", meta = (ClampMin = "0"))
	float LeanRoll = 12.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lean", meta = (ClampMin = "0"))
	float LeanStiffness = 90.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lean", meta = (ClampMin = "0"))
	float LeanDamping = 19.0f;

	/** Radius of the trace that stops the lean going into walls (cm). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lean", meta = (ClampMin = "0"))
	float LeanProbeRadius = 12.0f;

	// ---- Eye height ----
	UPROPERTY(EditAnywhere, Category = "FirstPerson|EyeHeight", meta = (ClampMin = "0"))
	float EyeHeightStiffness = 160.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|EyeHeight", meta = (ClampMin = "0"))
	float EyeHeightDamping = 25.0f;

	// ---- Poses ----
	/** Seconds to fully lower or raise the gun (sprint, switch). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Poses", meta = (ClampMin = "0.01", Units = "s"))
	float LowerTime = 0.2f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Poses")
	FVector LoweredOffset = FVector(-4.0f, 4.0f, -12.0f);

	/** Lowered gun rotation (degrees: pitch, yaw, roll). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Poses")
	FVector LoweredRotation = FVector(-35.0f, 25.0f, -15.0f);

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Poses")
	FVector ReloadOffset = FVector(-3.0f, -2.0f, -9.0f);

	/** Reload dip gun rotation (degrees: pitch, yaw, roll). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Poses")
	FVector ReloadRotation = FVector(-12.0f, -10.0f, 35.0f);

	/** Speed the reload dip blends in and out (per second). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Poses", meta = (ClampMin = "0"))
	float ReloadBlendSpeed = 5.0f;

	// ---- Death ----
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Death")
	float DeathEyeHeight = -60.0f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Death")
	float DeathRoll = 75.0f;

	/** Seconds for the death roll to complete. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Death", meta = (ClampMin = "0.01", Units = "s"))
	float DeathFallTime = 0.6f;

	// ---- Lens ----
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens", meta = (ClampMin = "5", ClampMax = "170", Units = "Degrees"))
	float BaseFOV = 105.0f;

	/** Post-process lens material with a "DistortionStrength" scalar (barrel / fisheye). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens")
	TSoftObjectPtr<UMaterialInterface> LensMaterial;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens")
	FName DistortionParameterName = TEXT("DistortionStrength");

	/** Fisheye strength (0 = none). */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens", meta = (ClampMin = "0", ClampMax = "1"))
	float DistortionStrength = 0.25f;

	/** Distortion multiplier at full aim. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens", meta = (ClampMin = "0", ClampMax = "1"))
	float AimDistortionScale = 0.4f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens", meta = (ClampMin = "0", ClampMax = "1"))
	float VignetteIntensity = 0.7f;

	/** Chromatic aberration (scene fringe) intensity. */
	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens", meta = (ClampMin = "0", ClampMax = "5"))
	float ChromaticAberration = 0.8f;

	UPROPERTY(EditAnywhere, Category = "FirstPerson|Lens", meta = (ClampMin = "0", ClampMax = "1"))
	float FilmGrainIntensity = 0.35f;

	// ---- Runtime ----
	UPROPERTY(Transient)
	TObjectPtr<AProtoCharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LensMID;

	/** Gun offset from the view inside the deadzone (X = yaw, Y = pitch, degrees). */
	FVector2D FreeAim = FVector2D::ZeroVector;
	FRotator LastControlRotation = FRotator::ZeroRotator;
	bool bHasLastControlRotation = false;

	FProtoSpringVector CameraLag;
	FProtoSpringVector Sway;
	FProtoSpringVector SwayOffset;
	FProtoSpringVector Shake;
	FProtoSpringVector GunRecoil;
	FProtoSpringVector CameraRecoil;
	FProtoSpringFloat KickBack;
	FProtoSpringFloat LandDip;
	FProtoSpringFloat Lean;
	FProtoSpringFloat EyeHeight;

	float RecoilRecoveryScale = 1.0f;
	float AimAlpha = 0.0f;
	float LoweredAlpha = 0.0f;
	float ReloadAlpha = 0.0f;
	float BobAlpha = 0.0f;
	float BobSpeedRatio = 0.0f;
	float StridePhase = 0.0f;
	int32 LastStepIndex = 0;
	float BreathTime = 0.0f;
	float DeathAlpha = 0.0f;
	bool bDead = false;
};
