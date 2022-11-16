 // Fill out your copyright notice in the Description page of Project Settings.

#include "SGameModeBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "AI/SAICharacter.h"
#include "SAttributeComponent.h"
#include "EngineUtils.h"
#include <DrawDebugHelpers.h>
#include "SCharacter.h"
#include "SPlayerState.h"
#include "SSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "SGameplayInterface.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

static TAutoConsoleVariable<bool> CVarSpawnBots(TEXT("su.SpawnBots"), true, TEXT("Enable spawning of bots via Timer."), ECVF_Cheat);

ASGameModeBase::ASGameModeBase()
{
	SpawnTimerInterval = 2.0f;

	CreditsPerKill = 20;

	DesiredPowerupCount = 10;
	RequiredPowerupDistance = 2000;

	PlayerStateClass = ASPlayerState::StaticClass();
	SlotName = "SaveGame01";
}


void ASGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	LoadSaveGame();
}

void ASGameModeBase::StartPlay()
{
	Super::StartPlay();

	// Continuous timer to spawn in more bots.
	// Actual amount of bots and whether its allowed to spawn determined by spawn logic later in the chain...
	GetWorldTimerManager().SetTimer(TimerHandle_SpawnBots, this, &ASGameModeBase::SpawnBotTimerElapsed, SpawnTimerInterval, true);

	// Make sure we have assigned at least one power-up class
	if (ensure(PowerupClasses.Num() > 0))
	{
		// Run EQS to find potential power-up spawn locations
		UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, PowerupSpawnQuery, this, EEnvQueryRunMode::AllMatching, nullptr);
		if (ensure(QueryInstance))
			QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnPowerupSpawnQueryCompleted);
	}
}
void ASGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	ASPlayerState* PS = NewPlayer->GetPlayerState<ASPlayerState>();

	if (ensure(PS))
		PS->LoadPlayerState(CurrentSaveGame);

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}
void ASGameModeBase::KillAll()
{
	// Iterator allows traversal through all SCharacter and derived instances
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It)
	{
		ASAICharacter* Bot = *It;
		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Bot);
		if (ensure(AttributeComp) && AttributeComp->IsAlive())
			AttributeComp->Kill(this);  // @fixme: Pass in player? for kill credit
	}
}

void ASGameModeBase::WriteSaveGame()
{
	// Iterate all player states, we don't have proper ID to match yet (requires Steam or EOS)
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		ASPlayerState* PS = Cast<ASPlayerState>(GameState->PlayerArray[i]);
		if (PS)
		{
			PS->SavePlayerState(CurrentSaveGame);
			break; // single player only at this point
		}
	}

	CurrentSaveGame->SavedActors.Empty();

	// Iterate the entire world of actors
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		// Only interested in our 'gameplay actors'
		if (!Actor->Implements<USGameplayInterface>())
		{
			continue;
		}

		FActorSaveData ActorData;
		ActorData.ActorName = Actor->GetName();
		ActorData.Transform = Actor->GetActorTransform();

		// Pass the array to fill with data from Actor
		FMemoryWriter MemWriter(ActorData.ByteData);

		FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
		// Find only variables with UPROPERTY(SaveGame)
		Ar.ArIsSaveGame = true;
		// Converts Actor's SaveGame UPROPERTIES into binary array
		Actor->Serialize(Ar);

		CurrentSaveGame->SavedActors.Add(ActorData);
	}

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, 0);
}


void ASGameModeBase::LoadSaveGame()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveGame = Cast<USSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (CurrentSaveGame == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load SaveGame Data."));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Loaded SaveGame Data."));


		// Iterate the entire world of actors
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			// Only interested in our 'gameplay actors'
			if (!Actor->Implements<USGameplayInterface>())
			{
				continue;
			}

			for (FActorSaveData ActorData : CurrentSaveGame->SavedActors)
			{
				if (ActorData.ActorName == Actor->GetName())
				{
					Actor->SetActorTransform(ActorData.Transform);

					FMemoryReader MemReader(ActorData.ByteData);

					FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
					Ar.ArIsSaveGame = true;
					// Convert binary array back into actor's variables
					Actor->Serialize(Ar);

					ISGameplayInterface::Execute_OnActorLoaded(Actor);

					break;
				}
			}
		}
	}
	else
	{
		CurrentSaveGame = Cast<USSaveGame>(UGameplayStatics::CreateSaveGameObject(USSaveGame::StaticClass()));

		UE_LOG(LogTemp, Log, TEXT("Created New SaveGame Data."));
	}
}

