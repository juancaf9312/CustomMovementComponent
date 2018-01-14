// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HynmersMovementComponent.generated.h"

/**
 * 
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

	virtual void PhysSwimming(float deltaTime, int32 Iterations) override;

	virtual void PhysFalling(float deltaTime, int32 Iterations) override;

	virtual bool DoJump(bool bReplayingMoves) override;
};
