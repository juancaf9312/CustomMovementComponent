// Fill out your copyright notice in the Description page of Project Settings.

#include "HynmersMovementComponent.h"

#include "GameFramework/GameStateBase.h"
#include "EngineStats.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/NetworkObjectList.h"

DECLARE_CYCLE_STAT(TEXT("Char Tick"), STAT_CharacterMovementTick, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char NonSimulated Time"), STAT_CharacterMovementNonSimulated, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PerformMovement"), STAT_CharacterMovementPerformMovement, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char RootMotionSource Calculate"), STAT_CharacterMovementRootMotionSourceCalculate, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char RootMotionSource Apply"), STAT_CharacterMovementRootMotionSourceApply, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Ch MoveAloar Physics Interation"), STAT_CharPhysicsInteraction, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysWalking"), STAT_CharPhysWalking, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysFalling"), STAT_CharPhysFalling, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char FindFloor"), STAT_CharFindFloor, STATGROUP_Character);


const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.

UHynmersMovementComponent::UHynmersMovementComponent() 
{
	PostPhysicsTickFunction.bCanEverTick = true;
	PostPhysicsTickFunction.bStartWithTickEnabled = false;
	PostPhysicsTickFunction.TickGroup = TG_PostPhysics;

	bApplyGravityWhileJumping = true;

	GravityScale = 1.f;
	GroundFriction = 8.0f;
	JumpZVelocity = 420.0f;
	JumpOffJumpZFactor = 0.5f;
	RotationRate = FRotator(0.f, 360.0f, 0.0f);
	SetWalkableFloorZ(0.71f);

	MaxStepHeight = 45.0f;
	PerchRadiusThreshold = 0.0f;
	PerchAdditionalHeight = 40.f;

	MaxFlySpeed = 600.0f;
	MaxWalkSpeed = 600.0f;
	MaxSwimSpeed = 300.0f;
	MaxCustomMovementSpeed = MaxWalkSpeed;

	MaxSimulationTimeStep = 0.05f;
	MaxSimulationIterations = 8;

	MaxDepenetrationWithGeometry = 500.f;
	MaxDepenetrationWithGeometryAsProxy = 100.f;
	MaxDepenetrationWithPawn = 100.f;
	MaxDepenetrationWithPawnAsProxy = 2.f;

	// Set to match EVectorQuantization::RoundTwoDecimals
	NetProxyShrinkRadius = 0.01f;
	NetProxyShrinkHalfHeight = 0.01f;

	NetworkSimulatedSmoothLocationTime = 0.100f;
	NetworkSimulatedSmoothRotationTime = 0.033f;
	ListenServerNetworkSimulatedSmoothLocationTime = 0.040f;
	ListenServerNetworkSimulatedSmoothRotationTime = 0.033f;
	NetworkMaxSmoothUpdateDistance = 256.f;
	NetworkNoSmoothUpdateDistance = 384.f;
	NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

	CrouchedSpeedMultiplier_DEPRECATED = 0.5f;
	MaxWalkSpeedCrouched = MaxWalkSpeed * CrouchedSpeedMultiplier_DEPRECATED;
	MaxOutOfWaterStepHeight = 40.0f;
	OutofWaterZ = 420.0f;
	AirControl = 0.05f;
	AirControlBoostMultiplier = 2.f;
	AirControlBoostVelocityThreshold = 25.f;
	FallingLateralFriction = 0.f;
	MaxAcceleration = 2048.0f;
	BrakingFrictionFactor = 2.0f; // Historical value, 1 would be more appropriate.
	BrakingDecelerationWalking = MaxAcceleration;
	BrakingDecelerationFalling = 0.f;
	BrakingDecelerationFlying = 0.f;
	BrakingDecelerationSwimming = 0.f;
	LedgeCheckThreshold = 4.0f;
	JumpOutOfWaterPitch = 11.25f;
	UpperImpactNormalScale_DEPRECATED = 0.5f;

	Mass = 100.0f;
	bJustTeleported = true;
	CrouchedHalfHeight = 40.0f;
	Buoyancy = 1.0f;
	LastUpdateRotation = FQuat::Identity;
	LastUpdateVelocity = FVector::ZeroVector;
	PendingImpulseToApply = FVector::ZeroVector;
	PendingLaunchVelocity = FVector::ZeroVector;
	DefaultWaterMovementMode = MOVE_Swimming;
	DefaultLandMovementMode = MOVE_Walking;
	bForceNextFloorCheck = true;
	bForceBraking_DEPRECATED = false;
	bShrinkProxyCapsule = true;
	bCanWalkOffLedges = true;
	bCanWalkOffLedgesWhenCrouching = false;
	bNetworkSmoothingComplete = true; // Initially true until we get a net update, so we don't try to smooth to an uninitialized value.
	bWantsToLeaveNavWalking = false;
	bIsNavWalkingOnServer = false;
	bSweepWhileNavWalking = true;

	bEnablePhysicsInteraction = true;
	StandingDownwardForceScale = 1.0f;
	InitialPushForceFactor = 500.0f;
	PushForceFactor = 750000.0f;
	PushForcePointZOffsetFactor = -0.75f;
	bPushForceUsingZOffset = false;
	bPushForceScaledToMass = false;
	bScalePushForceToVelocity = true;

	TouchForceFactor = 1.0f;
	bTouchForceScaledToMass = true;
	MinTouchForce = -1.0f;
	MaxTouchForce = 250.0f;
	RepulsionForce = 2.5f;

	bAllowPhysicsRotationDuringAnimRootMotion = false; // Old default behavior.
	bUseControllerDesiredRotation = false;

	bUseSeparateBrakingFriction = false; // Old default behavior.

	bMaintainHorizontalGroundVelocity = true;
	bImpartBaseVelocityX = true;
	bImpartBaseVelocityY = true;
	bImpartBaseVelocityZ = true;
	bImpartBaseAngularVelocity = true;
	bIgnoreClientMovementErrorChecksAndCorrection = false;
	bAlwaysCheckFloor = true;

	// default character can jump, walk, and swim
	NavAgentProps.bCanJump = true;
	NavAgentProps.bCanWalk = true;
	NavAgentProps.bCanSwim = true;
	ResetMoveState();

	ClientPredictionData = NULL;
	ServerPredictionData = NULL;

	// This should be greater than tolerated player timeout * 2.
	MinTimeBetweenTimeStampResets = 4.f * 60.f;

	bEnableScopedMovementUpdates = true;

	bRequestedMoveUseAcceleration = true;
	bUseRVOAvoidance = false;
	bUseRVOPostProcess = false;
	AvoidanceLockVelocity = FVector::ZeroVector;
	AvoidanceLockTimer = 0.0f;
	AvoidanceGroup.bGroup0 = true;
	GroupsToAvoid.Packed = 0xFFFFFFFF;
	GroupsToIgnore.Packed = 0;
	AvoidanceConsiderationRadius = 500.0f;

	OldBaseQuat = FQuat::Identity;
	OldBaseLocation = FVector::ZeroVector;

	NavMeshProjectionInterval = 0.1f;
	NavMeshProjectionInterpSpeed = 12.f;
	NavMeshProjectionHeightScaleUp = 0.67f;
	NavMeshProjectionHeightScaleDown = 1.0f;
	NavWalkingFloorDistTolerance = 10.0f;
}

void UHynmersMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) 
{
	SCOPED_NAMED_EVENT(UCharacterMovementComponent_TickComponent, FColor::Yellow);
	SCOPE_CYCLE_COUNTER(STAT_CharacterMovementTick);

	const FVector InputVector = ConsumeInputVector();
	UpVector = CharacterOwner->GetRootComponent()->GetUpVector();

	//UE_LOG(LogTemp, Warning, TEXT("Current floor normal: %s"), *CurrentFloor.HitResult.ImpactNormal.ToString())

	if (!HasValidData() || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	// See if we fell out of the world.
	const bool bIsSimulatingPhysics = UpdatedComponent->IsSimulatingPhysics();
	if (CharacterOwner->Role == ROLE_Authority && (!bCheatFlying || bIsSimulatingPhysics) && !CharacterOwner->CheckStillInWorld())
	{
		return;
	}

	AvoidanceLockTimer -= DeltaTime;

	if (CharacterOwner->Role > ROLE_SimulatedProxy)
	{
		SCOPE_CYCLE_COUNTER(STAT_CharacterMovementNonSimulated);

		// Allow root motion to move characters that have no controller.
		if (CharacterOwner->IsLocallyControlled() || (!CharacterOwner->Controller && bRunPhysicsWithNoController) || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()))
		{
			{
				SCOPE_CYCLE_COUNTER(STAT_CharUpdateAcceleration);

				// We need to check the jump state before adjusting input acceleration, to minimize latency
				// and to make sure acceleration respects our potentially new falling state.
				CharacterOwner->CheckJumpInput(DeltaTime);

				// apply input to acceleration
				Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));

				AnalogInputModifier = ComputeAnalogInputModifier();
			}

			if (CharacterOwner->Role == ROLE_Authority)
			{
				PerformMovement(DeltaTime);
			}
		}

	}

	if (bUseRVOAvoidance)
	{
		UpdateDefaultAvoidance();
	}

	if (bEnablePhysicsInteraction)
	{
		SCOPE_CYCLE_COUNTER(STAT_CharPhysicsInteraction);
		ApplyDownwardForce(DeltaTime);
		ApplyRepulsionForce(DeltaTime);
	}
}

void UHynmersMovementComponent::PerformMovement(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_CharacterMovementPerformMovement);

	if (!HasValidData())
	{
		return;
	}

	// no movement if we can't move, or if currently doing physical simulation on UpdatedComponent
	if (MovementMode == MOVE_None || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics())
	{
		UE_LOG(LogTemp, Warning, TEXT("MovementMode == MOVE_None || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics()"))
		if (!CharacterOwner->bClientUpdating && CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh() && !CharacterOwner->bServerMoveIgnoreRootMotion)
		{
			UE_LOG(LogTemp, Warning, TEXT("!CharacterOwner->bClientUpdating && CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh() && !CharacterOwner->bServerMoveIgnoreRootMotion"))
			// Consume root motion
			TickCharacterPose(DeltaSeconds);
			RootMotionParams.Clear();
			CurrentRootMotion.Clear();
		}
		// Clear pending physics forces
		ClearAccumulatedForces();
		return;
	}

	// Force floor update if we've moved outside of CharacterMovement since last update.
	bForceNextFloorCheck |= (IsMovingOnGround() && UpdatedComponent->GetComponentLocation() != LastUpdateLocation);

	// Update saved LastPreAdditiveVelocity with any external changes to character Velocity that happened since last update.
	if (CurrentRootMotion.HasAdditiveVelocity())
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentRootMotion.HasAdditiveVelocity()"))

		const FVector Adjustment = (Velocity - LastUpdateVelocity);
		CurrentRootMotion.LastPreAdditiveVelocity += Adjustment;

#if ROOT_MOTION_DEBUG
		if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnAnyThread() == 1)
		{
			if (!Adjustment.IsNearlyZero())
			{
				FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement HasAdditiveVelocity LastUpdateVelocityAdjustment LastPreAdditiveVelocity(%s) Adjustment(%s)"),
					*CurrentRootMotion.LastPreAdditiveVelocity.ToCompactString(), *Adjustment.ToCompactString());
				RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
			}
		}
#endif
	}

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		//MaybeUpdateBasedMovement(DeltaSeconds);

		// Clean up invalid RootMotion Sources.
		// This includes RootMotion sources that ended naturally.
		// They might want to perform a clamp on velocity or an override, 
		// so we want this to happen before ApplyAccumulatedForces and HandlePendingLaunch as to not clobber these.
		const bool bHasRootMotionSources = HasRootMotionSources();
		if (bHasRootMotionSources && !CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion)
		{
			UE_LOG(LogTemp, Warning, TEXT("bHasRootMotionSources && !CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion"))

			SCOPE_CYCLE_COUNTER(STAT_CharacterMovementRootMotionSourceCalculate);

			const FVector VelocityBeforeCleanup = Velocity;
			CurrentRootMotion.CleanUpInvalidRootMotion(DeltaSeconds, *CharacterOwner, *this);

#if ROOT_MOTION_DEBUG
			if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnAnyThread() == 1)
			{
				if (Velocity != VelocityBeforeCleanup)
				{
					const FVector Adjustment = Velocity - VelocityBeforeCleanup;
					FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement CleanUpInvalidRootMotion Velocity(%s) VelocityBeforeCleanup(%s) Adjustment(%s)"),
						*Velocity.ToCompactString(), *VelocityBeforeCleanup.ToCompactString(), *Adjustment.ToCompactString());
					RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
				}
			}
