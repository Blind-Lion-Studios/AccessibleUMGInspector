// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TestConcreteUserWidget.generated.h"

/**
 * Concrete UUserWidget subclass for synthetic unit tests.
 * Inherits from UUserWidget without the Abstract class flag, preventing
 * engine-level abstract class instantiation warnings.
 */
UCLASS()
class ACCESSIBLEUMGINSPECTORTESTS_API UTestConcreteUserWidget : public UUserWidget
{
	GENERATED_BODY()
};
