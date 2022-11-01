// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/SAICharacter.h"
#include <Perception/PawnSensingComponent.h>
#include <BehaviorTree/BlackboardComponent.h>
#include <AIController.h>
#include <DrawDebugHelpers.h>
#include "SAttributeComponent.h"
#include "BrainComponent.h"


// Sets default values
ASAICharacter::ASAICharacter()
{
	PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");
	AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");


	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	TimeToHitParamName = "TimeToHit";
}

void ASAICharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	PawnSensingComp->OnSeePawn.AddDynamic(this, &ASAICharacter::OnPawnSeen);
	AttributeComp->OnHealthChanged.AddDynamic(this, &ASAICharacter::OnHealthChanged);
}

void ASAICharacter::OnHealthChanged(AActor* InstigatorActor, USAttributeComponent* OwningComp, float NewHealth, float Delta)
{
	if (Delta < 0.0f) // Implies that the input is damage not healing
	{
		// Make sure that we are not the inst so we dont damage ourself
		if (InstigatorActor != this)
		{
			SetTargetActor(InstigatorActor);
		}

		GetMesh()->SetScalarParameterValueOnMaterials(TimeToHitParamName, GetWorld()->TimeSeconds);

		if (NewHealth <= 0.0f)  // Just died
		{
			// stop BT: Get BT from AI controller
			AAIController* AIC = Cast<AAIController>(GetController());
			if (AIC)
			{
				// GetBrainComponent is base class of BT..StopLogic('Reason')
				AIC->GetBrainComponent()->StopLogic("Killed");
			}

			// ragdoll
			GetMesh()->SetAllBodiesSimulatePhysics(true);

			// Current collision profile does not include physics sim --> change this to adopt ragdol sim @ death 
			GetMesh()->SetCollisionProfileName("Ragdoll");

			// set lifespan  -- Disapears after lifespan 
			SetLifeSpan(10.0f);
		}
	}
}


void ASAICharacter::SetTargetActor(AActor* NewTarget)
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC)
	{
		AIC->GetBlackboardComponent()->SetValueAsObject("TargetActor", NewTarget);
	}
}


void ASAICharacter::OnPawnSeen(APawn* Pawn)
{
	SetTargetActor(Pawn);

	DrawDebugString(GetWorld(), GetActorLocation(), "PLAYER SPOTTED", nullptr, FColor::White, 0.5f, true);
}