#endif
		}

		OldVelocity = Velocity;
		OldLocation = UpdatedComponent->GetComponentLocation();

		ApplyAccumulatedForces(DeltaSeconds);

		// Update the character state before we do our movement
		UpdateCharacterStateBeforeMovement();

		// Character::LaunchCharacter() has been deferred until now.
		HandlePendingLaunch();
		ClearAccumulatedForces();

#if ROOT_MOTION_DEBUG
		if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnAnyThread() == 1)
		{
			if (OldVelocity != Velocity)
			{
				const FVector Adjustment = Velocity - OldVelocity;
				FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement ApplyAccumulatedForces+HandlePendingLaunch Velocity(%s) OldVelocity(%s) Adjustment(%s)"),
					*Velocity.ToCompactString(), *OldVelocity.ToCompactString(), *Adjustment.ToCompactString());
				RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
			}
		}
#endif

		// Update saved LastPreAdditiveVelocity with any external changes to character Velocity that happened due to ApplyAccumulatedForces/HandlePendingLaunch
		if (CurrentRootMotion.HasAdditiveVelocity())
		{
			UE_LOG(LogTemp, Warning, TEXT("CurrentRootMotion.HasAdditiveVelocity()"))

			const FVector Adjustment = (Velocity - OldVelocity);
			CurrentRootMotion.LastPreAdditiveVelocity += Adjustment;

#if ROOT_MOTION_DEBUG
			if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnAnyThread() == 1)
			{
				if (!Adjustment.IsNearlyZero())
				{
					FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement HasAdditiveVelocity AccumulatedForces LastPreAdditiveVelocity(%s) Adjustment(%s)"),
						*CurrentRootMotion.LastPreAdditiveVelocity.ToCompactString(), *Adjustment.ToCompactString());
					RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
				}
			}
#endif
		}

		// Prepare Root Motion (generate/accumulate from root motion sources to be used later)
		if (bHasRootMotionSources && !CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion)
		{
			UE_LOG(LogTemp, Warning, TEXT("bHasRootMotionSources && !CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion"))

			// Animation root motion - If using animation RootMotion, tick animations before running physics.
			if (CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
			{
				UE_LOG(LogTemp, Warning, TEXT("CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh()"))

				TickCharacterPose(DeltaSeconds);

				// Make sure animation didn't trigger an event that destroyed us
				if (!HasValidData())
				{
					UE_LOG(LogTemp, Warning, TEXT("!HasValidData()"))

					return;
				}

				// For local human clients, save off root motion data so it can be used by movement networking code.
				if (CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy) && CharacterOwner->IsPlayingNetworkedRootMotionMontage())
				{
					UE_LOG(LogTemp, Warning, TEXT("CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy) && CharacterOwner->IsPlayingNetworkedRootMotionMontage()"))

					CharacterOwner->ClientRootMotionParams = RootMotionParams;
				}
			}

			// Generates root motion to be used this frame from sources other than animation
			{
				SCOPE_CYCLE_COUNTER(STAT_CharacterMovementRootMotionSourceCalculate);
				CurrentRootMotion.PrepareRootMotion(DeltaSeconds, *CharacterOwner, *this, true);
			}

			// For local human clients, save off root motion data so it can be used by movement networking code.
			if (CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy))
			{
				UE_LOG(LogTemp, Warning, TEXT("CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy)"))

				CharacterOwner->SavedRootMotion = CurrentRootMotion;
			}
		}

		// Apply Root Motion to Velocity
		if (CurrentRootMotion.HasOverrideVelocity() || HasAnimRootMotion())
		{
			UE_LOG(LogTemp, Warning, TEXT("CurrentRootMotion.HasOverrideVelocity() || HasAnimRootMotion()"))

			// Animation root motion overrides Velocity and currently doesn't allow any other root motion sources
			if (HasAnimRootMotion())
			{
				UE_LOG(LogTemp, Warning, TEXT("HasAnimRootMotion()"))

				// Convert to world space (animation root motion is always local)
				USkeletalMeshComponent * SkelMeshComp = CharacterOwner->GetMesh();
				if (SkelMeshComp)
				{
					UE_LOG(LogTemp, Warning, TEXT("SkelMeshComp"))

					// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
					RootMotionParams.Set(SkelMeshComp->ConvertLocalRootMotionToWorld(RootMotionParams.GetRootMotionTransform()));
				}

				// Then turn root motion to velocity to be used by various physics modes.
				if (DeltaSeconds > 0.f)
				{
					AnimRootMotionVelocity = CalcAnimRootMotionVelocity(RootMotionParams.GetRootMotionTransform().GetTranslation(), DeltaSeconds, Velocity);
					Velocity = ConstrainAnimRootMotionVelocity(AnimRootMotionVelocity, Velocity);
				}

				UE_LOG(LogTemp, Warning, TEXT("PerformMovement WorldSpaceRootMotion Translation: %s, Rotation: %s, Actor Facing: %s, Velocity: %s")
					, *RootMotionParams.GetRootMotionTransform().GetTranslation().ToCompactString()
					, *RootMotionParams.GetRootMotionTransform().GetRotation().Rotator().ToCompactString()
					, *CharacterOwner->GetActorForwardVector().ToCompactString()
					, *Velocity.ToCompactString()
				);
			}
			else
			{
				// We don't have animation root motion so we apply other sources
				if (DeltaSeconds > 0.f)
				{
					SCOPE_CYCLE_COUNTER(STAT_CharacterMovementRootMotionSourceApply);

					const FVector VelocityBeforeOverride = Velocity;
					FVector NewVelocity = Velocity;
					CurrentRootMotion.AccumulateOverrideRootMotionVelocity(DeltaSeconds, *CharacterOwner, *this, NewVelocity);
					Velocity = NewVelocity;

#if ROOT_MOTION_DEBUG
					if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnAnyThread() == 1)
					{
						if (VelocityBeforeOverride != Velocity)
						{
							FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement AccumulateOverrideRootMotionVelocity Velocity(%s) VelocityBeforeOverride(%s)"),
								*Velocity.ToCompactString(), *VelocityBeforeOverride.ToCompactString());
							RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
						}
					}
#endif
				}
			}
		}

