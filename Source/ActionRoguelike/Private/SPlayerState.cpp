// Fill out your copyright notice in the Description page of Project Settings.


#include "SPlayerState.h"

void ASPlayerState::AddCredits(int32 Delta)
{
	if (!ensure(Delta > 0.0f))	// Avoid user-error of adding a negative amount or zero
		return;

	Credits += Delta;

	OnCreditsChanged.Broadcast(this, Credits, Delta);
}


bool ASPlayerState::RemoveCredits(int32 Delta)
{
	// Avoid user-error of adding a subtracting negative amount or zero
	if (!ensure(Delta > 0.0f))
		return false;
	
	if (Credits < Delta)
		return false;	// Not enough credits available

	Credits -= Delta;

	OnCreditsChanged.Broadcast(this, Credits, -Delta);

	return true;
}