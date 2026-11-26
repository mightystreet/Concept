// Small damped spring helpers used by the procedural first-person rig.

#pragma once

#include "CoreMinimal.h"

namespace ProtoSpring
{
	/** Largest step the integrator takes; larger frame times are sub-stepped for stability. */
	inline constexpr float MaxSubStep = 1.0f / 120.0f;
	/** Frame times above this are clamped (hitches, breakpoints). */
	inline constexpr float MaxFrameTime = 0.1f;

	template <typename T>
	void Step(T& Value, T& Velocity, const T& Target, float Stiffness, float Damping, float DeltaTime)
	{
		float Remaining = FMath::Min(DeltaTime, MaxFrameTime);
		while (Remaining > UE_KINDA_SMALL_NUMBER)
		{
			const float Dt = FMath::Min(Remaining, MaxSubStep);
			const T Accel = (Target - Value) * Stiffness - Velocity * Damping;
			Velocity += Accel * Dt;
			Value += Velocity * Dt;
			Remaining -= Dt;
		}
	}
}

/** Damped spring on a float. */
struct FProtoSpringFloat
{
	float Value = 0.0f;
	float Velocity = 0.0f;

	void Step(float Target, float Stiffness, float Damping, float DeltaTime)
	{
		ProtoSpring::Step(Value, Velocity, Target, Stiffness, Damping, DeltaTime);
	}
};

/** Damped spring on a vector. Also used for rotations stored as (Pitch, Yaw, Roll). */
struct FProtoSpringVector
{
	FVector Value = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;

	void Step(const FVector& Target, float Stiffness, float Damping, float DeltaTime)
	{
		ProtoSpring::Step(Value, Velocity, Target, Stiffness, Damping, DeltaTime);
	}

	FRotator AsRotator() const { return FRotator(Value.X, Value.Y, Value.Z); }
};
