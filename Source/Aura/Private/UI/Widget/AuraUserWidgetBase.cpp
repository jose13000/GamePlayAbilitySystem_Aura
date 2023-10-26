// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/AuraUserWidgetBase.h"

void UAuraUserWidgetBase::SerWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}
