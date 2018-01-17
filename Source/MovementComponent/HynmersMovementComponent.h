// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HynmersMovementComponent.generated.h"

/*
  Revision:
	TickComponent
		PerformMovement
			PhysWalking
				MoveAlongthefloor
					Ramp

 */
UCLASS()
class MOVEMENTCOMPONENT_API UHynmersMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UHynmersMovementComponent();
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void PerformMovement(float DeltaSeconds) override;

	//Movements
	virtual void PhysWalking(float deltaTime, int32 Iterations) override;

	// Function in charged of move the updated component
	virtual void MoveAlongFloor(const FVector& InVelocity, float DeltaSeconds, FStepDownResult* OutStepDownResult = NULL) override;

	// function that clamp acceleration in Z
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const override;

	// Function that clamps velocity in Z
	virtual void MaintainHorizontalGroundVelocity() override;

	virtual void PhysSwimming(float deltaTime, int32 Iterations) override;

	virtual void PhysFalling(float deltaTime, int32 Iterations) override;

	virtual bool DoJump(bool bReplayingMoves) override;

	// Floor Finding functions
	virtual void FindFloor(const FVector& CapsuleLocation, FFindFloorResult& OutFloorResult, bool bZeroDelta, const FHitResult* DownwardSweepResult = NULL) const override;

	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = NULL) const override;

	virtual bool FloorSweepTest(struct FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionShape& CollisionShape,
		const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam) const override;

	virtual bool IsWalkable(const FHitResult& Hit) const override { return true; };

private:
	FVector UpVector;
};
