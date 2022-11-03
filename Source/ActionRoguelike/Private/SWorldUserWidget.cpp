// Fill out your copyright notice in the Description page of Project Settings.


#include "SWorldUserWidget.h"
#include <Kismet/GameplayStatics.h>
#include <Components/SizeBox.h>
#include <Blueprint/WidgetLayoutLibrary.h>

void USWorldUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Check for null and IsPendingKill
	if (!IsValid(AttachedActor))
	{
		RemoveFromParent();

		UE_LOG(LogTemp, Warning, TEXT("AttachedActor is no longer valid, removing it from HealthWidget"));
		return;
	}



	FVector2D ScreenPosition;
	
	// If a valid position was indeed found
	if (UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), AttachedActor->GetActorLocation() + WorldOffset, ScreenPosition))
	{
		float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
		ScreenPosition /= Scale;
	
		if (ParentSizeBox)
			ParentSizeBox->SetRenderTranslation(ScreenPosition);
	}
}
