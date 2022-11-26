
#include "SAnimInstance.h"
#include "GameplayTagContainer.h"
#include "SActionComponent.h"

void USAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Get player, get the value of this players IsStunned tag from attr comp
	AActor* OwningActor = GetOwningActor();
	if (OwningActor)
		ActionComp = Cast<USActionComponent>(OwningActor->GetComponentByClass(USActionComponent::StaticClass()));
}

void USAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	static FGameplayTag StunnedTag = FGameplayTag::RequestGameplayTag("Status.Stunned");

	// Grab the value of the players 'stunned' tag (i.e. active or not)
	if (ActionComp)
		bIsStunned = ActionComp->ActiveGameplayTags.HasTag(StunnedTag);
}