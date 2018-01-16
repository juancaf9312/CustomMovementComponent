// Fill out your copyright notice in the Description page of Project Settings.

#include "HynmersCameraComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/Engine.h"
#include "IHeadMountedDisplay.h"
#include "OculusFunctionLibrary.h"

void UHynmersCameraComponent::BeginPlay() {
	Super::BeginPlay();

	TArray<USceneComponent*> Parents;
	GetParentComponents(Parents);

	if (Parents.Num() > 0) {
		Parent = Parents[0];
	}
	

	FVector Offset;
	if (bLockToHmd && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHeadTrackingAllowed() && GetWorld()->WorldType != EWorldType::Editor) {

		GEngine->HMDDevice->UpdatePlayerCamera(StartHMDOrientation, Offset);
		Parent->SetWorldRotation(FRotator(Parent->GetComponentRotation().Pitch, StartHMDOrientation.Rotator().Yaw, StartHMDOrientation.Rotator().Roll));
		UE_LOG(LogTemp, Warning, TEXT("%s"), *StartHMDOrientation.Rotator().ToString())
	}

}

void UHynmersCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) {

	if (bLockToHmd && GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHeadTrackingAllowed() && GetWorld()->WorldType != EWorldType::Editor)
	{
		const FTransform ParentWorld = CalcNewComponentToWorld(FTransform());
		GEngine->HMDDevice->SetupLateUpdate(ParentWorld, this);

		FQuat Orientation;
		FVector Position;
		FRotator OculusOrientation;
		if (GEngine->HMDDevice->UpdatePlayerCamera(Orientation, Position))
		{
			FQuat DeltaRotation = StartHMDOrientation.Inverse() * Orientation;
			UE_LOG(LogTemp, Warning, TEXT("%s"), *DeltaRotation.Rotator().ToString());

			if (Parent){
				Parent->SetWorldRotation(DeltaRotation);
			}
			else {
				SetWorldRotation(DeltaRotation);
			}

		}
		else
		{
			ResetRelativeTransform();
		}
	}


	if (bUseAdditiveOffset)
	{
		FTransform OffsetCamToBaseCam = AdditiveOffset;
		FTransform BaseCamToWorld = GetComponentToWorld();
		FTransform OffsetCamToWorld = OffsetCamToBaseCam * BaseCamToWorld;

		DesiredView.Location = OffsetCamToWorld.GetLocation();
		DesiredView.Rotation = OffsetCamToWorld.Rotator();
	}
	else
	{
		DesiredView.Location = GetComponentLocation();
		DesiredView.Rotation = GetComponentRotation();
	}

	DesiredView.FOV = bUseAdditiveOffset ? (FieldOfView + AdditiveFOVOffset) : FieldOfView;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;

	// See if the CameraActor wants to override the PostProcess settings used.
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}

}

