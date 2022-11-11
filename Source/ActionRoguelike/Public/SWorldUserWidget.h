// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SWorldUserWidget.generated.h"

class USizeBox;
/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API USWorldUserWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	// Root of what we move across the screen
	UPROPERTY(meta = (BindWidget))	// So we can use in our BP graph later
	USizeBox* ParentSizeBox;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
public:

	UPROPERTY(EditAnywhere, Category = "UI")
	FVector WorldOffset;


	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = true))					// So we can use in our BP graph later
	AActor* AttachedActor;		// Note: Reference counting, imagine the Attached actor is our minion_ranged.
								//       If we mark this variable as UPROP, once the ai gets deleted, the pointer
								//	     will be set to point to a nullptr so we dont have to worry about 'dangling pointers'
};


