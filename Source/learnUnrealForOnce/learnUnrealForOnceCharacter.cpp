// Copyright Epic Games, Inc. All Rights Reserved.

#include "learnUnrealForOnceCharacter.h"

#include "DrawDebugHelpers.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AlearnUnrealForOnceCharacter

AlearnUnrealForOnceCharacter::AlearnUnrealForOnceCharacter()
{
	LastClick = 0;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	GrabbedObjectLocation = CreateDefaultSubobject<USceneComponent>(TEXT("GrabbedObjectLocation"));
	GrabbedObjectLocation->SetupAttachment(FollowCamera);

	DistanceMultiplier = 50.f;
}

#pragma region GrabRelease

void AlearnUnrealForOnceCharacter::GrabRelease(float Rate)
{
	if (Rate == 1 && LastClick == 0 && !Grabbing)
	{
		OnPickup();
		LastClick = Rate;
		return;
	}

	if (Rate == 1 && LastClick == 0 && Grabbing)
	{
		OnDrop();
		LastClick = Rate;
		return;
	}

	if(Grabbing) SetGrabbedObject(GrabbedObject);
	LastClick = Rate;
}

void AlearnUnrealForOnceCharacter::OnPickup()
{
	const FCollisionQueryParams QueryParams("GravityGunTrace", false, this);
	const float TraceRange = 250000.f;
	const FVector StartTrace = FollowCamera->GetComponentLocation();
	const FVector EndTrace = (FollowCamera->GetForwardVector() * TraceRange) + StartTrace;
	FHitResult Hit;

	if (GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_Visibility, QueryParams))
	{
		if (UPrimitiveComponent* Prim = Hit.GetComponent())
		{
			if (Prim->IsSimulatingPhysics())
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Prim->GetPhysicsAngularVelocityInDegrees().ToString());

				Prim->AddImpulse(FVector(0, 0, 1000) * Prim->GetMass());
				Prim->AddTorqueInDegrees(FVector(1000, 100, 100));

				GrabbedObjectLocation->SetWorldLocation((GetFollowCamera()->GetForwardVector() * Hit.Distance) + GrabbedObjectLocation->GetComponentLocation());

				/*
				//Cancel movement
				Prim->AddImpulse(-(Prim->GetComponentVelocity() * Prim->GetMass()));

				SetGrabbedObject(Prim);

				Grabbing = true;
				*/
			}
		}
	}
}

void AlearnUnrealForOnceCharacter::OnDrop()
{
	if (GrabbedObject)
	{
		GrabbedObjectLocation->SetRelativeLocation(FVector(0, 0, 0));
		GrabbedObject->SetEnableGravity(true);

		SetGrabbedObject(nullptr);

		Grabbing = false;
	}
}

void AlearnUnrealForOnceCharacter::SetGrabbedObject(UPrimitiveComponent* ObjectToGrab)
{
	GrabbedObject = ObjectToGrab;
	if (GrabbedObject)
	{
		if(GrabbedObject->IsGravityEnabled()) GrabbedObject->SetEnableGravity(false);
		GrabbedObject->SetWorldLocation(GrabbedObjectLocation->GetComponentLocation(), true);
	}
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// Input

void AlearnUnrealForOnceCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind Push/Pull actions
	PlayerInputComponent->BindAxis("PushPull", this, &AlearnUnrealForOnceCharacter::PushPull);

	// Bind Grab/Release actions
	PlayerInputComponent->BindAxis("GrabRelease", this, &AlearnUnrealForOnceCharacter::GrabRelease);

	// Bind mouse scroll
	PlayerInputComponent->BindAction("DistanceIncrease", IE_Pressed, this, &AlearnUnrealForOnceCharacter::ScrollUp);
	PlayerInputComponent->BindAction("DistanceDecrease", IE_Pressed, this, &AlearnUnrealForOnceCharacter::ScrollDown);
	
	// Bind movement Axis
	PlayerInputComponent->BindAxis("MoveForward", this, &AlearnUnrealForOnceCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AlearnUnrealForOnceCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AlearnUnrealForOnceCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AlearnUnrealForOnceCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AlearnUnrealForOnceCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AlearnUnrealForOnceCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AlearnUnrealForOnceCharacter::OnResetVR);
}

#pragma region Scroll distance

void AlearnUnrealForOnceCharacter::ScrollUp()
{
	if (Grabbing)
	{
		GrabbedObjectLocation->SetWorldLocation((GetFollowCamera()->GetForwardVector() * DistanceMultiplier) + GrabbedObjectLocation->GetComponentLocation());
	}
}

void AlearnUnrealForOnceCharacter::ScrollDown()
{
	if (Grabbing)
	{
		GrabbedObjectLocation->SetWorldLocation((GetFollowCamera()->GetForwardVector() * -DistanceMultiplier) + GrabbedObjectLocation->GetComponentLocation());
	}
}

#pragma endregion

void AlearnUnrealForOnceCharacter::OnResetVR()
{
	// If learnUnrealForOnce is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in learnUnrealForOnce.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AlearnUnrealForOnceCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AlearnUnrealForOnceCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

#pragma region Push & Pull

void AlearnUnrealForOnceCharacter::PushPull(float Rate)
{
	if(Rate == 0) return;

	FHitResult OutHit;

	FVector Start = FollowCamera->GetComponentLocation() + (FollowCamera->GetForwardVector() * 200);
	FVector End = Start + (FollowCamera->GetForwardVector() * 4200);

	FCollisionQueryParams TraceParams(TEXT("LineOfSight_Trace"), false, this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, TraceParams, FCollisionResponseParams::DefaultResponseParam);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 2, 0, 2.0f);

	if (auto HitObj = OutHit.GetActor())
	{
		if (HitObj->IsRootComponentMovable()) 
		{
			UStaticMeshComponent* MeshRootComp = Cast<UStaticMeshComponent>(HitObj->GetRootComponent());

			MeshRootComp->AddForce(FollowCamera->GetForwardVector() * 2000 * MeshRootComp->GetMass() * Rate);
		}
	}
}

#pragma endregion

void AlearnUnrealForOnceCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AlearnUnrealForOnceCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AlearnUnrealForOnceCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AlearnUnrealForOnceCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