void ASGameModeBase::SpawnBotTimerElapsed()
{
	if (!CVarSpawnBots.GetValueOnGameThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("Bot spawning disabled via cvar 'CVarSpawnBots'."));
		return;
	}
	// Make sure we have a limit for the number of bots in a level
	int32 NrOfAliveBots = 0;

	// Iterator allows traversal through all SCharacter and derived instances
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It)
	{
		ASAICharacter* Bot = *It;
		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Bot);
		if (AttributeComp && AttributeComp->IsAlive())
			NrOfAliveBots++;
	}

	UE_LOG(LogTemp, Log, TEXT("Found %i alive bots."), NrOfAliveBots);
	float MaxBotCount = 10.0f;

	// Curve specifies how many bots are aloud during certain periods of gameplay
	if (DifficultyCurve)
		MaxBotCount = DifficultyCurve->GetFloatValue(GetWorld()->TimeSeconds);

	// Return if we're past the bot limit
	if (NrOfAliveBots >= MaxBotCount)
	{
		UE_LOG(LogTemp, Log, TEXT("AI maximum bot capacity. Skipping bot spawn."));
		return;
	}


	// Will give us event trigger once we're done
	UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, SpawnBotQuery, this, EEnvQueryRunMode::RandomBest5Pct, nullptr);
	if (ensure(QueryInstance))
		QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnBotSpawnQueryCompleted);
}

void ASGameModeBase::OnBotSpawnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn bot EQS Query Failed!"));
		return;
	}

	// Get list of locations
	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations();

	// Spawn AI at first (valid) location in returned query list
	if (Locations.IsValidIndex(0))
	{
		GetWorld()->SpawnActor<AActor>(MinionClass, Locations[0], FRotator::ZeroRotator);
		// Track all the used spawn locations
		DrawDebugSphere(GetWorld(), Locations[0], 50.0f, 20, FColor::Blue, false, 60.0f);
	}
}

void ASGameModeBase::OnPowerupSpawnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn bot EQS Query Failed!"));
		return;
	}

	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations();

	// Keep used locations to easily check distance between points
	TArray<FVector> UsedLocations;

	int32 SpawnCounter = 0;
	// Break out if we reached the desired count or if we have no more potential positions remaining
	while (SpawnCounter < DesiredPowerupCount && Locations.Num() > 0)
	{
		// Pick a random location from remaining points.
		int32 RandomLocationIndex = FMath::RandRange(0, Locations.Num() - 1);

		FVector PickedLocation = Locations[RandomLocationIndex];
		// Remove to avoid picking again
		Locations.RemoveAt(RandomLocationIndex);

		// Check minimum distance requirement
		bool bValidLocation = true;
		for (FVector OtherLocation : UsedLocations)
		{
			float DistanceTo = (PickedLocation - OtherLocation).Size();

			if (DistanceTo < RequiredPowerupDistance)
			{
				// Show skipped locations due to distance
				//DrawDebugSphere(GetWorld(), PickedLocation, 50.0f, 20, FColor::Red, false, 10.0f);

				// too close, skip to next attempt
				bValidLocation = false;
				break;
			}
		}

		// Failed the distance test
		if (!bValidLocation)
		{
			continue;
		}

		// Pick a random powerup-class
		int32 RandomClassIndex = FMath::RandRange(0, PowerupClasses.Num() - 1);
		TSubclassOf<AActor> RandomPowerupClass = PowerupClasses[RandomClassIndex];

		GetWorld()->SpawnActor<AActor>(RandomPowerupClass, PickedLocation, FRotator::ZeroRotator);

		// Keep for distance checks
		UsedLocations.Add(PickedLocation);
		SpawnCounter++;
	}
}

void ASGameModeBase::RespawnPlayerElapsed(AController* Controller)
{
	if (ensure(Controller))
	{
		Controller->UnPossess();
		RestartPlayer(Controller);
	}
}

void ASGameModeBase::OnActorKilled(AActor* VictimActor, AActor* Killer)
{
	UE_LOG(LogTemp, Log, TEXT("OnActorKilled: Victim: %s, Killer: %s"), *GetNameSafe(VictimActor), *GetNameSafe(Killer));

	ASCharacter* Player = Cast<ASCharacter>(VictimActor);
	if (Player)
	{
		FTimerHandle TimeHandle_RespawnDelay;
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "RespawnPlayerElapsed", Player->GetController());

		float RespawnDelay = 2.0f;
		GetWorldTimerManager().SetTimer(TimeHandle_RespawnDelay, Delegate, 2.0f, false);
	}

	// Give Credits for kill
	APawn* KillerPawn = Cast<APawn>(Killer);
	if (KillerPawn)
	{
		if (ASPlayerState* PS = KillerPawn->GetPlayerState<ASPlayerState>()) // < can cast and check for nullptr within if-statement.
		{
			PS->AddCredits(CreditsPerKill);
		}
	}
}