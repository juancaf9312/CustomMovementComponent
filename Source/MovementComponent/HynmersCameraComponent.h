// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "HynmersCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class MOVEMENTCOMPONENT_API UHynmersCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

private:
	USceneComponent* Parent;

	FQuat StartHMDOrientation;

public:
	virtual void BeginPlay() override;

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
	
	
};
