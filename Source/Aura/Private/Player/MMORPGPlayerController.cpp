// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MMORPGPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Actor/MagicCircle.h"
#include "Aura/Aura.h"
#include "Character/AuraCharacter.h"
#include "Components/DecalComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"
#include "Input/AuraInputComponent.h"
#include "Interaction/EnemyInterface.h"
#include "Interaction/HighlightInterface.h"
#include "UI/Widget/DamageTextComponent.h"

AMMORPGPlayerController::AMMORPGPlayerController()
{
	bReplicates = true;
	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
}

void AMMORPGPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
	AutoRun();
	UpdateMagicCircleLocation();
}

void AMMORPGPlayerController::ShowMagicCircle(UMaterialInterface* DecalMaterial)
{
	if (!IsValid(MagicCircle))
	{
		MagicCircle = GetWorld()->SpawnActor<AMagicCircle>(MagicCircleClass);
		if (DecalMaterial)
		{
			MagicCircle->MagicCircleDecal->SetMaterial(0, DecalMaterial);
		}
	}
}

void AMMORPGPlayerController::HideMagicCircle()
{
	if (IsValid(MagicCircle))
	{
		MagicCircle->Destroy();
	}
}

void AMMORPGPlayerController::ShowDamageNumber_Implementation(float DamageAmount, ACharacter* TargetCharacter,
                                                              bool bBlockedHit, bool bCriticalHit)
{
	if (IsValid(TargetCharacter) && DamageTextComponentClass && IsLocalController())
	{
		UDamageTextComponent* DamageText = NewObject<UDamageTextComponent>(TargetCharacter, DamageTextComponentClass);
		DamageText->RegisterComponent();
		DamageText->AttachToComponent(TargetCharacter->GetRootComponent(),
		                              FAttachmentTransformRules::KeepRelativeTransform);
		DamageText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		DamageText->SetDamageText(DamageAmount, bBlockedHit, bCriticalHit);
	}
}

void AMMORPGPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(DefaultMappingContext);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);

	SetControlRotation(FRotator(-45.f, 0.f, 0.f));
}

void AMMORPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UAuraInputComponent* EtherniaInputComponent = CastChecked<UAuraInputComponent>(InputComponent);

	// Moving
	EtherniaInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMMORPGPlayerController::Move);

	// Jumping
	EtherniaInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMMORPGPlayerController::Jump);
	EtherniaInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
	                                   &AMMORPGPlayerController::StopJumping);

	// Looking
	EtherniaInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMMORPGPlayerController::Look);

	// MouseWheel
	EtherniaInputComponent->BindAction(MouseWheelAction, ETriggerEvent::Triggered, this,
	                                   &AMMORPGPlayerController::UpdateTargetArmLength);

	// Right Click
	EtherniaInputComponent->BindAction(RightClickAction, ETriggerEvent::Started, this,
	                                   &AMMORPGPlayerController::RightClickPressed);
	EtherniaInputComponent->BindAction(RightClickAction, ETriggerEvent::Completed, this,
	                                   &AMMORPGPlayerController::RightClickReleased);

	// Shift
	EtherniaInputComponent->BindAction(ShiftAction, ETriggerEvent::Started, this,
	                                   &AMMORPGPlayerController::ShiftPressed);
	EtherniaInputComponent->BindAction(ShiftAction, ETriggerEvent::Completed, this,
	                                   &AMMORPGPlayerController::ShiftReleased);


	// AbilityInput actions
	EtherniaInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed,
	                                           &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
}

void AMMORPGPlayerController::Move(const FInputActionValue& InputActionValue)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void AMMORPGPlayerController::Look(const FInputActionValue& Value)
{
	if (!bRightClickKeyDown)
	{
		return;
	}
	// input is a Vector2D
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddControllerYawInput(LookAxisVector.X);
		ControlledPawn->AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMMORPGPlayerController::Jump(const FInputActionValue& Value)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}

	if (ACharacter* PlayerCharacter = GetCharacter())
	{
		PlayerCharacter->Jump();
	}
}

void AMMORPGPlayerController::StopJumping(const FInputActionValue& Value)
{
	if (ACharacter* PlayerCharacter = GetCharacter())
	{
		PlayerCharacter->StopJumping();
	}
}

void AMMORPGPlayerController::UpdateTargetArmLength(const FInputActionValue& Value)
{
	if (const AAuraCharacter* PlayerCharacter = Cast<AAuraCharacter>(GetCharacter()))
	{
		const float AxisVector = Value.Get<float>();
		PlayerCharacter->SetTargetArmLength(200.f * AxisVector);
	}
}

void AMMORPGPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	/** Avoid to execute player actions if player is blocked to do actions */
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}

	if (InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (IsValid(CurrentPointedActor))
		{
			TargetingStatus = CurrentPointedActor->Implements<UEnemyInterface>()
				                  ? ETargetingStatus::TargetingEnemy
				                  : ETargetingStatus::TargetingNonEnemy;

			bTargeting = CurrentPointedActor->Implements<UEnemyInterface>();
		}
		else
		{
			TargetingStatus = ETargetingStatus::NotTargeting;
			bTargeting = false;
		}
		bAutoRunning = false;
	}

	if (IsValid(TargetActor))
	{
		if (GetASC()) GetASC()->AbilityInputTagPressed(InputTag);
	}
}

void AMMORPGPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	/** Avoid to execute player actions if player is blocked to do actions */
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputReleased))
	{
		return;
	}
	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC()) GetASC()->AbilityInputTagReleased(InputTag);
	}
	else
	{
		if (TargetingStatus != ETargetingStatus::TargetingEnemy && !bShiftKeyDown)
		{
			const APawn* ControlledPawn = GetPawn();
			if (InputElapsedTimeHeld <= ShortPressThreshold && ControlledPawn)
			{
				if (IsValid(CurrentPointedActor) && CurrentPointedActor->Implements<UHighlightInterface>())
				{
					IHighlightInterface::Execute_SetMoveToLocation(CurrentPointedActor, CachedDestination);
				}
				else if (GetASC() && !GetASC()->HasMatchingGameplayTag(
					FAuraGameplayTags::Get().Player_Block_InputPressed))
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ClickDecal, CachedDestination);
				}
				if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(
					this, ControlledPawn->GetActorLocation(), CachedDestination))
				{
					Spline->ClearSplinePoints();
					for (const FVector& PointLoc : NavPath->PathPoints)
					{
						Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
					}
					if (NavPath->PathPoints.Num() > 0)
					{
						CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];
						bAutoRunning = true;
					}
				}
			}
			InputElapsedTimeHeld = 0.f;
			TargetingStatus = ETargetingStatus::NotTargeting;
			bTargeting = false;
		}
		else
		{
			if (IsValid(CurrentPointedActor))
			{
				if (CurrentPointedActor != TargetActor)
				{
					TargetActor = CurrentPointedActor;
				}
				else
				{
					if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
					// if (GetASC()) GetASC()->AbilityInputTagReleased(InputTag);
				}
			}
		}
	}
}

void AMMORPGPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	/** Avoid to execute player actions if player is blocked to do actions */
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputHeld))
	{
		return;
	}
	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
	}
	else
	{
		if (TargetingStatus != ETargetingStatus::TargetingEnemy && !bShiftKeyDown)
		{
			InputElapsedTimeHeld += GetWorld()->GetDeltaSeconds();
			if (CursorHit.bBlockingHit) CachedDestination = CursorHit.ImpactPoint;

			if (APawn* ControlledPawn = GetPawn())
			{
				const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
				ControlledPawn->AddMovementInput(WorldDirection);
			}
		}
	}
}

UAuraAbilitySystemComponent* AMMORPGPlayerController::GetASC()
{
	if (EtherniaAbilitySystemComponent == nullptr)
	{
		EtherniaAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return EtherniaAbilitySystemComponent;
}

void AMMORPGPlayerController::AutoRun()
{
	if (!bAutoRunning) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		const FVector LocationOnSpline = Spline->FindLocationClosestToWorldLocation(
			ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		const FVector Direction = Spline->FindDirectionClosestToWorldLocation(
			LocationOnSpline, ESplineCoordinateSpace::World);
		ControlledPawn->AddMovementInput(Direction);

		const float DistanceToDestination = (LocationOnSpline - CachedDestination).Length();
		if (DistanceToDestination <= AutoRunAcceptanceRadius)
		{
			bAutoRunning = false;
		}
	}
}

void AMMORPGPlayerController::CursorTrace()
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_CursorTrace))
	{
		UnHighlightActor(PreviousPointedActor);
		UnHighlightActor(CurrentPointedActor);
		if (IsValid(CurrentPointedActor) && CurrentPointedActor->Implements<UHighlightInterface>())
		{
			PreviousPointedActor = nullptr;
		}
		CurrentPointedActor = nullptr;
		return;
	}

	const ECollisionChannel TraceChannel = IsValid(MagicCircle) ? ECC_ExcludePlayers : ECC_Visibility;
	GetHitResultUnderCursor(TraceChannel, false, CursorHit);
	if (!CursorHit.bBlockingHit) return;

	PreviousPointedActor = CurrentPointedActor;
	if (IsValid(CursorHit.GetActor()) && CursorHit.GetActor()->Implements<UHighlightInterface>())
	{
		CurrentPointedActor = CursorHit.GetActor();
	}
	else
	{
		CurrentPointedActor = nullptr;
	}

	if (PreviousPointedActor != CurrentPointedActor)
	{
		UnHighlightActor(PreviousPointedActor);
		HighlightActor(CurrentPointedActor);
	}
}

void AMMORPGPlayerController::HighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_HighlightActor(InActor);
	}
}

void AMMORPGPlayerController::UnHighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_UnHighlightActor(InActor);
	}
}

void AMMORPGPlayerController::UpdateMagicCircleLocation() const
{
	if (IsValid(MagicCircle))
	{
		MagicCircle->SetActorLocation(CursorHit.ImpactPoint);
	}
}
