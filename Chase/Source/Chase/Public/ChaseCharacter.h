// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"
#include "ChaseCharacter.generated.h"

UENUM(BlueprintType)
enum class EChaseTeam : uint8
{
	Chaser,
	Victim
};

UCLASS(config=Game)
class AChaseCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	AChaseCharacter(const FObjectInitializer& ObjectInitializer);

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float RunSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float ChaserWalkSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Game, Replicated)
	EChaseTeam Team;

	UFUNCTION(BlueprintImplementableEvent)
	void GameStateChanged();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
	UMaterialInterface* VictimMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
	UMaterialInterface* ChaserMaterial;

protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	virtual void Jump() override;

	virtual void AddControllerYawInput(float Val) override;

	virtual void AddControllerPitchInput(float Val) override;

	virtual void Tackle();

	bool IsMovementEnabled(bool CheckOnlyState = false);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Game, Replicated)
	float ChargeTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Game, Replicated)
	float ChargeCooldownTimer;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

	UFUNCTION(reliable, server, WithValidation)
	void Charge();

	UFUNCTION()
	void HitOtherActor(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	UPROPERTY(Replicated)
	bool InputEnabled;
};