#if ROOT_MOTION_DEBUG
		if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnAnyThread() == 1)
		{
			FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement Velocity(%s) OldVelocity(%s)"),
				*Velocity.ToCompactString(), *OldVelocity.ToCompactString());
			RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
		}
#endif

		// NaN tracking
		checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("UCharacterMovementComponent::PerformMovement: Velocity contains NaN (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

		// Clear jump input now, to allow movement events to trigger it for next update.
		CharacterOwner->ClearJumpInput();

		// change position
		StartNewPhysics(DeltaSeconds, 0);

		if (!HasValidData())
		{
			return;
		}

		// Update character state based on change from movement
		UpdateCharacterStateAfterMovement();

		if ((bAllowPhysicsRotationDuringAnimRootMotion || !HasAnimRootMotion()) && !CharacterOwner->IsMatineeControlled())
		{
			PhysicsRotation(DeltaSeconds);
		}

		// Apply Root Motion rotation after movement is complete.
		if (HasAnimRootMotion())
		{
			UE_LOG(LogTemp, Warning, TEXT("HasAnimRootMotion"))

			const FQuat OldActorRotationQuat = UpdatedComponent->GetComponentQuat();
			const FQuat RootMotionRotationQuat = RootMotionParams.GetRootMotionTransform().GetRotation();
			if (!RootMotionRotationQuat.IsIdentity())
			{
				UE_LOG(LogTemp, Warning, TEXT("!RootMotionRotationQuat.IsIdentity()"))

				const FQuat NewActorRotationQuat = RootMotionRotationQuat * OldActorRotationQuat;
				MoveUpdatedComponent(FVector::ZeroVector, NewActorRotationQuat, true);
			}

#if !(UE_BUILD_SHIPPING)
			// debug
			if (true)
			{
				const FRotator OldActorRotation = OldActorRotationQuat.Rotator();
				const FVector ResultingLocation = UpdatedComponent->GetComponentLocation();
				const FRotator ResultingRotation = UpdatedComponent->GetComponentRotation();

				// Show current position
				DrawDebugCoordinateSystem(GetWorld(), CharacterOwner->GetMesh()->GetComponentLocation() + FVector(0, 0, 1), ResultingRotation, 50.f, false);

				// Show resulting delta move.
				DrawDebugLine(GetWorld(), OldLocation, ResultingLocation, FColor::Red, true, 10.f);

				// Log details.
				UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaMove Translation: %s, Rotation: %s, MovementBase: %s"),
					*(ResultingLocation - OldLocation).ToCompactString(), *(ResultingRotation - OldActorRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()));

				const FVector RMTranslation = RootMotionParams.GetRootMotionTransform().GetTranslation();
				const FRotator RMRotation = RootMotionParams.GetRootMotionTransform().GetRotation().Rotator();
				UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaError Translation: %s, Rotation: %s"),
					*(ResultingLocation - OldLocation - RMTranslation).ToCompactString(), *(ResultingRotation - OldActorRotation - RMRotation).GetNormalized().ToCompactString());
			}
#endif // !(UE_BUILD_SHIPPING)

			// Root Motion has been used, clear
			RootMotionParams.Clear();
		}

		// consume path following requested velocity
		bHasRequestedVelocity = false;

		OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	} // End scoped movement update

	  // Call external post-movement events. These happen after the scoped movement completes in case the events want to use the current state of overlaps etc.
	CallMovementUpdateDelegate(DeltaSeconds, OldLocation, OldVelocity);

	MaybeSaveBaseLocation();
	UpdateComponentVelocity();

	const bool bHasAuthority = CharacterOwner && CharacterOwner->HasAuthority();

	// If we move we want to avoid a long delay before replication catches up to notice this change, especially if it's throttling our rate.
	if (bHasAuthority && UNetDriver::IsAdaptiveNetUpdateFrequencyEnabled() && UpdatedComponent)
	{
		const UWorld* MyWorld = GetWorld();
		if (MyWorld)
		{
			UNetDriver* NetDriver = MyWorld->GetNetDriver();
			if (NetDriver && NetDriver->IsServer())
			{
				UE_LOG(LogTemp, Warning, TEXT("NetDriver && NetDriver->IsServer()"))

				FNetworkObjectInfo* NetActor = NetDriver->GetNetworkObjectInfo(CharacterOwner);

				if (NetActor && MyWorld->GetTimeSeconds() <= NetActor->NextUpdateTime && NetDriver->IsNetworkActorUpdateFrequencyThrottled(*NetActor))
				{
					UE_LOG(LogTemp, Warning, TEXT("NetActor && MyWorld->GetTimeSeconds() <= NetActor->NextUpdateTime"))

					if (ShouldCancelAdaptiveReplication())
					{
						UE_LOG(LogTemp, Warning, TEXT("ShouldCancelAdaptiveReplication"))

						NetDriver->CancelAdaptiveReplication(*NetActor);
					}
				}
			}
		}
	}

	const FVector NewLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
	const FQuat NewRotation = UpdatedComponent ? UpdatedComponent->GetComponentQuat() : FQuat::Identity;

	if (bHasAuthority && UpdatedComponent && !IsNetMode(NM_Client))
	{
		const bool bLocationChanged = (NewLocation != LastUpdateLocation);
		const bool bRotationChanged = (NewRotation != LastUpdateRotation);
		if (bLocationChanged || bRotationChanged)
		{
			const UWorld* MyWorld = GetWorld();
			ServerLastTransformUpdateTimeStamp = MyWorld ? MyWorld->GetTimeSeconds() : 0.f;
		}
	}

	LastUpdateLocation = NewLocation;
	LastUpdateRotation = NewRotation;
	LastUpdateVelocity = Velocity;
}

void UHynmersMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	SCOPE_CYCLE_COUNTER(STAT_CharPhysWalking);

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->Role != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsQueryCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN before Iteration (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->Role == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent * const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Root motion is not updated. All is false
		//RestorePreAdditiveRootMotionVelocity();

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;
		Acceleration -= (Acceleration | UpVector)*UpVector;

		// Apply acceleration
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			// Funciron importante para calcular la velocidad
			CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());
			checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after CalcVelocity (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));
		}

		ApplyRootMotionToVelocity(timeTick);
		checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after Root Motion application (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));
		//UE_LOG(LogTemp, Warning, TEXT("PhysWalking: %s"), *Velocity.ToString());

		if (IsFalling())
		{
			// Root motion could have put us into Falling.
			// No movement has taken place this movement tick so we pass on full time/past iteration count
			StartNewPhysics(remainingTime + timeTick, Iterations - 1);
			return;
		}

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}

		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					CharacterOwner->OnWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}

			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}

		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move
			if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
			}
		}
		//UE_LOG(LogTemp, Warning, TEXT("Velocity after: %s"), *Velocity.ToString());
		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}

