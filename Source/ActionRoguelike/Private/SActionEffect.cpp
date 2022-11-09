#include "SActionEffect.h"
#include "SActionComponent.h"

USActionEffect::USActionEffect()
{
	bAutoStart = true;
}

void USActionEffect::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);


	// Only exe if duration is > 0 (i.e. some buffs are instant cast)
	if (Duration > 0.0f)
	{
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "StopAction", Instigator);

		GetWorld()->GetTimerManager().SetTimer(DurationHandle, Delegate, Duration, false);
	}

	if (Period > 0.0f)
	{
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "ExecutePeriodicEffect", Instigator);

		GetWorld()->GetTimerManager().SetTimer(PeriodHandle, Delegate, Period, true);
	}
}

void USActionEffect::StopAction_Implementation(AActor* Instigator)
{
	// Apply damage (So we dont miss out on last tick, check the remaining time on period handle before calling super)
	if (GetWorld()->GetTimerManager().GetTimerRemaining(PeriodHandle) < KINDA_SMALL_NUMBER)
		ExecutePeriodicEffect(Instigator);

	Super::StopAction_Implementation(Instigator);  // Call parents stop action

	GetWorld()->GetTimerManager().ClearTimer(PeriodHandle);  // Stop timers
	GetWorld()->GetTimerManager().ClearTimer(DurationHandle);

	USActionComponent* Comp = GetOwningComponent();
	if (Comp)
		Comp->RemoveAction(this);   // Remove this action from the respective players ActionComp
}									// As long as no one is hanging onto this action, UE will delete eventually with GarbCol

void USActionEffect::ExecutePeriodicEffect_Implementation(AActor* Instigator)
{

}