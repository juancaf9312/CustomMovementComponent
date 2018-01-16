// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HynmersCharacter.h"
#include "HynmersCameraComponent.h"
#include "HynmersMovementComponent.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Kismet/KismetSystemLibrary.h"



DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AHynmersCharacter

AHynmersCharacter::AHynmersCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UHynmersMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh = GetMesh();
	Mesh->SetOnlyOwnerSee(true);
	Mesh->SetupAttachment(GetCapsuleComponent());
	Mesh->bCastDynamicShadow = false;
	Mesh->CastShadow = false;
	//Mesh->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	//Mesh->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a CameraComponent	
	RootCameraComponent = CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonCameraRoot"));
	RootCameraComponent->SetupAttachment(Mesh, FName("CameraPoint"));
	RootCameraComponent->RelativeRotation = FRotator(-6.5f, 56.f, -90.f);

	FirstPersonCameraComponent = CreateDefaultSubobject<UHynmersCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootCameraComponent);
	FirstPersonCameraComponent->RelativeLocation = FVector(16.65f, 0.8f, 15.53f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	
	
	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->Hand = EControllerHand::Right;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	}

void AHynmersCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void AHynmersCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//FirstPersonCameraComponent->SetWorldRotation(FRotator(-6.5f, 56.f, -90.f));
	//UE_LOG(LogTemp,Warning,TEXT("Im ticking"))
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHynmersCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Exit", IE_Pressed, this, &AHynmersCharacter::OnExit);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHynmersCharacter::OnResetVR);

	PlayerInputComponent->BindAxis("MoveForward", this, &AHynmersCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHynmersCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHynmersCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHynmersCharacter::LookUpAtRate);
}


void AHynmersCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHynmersCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AHynmersCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AHynmersCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHynmersCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AHynmersCharacter::OnExit(void)
{
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit);
}