void UHynmersMovementComponent::MoveAlongFloor(const FVector & InVelocity, float DeltaSeconds, FStepDownResult * OutStepDownResult)
{
	if (!CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	// Move along the current floor
	// Have to changed  to avoid z clamping. Have to make UpVector Clamping
	const FVector Delta = (InVelocity - (InVelocity | UpVector)*UpVector) * DeltaSeconds;

	FHitResult Hit(1.f);
	FVector RampVector = ComputeGroundMovementDelta(Delta, CurrentFloor.HitResult, CurrentFloor.bLineTrace);
	SafeMoveUpdatedComponent(RampVector, UpdatedComponent->GetComponentQuat(), true, Hit);
	
	float LastMoveTimeSlice = DeltaSeconds;

	if (Hit.bStartPenetrating)
	{
		UE_LOG(LogTemp, Warning, TEXT("bStartPenetrating = %d"), Hit.bStartPenetrating);
		// Allow this hit to be used as an impact we can deflect off, otherwise we do nothing the rest of the update and appear to hitch.
		HandleImpact(Hit);
		SlideAlongSurface(Delta, 1.f, Hit.Normal, Hit, true);

		if (Hit.bStartPenetrating)
		{
			OnCharacterStuckInGeometry(&Hit);
		}
	}
	else if (Hit.IsValidBlockingHit())
	{
		UE_LOG(LogTemp, Warning, TEXT("IsValidBlockingHit"));

		// We impacted something (most likely another ramp, but possibly a barrier).
		float PercentTimeApplied = Hit.Time;
		if ((Hit.Time > 0.f) && (Hit.Normal.Z > KINDA_SMALL_NUMBER) && IsWalkable(Hit))
		{
			// Another walkable ramp.
			const float InitialPercentRemaining = 1.f - PercentTimeApplied;
			RampVector = ComputeGroundMovementDelta(Delta * InitialPercentRemaining, Hit, false);
			LastMoveTimeSlice = InitialPercentRemaining * LastMoveTimeSlice;
			SafeMoveUpdatedComponent(RampVector, UpdatedComponent->GetComponentQuat(), true, Hit);

			const float SecondHitPercent = Hit.Time * InitialPercentRemaining;
			PercentTimeApplied = FMath::Clamp(PercentTimeApplied + SecondHitPercent, 0.f, 1.f);
		}

		if (Hit.IsValidBlockingHit())
		{
			if (CanStepUp(Hit) || (CharacterOwner->GetMovementBase() != NULL && CharacterOwner->GetMovementBase()->GetOwner() == Hit.GetActor()))
			{
				// hit a barrier, try to step up
				const FVector GravDir(0.f, 0.f, -1.f);
				if (!StepUp(GravDir, Delta * (1.f - PercentTimeApplied), Hit, OutStepDownResult))
				{
					//UE_LOG(LogCharacterMovement, Verbose, TEXT("- StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					HandleImpact(Hit, LastMoveTimeSlice, RampVector);
					SlideAlongSurface(Delta, 1.f - PercentTimeApplied, Hit.Normal, Hit, true);
				}
				else
				{
					// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
					//UE_LOG(LogCharacterMovement, Verbose, TEXT("+ StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					bJustTeleported |= !bMaintainHorizontalGroundVelocity;
				}
			}
			else if (Hit.Component.IsValid() && !Hit.Component.Get()->CanCharacterStepUp(CharacterOwner))
			{
				HandleImpact(Hit, LastMoveTimeSlice, RampVector);
				SlideAlongSurface(Delta, 1.f - PercentTimeApplied, Hit.Normal, Hit, true);
			}
		}
	}
}

FVector UHynmersMovementComponent::ConstrainInputAcceleration(const FVector & InputAcceleration) const
{
	if ((InputAcceleration | UpVector) != 0.f && (IsMovingOnGround() || IsFalling()))
	{
		return InputAcceleration - (InputAcceleration | UpVector)*UpVector;
	}
	return InputAcceleration;
}

void UHynmersMovementComponent::MaintainHorizontalGroundVelocity()
{
	if ((Velocity | UpVector) != 0.f && bMaintainHorizontalGroundVelocity)
	{
		// Ramp movement already maintained the velocity, so we just want to remove the vertical component.
		Velocity -= (Velocity | UpVector)*UpVector;

	}
}

FVector UHynmersMovementComponent::ComputeGroundMovementDelta(const FVector & Delta, const FHitResult & RampHit, const bool bHitFromLineTrace) const
{
	const FVector FloorNormal = RampHit.ImpactNormal;
	const FVector ContactNormal = RampHit.Normal;

	if ((FloorNormal | UpVector) < (1.f - KINDA_SMALL_NUMBER) && (FloorNormal | UpVector) > KINDA_SMALL_NUMBER && (ContactNormal | UpVector) > KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit))
	{

		// Compute a vector that moves parallel to the surface, by projecting the horizontal movement direction onto the ramp.
		const float FloorDotDelta = (FloorNormal | Delta);
		FVector RightVector = CharacterOwner->GetRootComponent()->GetRightVector();
		FVector ForwardVector = CharacterOwner->GetRootComponent()->GetForwardVector();
		FVector RampMovement = (Delta | RightVector)*RightVector + (Delta | ForwardVector)*ForwardVector + (-FloorDotDelta / (FloorNormal | UpVector))*UpVector;

		if (bMaintainHorizontalGroundVelocity)
		{
			return RampMovement;
		}
		else
		{
			return RampMovement.GetSafeNormal() * Delta.Size();
		}
	}

	return Delta;
}

