// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SGameplayInterface.h"
#include <Components/StaticMeshComponent.h>
#include "SItemChest.generated.h"

UCLASS()
class ACTIONROGUELIKE_API ASItemChest : public AActor, public ISGameplayInterface
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere)
	float TargetPitch;
	void Interact_Implementation(APawn* InstigatorPawn);  // Syntax is because we used "UFUNCTION(BlueprintNativeEvent)"

protected:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* LidMesh;

public:
	ASItemChest();
};
