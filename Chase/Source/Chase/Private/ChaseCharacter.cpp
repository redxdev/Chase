// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Chase.h"
#include "ChaseCharacter.h"
#include "Engine.h"
#include "ChaseGameState.h"
#include "UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AChaseCharacter

AChaseCharacter::AChaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FollowCamera"));
	FollowCamera->AttachTo(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	InputEnabled = true;

	PrimaryActorTick.bCanEverTick = true;

	WalkSpeed = 600;
	ChaserWalkSpeed = 500;
	RunSpeed = 1000;
	PowerupSpeed = 0;

	Team = EChaseTeam::Chaser;

	SetActorEnableCollision(true);
	OnActorHit.AddDynamic(this, &AChaseCharacter::HitOtherActor);

	VictimMaterial = NULL;
	ChaserMaterial = NULL;
}

void AChaseCharacter::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChaseCharacter, Team);
	DOREPLIFETIME(AChaseCharacter, InputEnabled);
	DOREPLIFETIME(AChaseCharacter, ChargeTimer);
	DOREPLIFETIME(AChaseCharacter, ChargeCooldownTimer);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AChaseCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("Jump", IE_Pressed, this, &AChaseCharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAxis("MoveForward", this, &AChaseCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AChaseCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AChaseCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AChaseCharacter::LookUpAtRate);

	InputComponent->BindAction("Tackle", IE_Pressed, this, &AChaseCharacter::Tackle);
}

void AChaseCharacter::TurnAtRate(float Rate)
{
	if (IsMovementEnabled())
	{
		// calculate delta for this frame from the rate information
		AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void AChaseCharacter::LookUpAtRate(float Rate)
{
	if (IsMovementEnabled())
	{
		// calculate delta for this frame from the rate information
		AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
	}
}

void AChaseCharacter::MoveForward(float Value)
{
	if (IsMovementEnabled() && (Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AChaseCharacter::MoveRight(float Value)
{
	if (IsMovementEnabled() && (Controller != NULL) && (Value != 0.0f))
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

void AChaseCharacter::Jump()
{
	if (IsMovementEnabled())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor(100, 149, 237), TEXT("LOL JUMPED"));
		Super::Jump();
	}
}

void AChaseCharacter::AddControllerYawInput(float Val)
{
	if (IsMovementEnabled(true))
	{
		Super::AddControllerYawInput(Val / (ChargeTimer > 0.f ? 8.f : 1.f));
	}
}

void AChaseCharacter::AddControllerPitchInput(float Val)
{
	if (IsMovementEnabled(true))
	{
		Super::AddControllerPitchInput(Val);
	}
}

void AChaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ChargeCooldownTimer > 0.0f)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

		if ((Controller != NULL))
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(Direction, ChargeTimer);
		}

		if (ChargeTimer > 0.f)
			ChargeTimer -= DeltaSeconds;
		else
			ChargeTimer = 0.f;

		ChargeCooldownTimer -= DeltaSeconds;
	}
	else
	{
		InputEnabled = true;
		GetCharacterMovement()->MaxWalkSpeed = Team == EChaseTeam::Chaser ? ChaserWalkSpeed : WalkSpeed;
	}

	GetCharacterMovement()->MaxWalkSpeed += PowerupSpeed;
}

void AChaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (Role < ROLE_Authority)
		return;

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AChaseCharacter::Tackle()
{
	Charge();
}

bool AChaseCharacter::Charge_Validate()
{
	return true;
}

void AChaseCharacter::Charge_Implementation()
{
	if (IsMovementEnabled(true) && Team == EChaseTeam::Chaser)
	{
		ChargeTimer = Team == EChaseTeam::Victim ? 5.f : 5.0f;
		ChargeCooldownTimer = 7.f;
		InputEnabled = false;
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
}

bool AChaseCharacter::IsMovementEnabled(bool CheckOnlyState)
{
	if (!CheckOnlyState && !InputEnabled)
		return false;

	AChaseGameState* GameState = Cast<AChaseGameState>(UGameplayStatics::GetGameState(this));
	if (GameState && GameState->IsValidLowLevel())
	{
		if (Team == EChaseTeam::Victim)
		{
			return GameState->CurrentGameState == EChaseGameState::Setup || GameState->CurrentGameState == EChaseGameState::Playing;
		}
		else
		{
			return GameState->CurrentGameState == EChaseGameState::Playing;
		}
	}

	return true;
}

void AChaseCharacter::HitOtherActor(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Role < ROLE_Authority)
		return;

	if (Team == EChaseTeam::Victim)
		return;

	AChaseGameState* State = Cast<AChaseGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if (!State || !State->IsValidLowLevel())
		return;

	if (State->CurrentGameState != EChaseGameState::Playing)
		return;

	AChaseCharacter* OtherCharacter = Cast<AChaseCharacter>(OtherActor);
	if (!OtherCharacter || !OtherCharacter->IsValidLowLevel())
		return;

	if (OtherCharacter->Team == EChaseTeam::Victim)
	{
		State->FinishGame(this);
	}
}