bool UHynmersMovementComponent::SafeMoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult& OutHit, ETeleportType Teleport)
{
	if (UpdatedComponent == NULL)
	{
		OutHit.Reset(1.f);
		return false;
	}

	bool bMoveResult = false;

	// Scope for move flags
	{
		// Conditionally ignore blocking overlaps (based on CVar)
		const EMoveComponentFlags IncludeBlockingOverlapsWithoutEvents = (MOVECOMP_NeverIgnoreBlockingOverlaps | MOVECOMP_DisableBlockingOverlapDispatch);
		bMoveResult = MoveUpdatedComponent(Delta, NewRotation, bSweep, &OutHit, Teleport);
	}

	// Handle initial penetrations
	if (OutHit.bStartPenetrating && UpdatedComponent)
	{
		const FVector RequestedAdjustment = GetPenetrationAdjustment(OutHit);
		if (ResolvePenetration(RequestedAdjustment, OutHit, NewRotation))
		{
			// Retry original move
			bMoveResult = MoveUpdatedComponent(Delta, NewRotation, bSweep, &OutHit, Teleport);
		}
	}

	return bMoveResult;
}

bool UHynmersMovementComponent::MoveUpdatedComponent(const FVector & Delta, const FQuat & NewRotation, bool bSweep, FHitResult * OutHit, ETeleportType Teleport)
{
	if (UpdatedComponent)
	{
		if (bConstrainToPlane)PlaneConstraintNormal = CurrentFloor.HitResult.ImpactNormal;

		const FVector NewDelta = ConstrainDirectionToPlane(Delta);
		return UpdatedComponent->MoveComponent(Delta, NewRotation, bSweep, OutHit, MoveComponentFlags, Teleport);
	}

	return false;
}

void UHynmersMovementComponent::PhysSwimming(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	float NetFluidFriction = 0.f;
	float Depth = ImmersionDepth();
	float NetBuoyancy = Buoyancy * Depth;
	float OriginalAccelZ = Acceleration.Z;
	bool bLimitedUpAccel = false;

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (Velocity.Z > 0.33f * MaxSwimSpeed) && (NetBuoyancy != 0.f))
	{
		//damp positive Z out of water
		Velocity.Z = FMath::Max(0.33f * MaxSwimSpeed, Velocity.Z * Depth*Depth);
	}
	else if (Depth < 0.65f)
	{
		bLimitedUpAccel = (Acceleration.Z > 0.f);
		Acceleration.Z = FMath::Min(0.1f, Acceleration.Z);
	}

	Iterations++;
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	bJustTeleported = false;
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction * Depth;
		CalcVelocity(deltaTime, Friction, true, GetMaxBrakingDeceleration());
		Velocity.Z += GetGravityZ() * deltaTime * (1.f - NetBuoyancy);
	}

	ApplyRootMotionToVelocity(deltaTime);

	FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	float remainingTime = deltaTime * Swim(Adjusted, Hit);

	//may have left water - if so, script might have set new physics mode
	if (!IsSwimming())
	{
		StartNewPhysics(remainingTime, Iterations);
		return;
	}

	if (Hit.Time < 1.f && CharacterOwner)
	{
		HandleSwimmingWallHit(Hit, deltaTime);
		if (bLimitedUpAccel && (Velocity.Z >= 0.f))
		{
			// allow upward velocity at surface if against obstacle
			Velocity.Z += OriginalAccelZ * deltaTime;
			Adjusted = Velocity * (1.f - Hit.Time)*deltaTime;
			Swim(Adjusted, Hit);
			if (!IsSwimming())
			{
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
		}

		const FVector GravDir = FVector(0.f, 0.f, -1.f);
		const FVector VelDir = Velocity.GetSafeNormal();
		const float UpDown = GravDir | VelDir;

		bool bSteppedUp = false;
		if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		{
			float stepZ = UpdatedComponent->GetComponentLocation().Z;
			const FVector RealVelocity = Velocity;
			Velocity.Z = 1.f;	// HACK: since will be moving up, in case pawn leaves the water
			bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				//may have left water - if so, script might have set new physics mode
				if (!IsSwimming())
				{
					StartNewPhysics(remainingTime, Iterations);
					return;
				}
				OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - stepZ);
			}
			Velocity = RealVelocity;
		}

		if (!bSteppedUp)
		{
			//adjust and try again
			HandleImpact(Hit, deltaTime, Adjusted);
			SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		}
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && !bJustTeleported && ((deltaTime - remainingTime) > KINDA_SMALL_NUMBER) && CharacterOwner)
	{
		bool bWaterJump = !GetPhysicsVolume()->bWaterVolume;
		float velZ = Velocity.Z;
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / (deltaTime - remainingTime);
		if (bWaterJump)
		{
			Velocity.Z = velZ;
		}
	}

	if (!GetPhysicsVolume()->bWaterVolume && IsSwimming())
	{
		SetMovementMode(MOVE_Falling); //in case script didn't change it (w/ zone change)
	}

	//may have left water - if so, script might have set new physics mode
	if (!IsSwimming())
	{
		StartNewPhysics(remainingTime, Iterations);
	}
}

void UHynmersMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	//SCOPE_CYCLE_COUNTER(STAT_CharPhysFalling);

	//if (deltaTime < MIN_TICK_TIME)
	//{
	//	return;
	//}

	//FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
	//FallAcceleration.Z = 0.f;
	//const bool bHasAirControl = (FallAcceleration.SizeSquared2D() > 0.f);

	//float remainingTime = deltaTime;
	//while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
	//{
	//	Iterations++;
	//	const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
	//	remainingTime -= timeTick;

	//	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	//	const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
	//	bJustTeleported = false;

	//	RestorePreAdditiveRootMotionVelocity();

	//	FVector OldVelocity = Velocity;
	//	FVector VelocityNoAirControl = Velocity;

	//	// Apply input
	//	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	//	{
	//		const float MaxDecel = GetMaxBrakingDeceleration();
	//		// Compute VelocityNoAirControl
	//		if (bHasAirControl)
	//		{
	//			// Find velocity *without* acceleration.
	//			TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
	//			TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);
	//			Velocity.Z = 0.f;
	//			CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
	//			VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
	//		}

	//		// Compute Velocity
	//		{
	//			// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
	//			TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
	//			Velocity.Z = 0.f;
	//			CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
	//			Velocity.Z = OldVelocity.Z;
	//		}

	//		// Just copy Velocity to VelocityNoAirControl if they are the same (ie no acceleration).
	//		if (!bHasAirControl)
	//		{
	//			VelocityNoAirControl = Velocity;
	//		}
	//	}

	//	// Apply gravity
	//	const FVector Gravity(0.f, 0.f, GetGravityZ());
	//	Velocity = NewFallVelocity(Velocity, Gravity, timeTick);
	//	VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, timeTick);
	//	const FVector AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;

	//	ApplyRootMotionToVelocity(timeTick);

	//	if (bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f))
	//	{
	//		// Just passed jump apex since now going down
	//		bNotifyApex = false;
	//		NotifyJumpApex();
	//	}


	//	// Move
	//	FHitResult Hit(1.f);
	//	FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;
	//	SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

	//	if (!HasValidData())
	//	{
	//		return;
	//	}

	//	float LastMoveTimeSlice = timeTick;
	//	float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

	//	if (IsSwimming()) //just entered water
	//	{
	//		remainingTime += subTimeTickRemaining;
	//		StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
	//		return;
	//	}
	//	else if (Hit.bBlockingHit)
	//	{
	//		if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
	//		{
	//			remainingTime += subTimeTickRemaining;
	//			ProcessLanded(Hit, remainingTime, Iterations);
	//			return;
	//		}
	//		else
	//		{
	//			// Compute impact deflection based on final velocity, not integration step.
	//			// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
	//			Adjusted = Velocity * timeTick;

	//			// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
	//			if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
	//			{
	//				const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
	//				FFindFloorResult FloorResult;
	//				FindFloor(PawnLocation, FloorResult, false);
	//				if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
	//				{
	//					remainingTime += subTimeTickRemaining;
	//					ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
	//					return;
	//				}
	//			}

	//			HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

	//			// If we've changed physics mode, abort.
	//			if (!HasValidData() || !IsFalling())
	//			{
	//				return;
	//			}

	//			// Limit air control based on what we hit.
	//			// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
	//			if (bHasAirControl)
	//			{
	//				const bool bCheckLandingSpot = false; // we already checked above.
	//				const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
	//				Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
	//			}

	//			const FVector OldHitNormal = Hit.Normal;
	//			const FVector OldHitImpactNormal = Hit.ImpactNormal;
	//			FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

	//			// Compute velocity after deflection (only gravity component for RootMotion)
	//			if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
	//			{
	//				const FVector NewVelocity = (Delta / subTimeTickRemaining);
	//				Velocity = HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
	//			}

	//			if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
	//			{
	//				// Move in deflected direction.
	//				SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

	//				if (Hit.bBlockingHit)
	//				{
	//					// hit second wall
	//					LastMoveTimeSlice = subTimeTickRemaining;
	//					subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

	//					if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
	//					{
	//						remainingTime += subTimeTickRemaining;
	//						ProcessLanded(Hit, remainingTime, Iterations);
	//						return;
	//					}

	//					HandleImpact(Hit, LastMoveTimeSlice, Delta);

	//					// If we've changed physics mode, abort.
	//					if (!HasValidData() || !IsFalling())
	//					{
	//						return;
	//					}

	//					// Act as if there was no air control on the last move when computing new deflection.
	//					if (bHasAirControl && Hit.Normal.Z > VERTICAL_SLOPE_NORMAL_Z)
	//					{
	//						const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
	//						Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
	//					}

	//					FVector PreTwoWallDelta = Delta;
	//					TwoWallAdjust(Delta, Hit, OldHitNormal);

	//					// Limit air control, but allow a slide along the second wall.
	//					if (bHasAirControl)
	//					{
	//						const bool bCheckLandingSpot = false; // we already checked above.
	//						const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

	//						// Only allow if not back in to first wall
	//						if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
	//						{
	//							Delta += (AirControlDeltaV * subTimeTickRemaining);
	//						}
	//					}

	//					// Compute velocity after deflection (only gravity component for RootMotion)
	//					if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
	//					{
	//						const FVector NewVelocity = (Delta / subTimeTickRemaining);
	//						Velocity = HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
	//					}

	//					// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
	//					bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
	//					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
	//					if (Hit.Time == 0.f)
	//					{
	//						// if we are stuck then try to side step
	//						FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
	//						if (SideDelta.IsNearlyZero())
	//						{
	//							SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
	//						}
	//						SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
	//					}

	//					if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f)
	//					{
	//						remainingTime = 0.f;
	//						ProcessLanded(Hit, remainingTime, Iterations);
	//						return;
	//					}
	//					else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
	//					{
	//						// We might be in a virtual 'ditch' within our perch radius. This is rare.
	//						const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
	//						const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
	//						const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
	//						if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
	//						{
	//							Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
	//							Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
	//							Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
	//							Delta = Velocity * timeTick;
	//							SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}

	//	if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
	//	{
	//		Velocity.X = 0.f;
	//		Velocity.Y = 0.f;
	//	}
	//}
}

bool UHynmersMovementComponent::DoJump(bool bReplayingMoves)
{
	//UE_LOG(LogTemp, Warning, TEXT("JUMP_TIME: %d"), GetWorld()->GetTimeSeconds())
	//if (CharacterOwner && CharacterOwner->CanJump())
	//{
	//	// Don't jump if we can't move up/down.
	//	if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
	//	{
	//		Velocity.Z = JumpZVelocity;
	//		SetMovementMode(MOVE_Falling);
	//		return true;
	//	}
	//}

	return false;
}

void UHynmersMovementComponent::FindFloor(const FVector & CapsuleLocation, FFindFloorResult & OutFloorResult, bool bZeroDelta, const FHitResult * DownwardSweepResult) const
{
	SCOPE_CYCLE_COUNTER(STAT_CharFindFloor);
	// No collision, no floor...
	if (!HasValidData() || !UpdatedComponent->IsQueryCollisionEnabled())
	{
		OutFloorResult.Clear();
		return;
	}

	check(CharacterOwner->GetCapsuleComponent());

	// Increase height check slightly if walking, to prevent floor height adjustment from later invalidating the floor result.
	const float HeightCheckAdjust = (IsMovingOnGround() ? MAX_FLOOR_DIST + KINDA_SMALL_NUMBER : -MAX_FLOOR_DIST);

	float FloorSweepTraceDist = FMath::Max(MAX_FLOOR_DIST, MaxStepHeight + HeightCheckAdjust);
	float FloorLineTraceDist = FloorSweepTraceDist;

	// Sweep floor
	if (FloorLineTraceDist > 0.f || FloorSweepTraceDist > 0.f)
	{
		UHynmersMovementComponent* MutableThis = const_cast<UHynmersMovementComponent*>(this);

		if (bAlwaysCheckFloor || !bZeroDelta || bForceNextFloorCheck || bJustTeleported)
		{
			MutableThis->bForceNextFloorCheck = false;
			ComputeFloorDist(CapsuleLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), DownwardSweepResult);
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Please enable AlwaysCheckFloor"))
		}
	}

}

