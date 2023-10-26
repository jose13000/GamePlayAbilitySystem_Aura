// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuraUserWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SerWidgetController(UObject* InWidgetController);
	
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> WidgetController;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void WidgetControllerSet();
};
