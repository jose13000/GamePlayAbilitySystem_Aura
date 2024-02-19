// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "MMORPGPlayerController.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerTargetChanged, AActor* /*TargetActor*/)

class AMagicCircle;
class UDamageTextComponent;
class USplineComponent;
class UNiagaraSystem;
class UAuraAbilitySystemComponent;
struct FGameplayTag;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UAuraInputConfiguration;
class UCameraComponent;
class USpringArmComponent;

enum class ETargetingStatus : uint8
{
	TargetingEnemy,
	TargetingNonEnemy,
	NotTargeting
};

/**
 * 
 */
UCLASS()
class AURA_API AMMORPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMMORPGPlayerController();
	virtual void PlayerTick(float DeltaTime) override;

	/** Show the damage or effect result */
	UFUNCTION(Client, Reliable)
	void ShowDamageNumber(float DamageAmount, ACharacter* TargetCharacter, bool bBlockedHit, bool bCriticalHit);

	/** Show Magic Circle */
	UFUNCTION(BlueprintCallable)
	void ShowMagicCircle(UMaterialInterface* DecalMaterial = nullptr);

	/** Hide Magic Circle */
	UFUNCTION(BlueprintCallable)
	void HideMagicCircle();

	FOnPlayerTargetChanged OnTargetActorChangedDelegate;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	/** Data Asset that contains the InputActions with the Abilities Tags that should activate */
	UPROPERTY(EditDefaultsOnly, Category=Input)
	TObjectPtr<UAuraInputConfiguration> InputConfig;

	/** Input Mapping Context */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	/** RightClick Input Action */
	UPROPERTY(EditAnywhere, Category=Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> RightClickAction;
	void RightClickPressed() { bRightClickKeyDown = true; };
	void RightClickReleased() { bRightClickKeyDown = false; };
	bool bRightClickKeyDown = false;
	/** Shift Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ShiftAction;
	void ShiftPressed() { bShiftKeyDown = true; };
	void ShiftReleased() { bShiftKeyDown = false; };
	bool bShiftKeyDown = false;
	/** MouseWheel Input Action */
	UPROPERTY(EditAnywhere, Category=Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MouseWheelAction;

	/** Called for MoveAction input */
	void Move(const FInputActionValue& InputActionValue);
	/** Called for LookAction input */
	void Look(const FInputActionValue& Value);
	/** Called for JumpAction input */
	void Jump(const FInputActionValue& Value);
	void StopJumping(const FInputActionValue& Value);
	/** Called for MouseWheelAction input */
	void UpdateTargetArmLength(const FInputActionValue& Value);

	/** Abilities Input Actions */
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	/** Custom Ability System Component */
	UPROPERTY()
	TObjectPtr<UAuraAbilitySystemComponent> EtherniaAbilitySystemComponent;
	/** Get the Ability System Component */
	UAuraAbilitySystemComponent* GetASC();

	/** Current player's target */
	TObjectPtr<AActor> TargetActor;
	/** Current player's target */
	TObjectPtr<AActor> CurrentPointedActor;
	/** Previous player's target */
	TObjectPtr<AActor> PreviousPointedActor;
	/** status of the player target */
	ETargetingStatus TargetingStatus = ETargetingStatus::NotTargeting;
	UFUNCTION()
	void TargetActorDied(AActor* DeadActor);

	/** Determine if the player character is moving the a clicked point of getting close to a target */
	bool bAutoRunning = false;
	/** Current destination point where the character should move to */
	FVector CachedDestination = FVector::ZeroVector;
	/** Time elapsed since the player held down the input */
	float InputElapsedTimeHeld = 0.f;
	/** Minimum time the player must hold down the input to be considered a Held Input */
	float ShortPressThreshold = 0.5f;
	/** Decal to spawn when click on a navigable point */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> ClickDecal;
	/** NavigationMesh Points Collection */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;
	/** Move the character to each Spline collection point */
	void AutoRun();
	/** Minimum distance between the character and the spline point con consider the point reached */
	UPROPERTY(EditDefaultsOnly)
	float AutoRunAcceptanceRadius = 50.f;

	/** Do a line trace from the cursor location to the world */
	void CursorTrace();
	/** Resulting hit struct resulting of the  CursorHit() method */
	FHitResult CursorHit;
	/** Highlight a desire actor */
	static void HighlightActor(AActor* InActor);
	/** UnHighlight a desire actor */
	static void UnHighlightActor(AActor* InActor);

	/** WidgetComponent class of the damage of effects IU view */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageTextComponent> DamageTextComponentClass;

	/** Magic Circle Class */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AMagicCircle> MagicCircleClass;
	/** Magic Circle Instance */
	UPROPERTY()
	TObjectPtr<AMagicCircle> MagicCircle;
	/** Magic Circle Instance */
	void UpdateMagicCircleLocation() const;

	bool bTargeting = false;

public:
	FORCEINLINE AActor* GetTargetActor() const { return TargetActor; };
};