void UHynmersMovementComponent::ComputeFloorDist(const FVector & CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult & OutFloorResult, float SweepRadius, const FHitResult * DownwardSweepResult) const
{
	OutFloorResult.Clear();

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	bool bSkipSweep = false;
	//if (DownwardSweepResult != NULL && DownwardSweepResult->IsValidBlockingHit())
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("TIME_%d: DownwardSweepResult different from null"),GetWorld()->GetTimeSeconds())
	//	// Only if the supplied sweep was vertical and downward.
	//	if ((DownwardSweepResult->TraceStart.Z > DownwardSweepResult->TraceEnd.Z) &&
	//		(DownwardSweepResult->TraceStart - DownwardSweepResult->TraceEnd).SizeSquared2D() <= KINDA_SMALL_NUMBER)
	//	{
	//		// Reject hits that are barely on the cusp of the radius of the capsule
	//		if (IsWithinEdgeTolerance(DownwardSweepResult->Location, DownwardSweepResult->ImpactPoint, PawnRadius))
	//		{
	//			// Don't try a redundant sweep, regardless of whether this sweep is usable.
	//			bSkipSweep = true;
	//			const bool bIsWalkable = IsWalkable(*DownwardSweepResult);
	//			const float FloorDist = (CapsuleLocation.Z - DownwardSweepResult->Location.Z);
	//			OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);
	//			if (bIsWalkable)
	//			{
	//				// Use the supplied downward sweep as the floor hit result.			
	//				return;
	//			}
	//		}
	//	}
	//}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		ensure(SweepDistance >= LineDistance);
		return;
	}

	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComputeFloorDist), false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test
	if (!bSkipSweep && SweepDistance > 0.f && SweepRadius > 0.f)
	{
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		const float ShrinkScale = 0.9f;
		const float ShrinkScaleOverlap = 0.1f;
		float ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScale);
		float TraceDist = SweepDistance + ShrinkHeight;
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.f);
		bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation - CharacterOwner->GetRootComponent()->GetUpVector()*TraceDist , CollisionChannel, CapsuleShape, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			if (Hit.bStartPenetrating || !IsWithinEdgeTolerance(CapsuleLocation, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			{
				// Use a capsule with a slightly smaller radius and shorter height to avoid the adjacent object.
				// Capsule must not be nearly zero or the trace will fall back to a line trace from the start point and have the wrong length.
				CapsuleShape.Capsule.Radius = FMath::Max(0.f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - KINDA_SMALL_NUMBER);
				if (!CapsuleShape.IsNearlyZero())
				{
					ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScaleOverlap);
					TraceDist = SweepDistance + ShrinkHeight;
					CapsuleShape.Capsule.HalfHeight = FMath::Max(PawnHalfHeight - ShrinkHeight, CapsuleShape.Capsule.Radius);
					Hit.Reset(1.f, false);

					bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation - CharacterOwner->GetRootComponent()->GetUpVector()*TraceDist, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
				}
			}

			// Reduce hit distance by ShrinkHeight because we shrank the capsule for the trace.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			if (Hit.IsValidBlockingHit() && IsWalkable(Hit))
			{
				if (SweepResult <= SweepDistance)
				{
					// Hit within test distance.
					OutFloorResult.bWalkableFloor = true;
					return;
				}
			}
		}
	}

	// Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
	// We do however want to try a line trace if the sweep was stuck in penetration.
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		OutFloorResult.FloorDist = SweepDistance;
		return;
	}

	// Line trace
	if (LineDistance > 0.f)
	{
		const float ShrinkHeight = PawnHalfHeight;
		const FVector LineTraceStart = CapsuleLocation;
		const float TraceDist = LineDistance + ShrinkHeight;
		const FVector Down = -CharacterOwner->GetRootComponent()->GetUpVector();
		QueryParams.TraceTag = SCENE_QUERY_STAT_NAME_ONLY(FloorLineTrace);

		FHitResult Hit(1.f);
		bBlockingHit = GetWorld()->LineTraceSingleByChannel(Hit, LineTraceStart, LineTraceStart + Down, CollisionChannel, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			if (Hit.Time > 0.f)
			{
				// Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
				// We allow negative distances here, because this allows us to pull out of penetrations.
				const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
				const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

				OutFloorResult.bBlockingHit = true;
				if (LineResult <= LineDistance && IsWalkable(Hit))
				{
					OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
					return;
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No walkable floor found"))
		// No hits were acceptable.
		OutFloorResult.bWalkableFloor = false;
	OutFloorResult.FloorDist = SweepDistance;
}

bool UHynmersMovementComponent::FloorSweepTest(FHitResult & OutHit, const FVector & Start, const FVector & End,
	ECollisionChannel TraceChannel, const FCollisionShape & CollisionShape, const FCollisionQueryParams & Params,
	const FCollisionResponseParams & ResponseParam) const
{
	bool bBlockingHit = false;

	if (!bUseFlatBaseForFloorChecks)
	{
		bBlockingHit = GetWorld()->SweepSingleByChannel(OutHit, Start, End, CharacterOwner->GetRootComponent()->GetComponentQuat(), TraceChannel, CollisionShape, Params, ResponseParam);
	}
	else
	{
		// Test with a box that is enclosed by the capsule.
		const float CapsuleRadius = CollisionShape.GetCapsuleRadius();
		const float CapsuleHeight = CollisionShape.GetCapsuleHalfHeight();
		const FCollisionShape BoxShape = FCollisionShape::MakeBox(FVector(CapsuleRadius * 0.707f, CapsuleRadius * 0.707f, CapsuleHeight));

		// First test with the box rotated so the corners are along the major axes (ie rotated 45 degrees).
		bBlockingHit = GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat(FVector(0.f, 0.f, -1.f), PI * 0.25f), TraceChannel, BoxShape, Params, ResponseParam);

		if (!bBlockingHit)
		{
			// Test again with the same box, not rotated.
			OutHit.Reset(1.f, false);
			bBlockingHit = GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, TraceChannel, BoxShape, Params, ResponseParam);
		}
	}

	return bBlockingHit;
}
